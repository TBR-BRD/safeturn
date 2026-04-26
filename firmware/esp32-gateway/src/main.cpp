#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include <algorithm>
#include "safeturn_protocol.h"

// Hardware MVP: one ESP32 gateway in the LKW, powered from cigarette lighter / USB adapter.
// Buzzer: use an ACTIVE 3.3V buzzer module. Default GPIO 25. Change if needed.
static constexpr int BUZZER_PIN = 25;
static constexpr int STATUS_LED_PIN = 26;
static constexpr int WARN_LED_PIN = 27;
static constexpr int TEST_BUTTON_PIN = 34; // optional external pull-up/down recommended
static constexpr bool BUZZER_ACTIVE_HIGH = true;

static NimBLECharacteristic* vehicleStateChar = nullptr;
static NimBLECharacteristic* vruUpdateChar = nullptr;
static NimBLECharacteristic* gatewayStatusChar = nullptr;
static NimBLECharacteristic* warningEventChar = nullptr;
static NimBLECharacteristic* detectedVruChar = nullptr;
static NimBLECharacteristic* settingsChar = nullptr;
static NimBLEScan* bleScan = nullptr;

static VehicleState vehicle;
static std::vector<VruData> vruList;
static int connectedClients = 0;
static uint32_t lastStatusMs = 0;
static uint32_t lastRiskMs = 0;
static uint32_t lastScanRestartMs = 0;
static uint32_t lastWarningNotifyMs = 0;
static uint32_t lastWarnLedMs = 0;
static bool statusLedOn = false;

static constexpr uint32_t VRU_TTL_MS = 4500;
static constexpr uint32_t VEHICLE_TTL_MS = 3000;
static constexpr uint32_t RISK_INTERVAL_MS = 300;
static constexpr uint32_t STATUS_INTERVAL_MS = 1000;
static constexpr int RSSI_FALLBACK_WARN = -70;
static constexpr int RSSI_FALLBACK_HIGH = -62;

struct BuzzerState {
  int level = 0;
  uint32_t nextToggleMs = 0;
  uint32_t lastPatternMs = 0;
  int step = -1;
  bool isOn = false;
};
static BuzzerState buzzer;

static void buzzerWrite(bool on) {
  digitalWrite(BUZZER_PIN, (on == BUZZER_ACTIVE_HIGH) ? HIGH : LOW);
}

static uint16_t buzzerDurationForStep(int level, int step) {
  // Alternating ON/OFF durations. Step 0 = ON duration.
  static const uint16_t highPattern[] = {90, 70, 90, 70, 260, 0};
  static const uint16_t mediumPattern[] = {180, 0};
  static const uint16_t lowPattern[] = {60, 0};
  const uint16_t* pattern = lowPattern;
  if (level >= 3) pattern = highPattern;
  else if (level == 2) pattern = mediumPattern;
  return pattern[step];
}

static void requestBuzzer(int level) {
  if (level <= 0) return;
  const uint32_t now = millis();
  const uint32_t minInterval = level >= 3 ? 1200 : 2200;
  if (now - buzzer.lastPatternMs < minInterval) return;

  buzzer.level = level;
  buzzer.step = 0;
  buzzer.isOn = true;
  buzzer.lastPatternMs = now;
  buzzer.nextToggleMs = now + buzzerDurationForStep(level, 0);
  buzzerWrite(true);
}

static void updateBuzzer() {
  if (buzzer.step < 0) return;
  uint32_t now = millis();
  if (now < buzzer.nextToggleMs) return;

  buzzer.step++;
  uint16_t duration = buzzerDurationForStep(buzzer.level, buzzer.step);
  if (duration == 0) {
    buzzerWrite(false);
    buzzer.step = -1;
    buzzer.isOn = false;
    return;
  }

  buzzer.isOn = !buzzer.isOn;
  buzzerWrite(buzzer.isOn);
  buzzer.nextToggleMs = now + duration;
}

static void setCharacteristicAndNotify(NimBLECharacteristic* chr, const String& value) {
  if (!chr) return;
  chr->setValue(value.c_str());
  if (connectedClients > 0) chr->notify();
}

static std::vector<String> splitString(const String& input, char separator) {
  std::vector<String> parts;
  int start = 0;
  while (true) {
    int idx = input.indexOf(separator, start);
    if (idx < 0) {
      parts.push_back(input.substring(start));
      break;
    }
    parts.push_back(input.substring(start, idx));
    start = idx + 1;
  }
  return parts;
}

static uint16_t hashStringToTempId(const String& text) {
  uint16_t h = 0xA55A;
  for (size_t i = 0; i < text.length(); i++) {
    h = static_cast<uint16_t>((h << 5) ^ (h >> 1) ^ static_cast<uint8_t>(text[i]));
  }
  return h;
}

static uint16_t hashAddressToTempId(const std::string& addr) {
  uint16_t h = 0xA55A;
  for (char c : addr) {
    h = static_cast<uint16_t>((h << 5) ^ (h >> 1) ^ static_cast<uint8_t>(c));
  }
  return h;
}

static void pruneOldVrus() {
  uint32_t now = millis();
  vruList.erase(
    std::remove_if(vruList.begin(), vruList.end(), [now](const VruData& v) {
      return (now - v.lastSeenMs) > VRU_TTL_MS;
    }),
    vruList.end()
  );
}

static void upsertVru(const VruData& data) {
  for (auto& item : vruList) {
    if (item.tempId == data.tempId) {
      item = data;
      return;
    }
  }
  vruList.push_back(data);
}

static bool isInterestingService(NimBLEAdvertisedDevice* dev) {
  if (!dev) return false;
  return dev->isAdvertisingService(NimBLEUUID(ST_VRU_SERVICE_UUID));
}

static bool vehicleFresh() {
  return vehicle.valid && (millis() - vehicle.lastUpdateMs) <= VEHICLE_TTL_MS;
}

static void parseVehicleState(const String& text) {
  // v0.2 expected: V;lat;lon;speedKmh;headingDeg;accuracyM;truck
  auto parts = splitString(text, ';');
  if (parts.size() < 7 || parts[0] != "V") {
    Serial.println("[WARN] Invalid vehicle state: " + text);
    return;
  }

  vehicle.valid = true;
  vehicle.lat = atof(parts[1].c_str());
  vehicle.lon = atof(parts[2].c_str());
  vehicle.speedKmh = parts[3].toFloat();
  vehicle.headingDeg = static_cast<uint16_t>(parts[4].toInt() % 360);
  vehicle.accuracyM = parts[5].toFloat();
  vehicle.vehicleType = parts[6];
  vehicle.lastUpdateMs = millis();
  Serial.println("[VEHICLE] " + text);
}

static void parseVruGpsUpdate(const String& text, int rssiHint) {
  // Expected from iPhone cyclist app: C;tempId;role;lat;lon;speedKmh;headingDeg;accuracyM
  auto parts = splitString(text, ';');
  if (parts.size() < 8 || parts[0] != "C") {
    Serial.println("[WARN] Invalid VRU update: " + text);
    return;
  }

  VruData v;
  v.valid = true;
  v.hasGps = true;
  v.tempId = hashStringToTempId(parts[1]);
  String role = parts[2];
  v.role = role == "ped" ? 2 : (role == "sco" ? 3 : 1);
  v.lat = atof(parts[3].c_str());
  v.lon = atof(parts[4].c_str());
  v.speedKmh = parts[5].toFloat();
  v.headingDeg = static_cast<uint16_t>(parts[6].toInt() % 360);
  v.accuracyM = parts[7].toFloat();
  v.rssi = rssiHint;
  v.lastSeenMs = millis();

  upsertVru(v);

  String detected = "D;" + tempIdHex(v.tempId) + ";" + String(roleToShort(v.role)) + ";" +
    String(v.rssi) + ";" + String(v.speedKmh, 1) + ";" + String(v.headingDeg) + ";gps";
  setCharacteristicAndNotify(detectedVruChar, detected);
  Serial.println("[VRU-GPS] " + detected);
}

class VehicleStateCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& connInfo) override {
    std::string value = chr->getValue();
    parseVehicleState(String(value.c_str()));
  }
};

class VruUpdateCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& connInfo) override {
    std::string value = chr->getValue();
    parseVruGpsUpdate(String(value.c_str()), -40); // Connected BLE has no RSSI in this callback; use neutral hint.
  }
};

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    NimBLEAdvertisedDevice* dev = const_cast<NimBLEAdvertisedDevice*>(advertisedDevice);
    VruData parsed;
    bool ok = false;

    if (dev->haveManufacturerData()) {
      std::string mfr = dev->getManufacturerData();
      ok = parseVruManufacturerPayload(mfr, parsed);
    }

    if (!ok && isInterestingService(dev)) {
      // iOS foreground peripheral fallback: service-only detection, no GPS.
      parsed.valid = true;
      parsed.hasGps = false;
      parsed.tempId = hashAddressToTempId(dev->getAddress().toString());
      parsed.role = 1;
      parsed.flags = 0;
      parsed.seq = 0;
      parsed.speedKmh = 0;
      parsed.headingDeg = 0;
      parsed.accuracyM = 255;
      parsed.battery = 255;
      ok = true;
    }

    if (!ok) return;

    parsed.rssi = dev->getRSSI();
    parsed.lastSeenMs = millis();
    upsertVru(parsed);

    String detected = "D;" + tempIdHex(parsed.tempId) + ";" + String(roleToShort(parsed.role)) + ";" +
      String(parsed.rssi) + ";" + String(parsed.speedKmh, 1) + ";" + String(parsed.headingDeg) + ";adv";
    setCharacteristicAndNotify(detectedVruChar, detected);
    Serial.println("[VRU-ADV] " + detected);
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    connectedClients++;
    Serial.printf("[BLE] client connected, clients=%d\n", connectedClients);
    // Keep advertising so another app can also connect in MVP tests.
    NimBLEDevice::startAdvertising();
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
    connectedClients = max(0, connectedClients - 1);
    Serial.printf("[BLE] client disconnected, clients=%d, reason=%d\n", connectedClients, reason);
    NimBLEDevice::startAdvertising();
  }
};

static RiskResult calculateGpsRisk(const VruData& vru) {
  RiskResult r;
  if (!vehicleFresh() || !vru.valid || !vru.hasGps) return r;

  r.distanceM = distanceMeters(vehicle.lat, vehicle.lon, vru.lat, vru.lon);
  float bearing = bearingDeg(vehicle.lat, vehicle.lon, vru.lat, vru.lon);
  r.relativeBearingDeg = normalize180(bearing - static_cast<float>(vehicle.headingDeg));
  r.onRight = r.relativeBearingDeg > 10.0f && r.relativeBearingDeg < 170.0f;
  r.sameDirection = headingDeltaAbs(static_cast<float>(vehicle.headingDeg), static_cast<float>(vru.headingDeg)) <= 55.0f;

  bool vehicleSlow = vehicle.speedKmh <= 40.0f;
  bool vehicleVerySlow = vehicle.speedKmh <= 25.0f;
  bool gpsOk = vehicle.accuracyM <= 25.0f && vru.accuracyM <= 25.0f;

  if (r.distanceM < 10) r.score += 55;
  else if (r.distanceM < 20) r.score += 42;
  else if (r.distanceM < 35) r.score += 24;
  else if (r.distanceM < 60) r.score += 8;

  if (r.onRight) r.score += 28;
  if (r.sameDirection) r.score += 14;
  if (vehicleVerySlow) r.score += 12;
  else if (vehicleSlow) r.score += 6;
  else r.score -= 15;

  if (vru.speedKmh > 4.0f && vru.speedKmh < 35.0f) r.score += 6;

  if (!gpsOk) r.score -= 25;
  if (vehicle.accuracyM > 50.0f || vru.accuracyM > 50.0f) r.score -= 40;

  if (r.score >= 78) r.level = 3;
  else if (r.score >= 58) r.level = 2;
  else if (r.score >= 38) r.level = 1;
  else r.level = 0;
  return r;
}

static RiskResult calculateRssiFallbackRisk(const VruData& vru) {
  RiskResult r;
  if (!vehicleFresh() || !vru.valid || vru.hasGps) return r;
  // Fallback only if no GPS from the VRU is available. This is less reliable.
  if (vehicle.speedKmh > 25.0f) return r;
  r.score = 0;
  if (vru.rssi >= RSSI_FALLBACK_HIGH) { r.score = 70; r.level = 2; }
  else if (vru.rssi >= RSSI_FALLBACK_WARN) { r.score = 45; r.level = 1; }
  return r;
}

static String warningMessageFor(const VruData& vru, const RiskResult& risk) {
  const char* subject = vru.role == 2 ? "Fussgaenger" : "Radfahrer";
  if (risk.level >= 3) return String("Achtung, ") + subject + " rechts nahe am LKW";
  if (risk.level == 2) return String(subject) + " rechts / im Nahbereich";
  if (risk.level == 1) return String(subject) + " in der Naehe";
  return "Hinweis";
}

static void evaluateRisks() {
  pruneOldVrus();
  if (!vehicleFresh() || vruList.empty()) return;

  VruData best = vruList[0];
  RiskResult bestRisk;

  for (const auto& v : vruList) {
    RiskResult risk = v.hasGps ? calculateGpsRisk(v) : calculateRssiFallbackRisk(v);
    if (risk.level > bestRisk.level || (risk.level == bestRisk.level && risk.score > bestRisk.score)) {
      bestRisk = risk;
      best = v;
    }
  }

  if (bestRisk.level <= 0) return;

  requestBuzzer(bestRisk.level);
  digitalWrite(WARN_LED_PIN, HIGH);
  lastWarnLedMs = millis();

  // Throttle BLE notifications; buzzer can still repeat via requestBuzzer.
  uint32_t now = millis();
  if (now - lastWarningNotifyMs < 900) return;
  lastWarningNotifyMs = now;

  String msg = warningMessageFor(best, bestRisk);
  String warning = "W;" + String(bestRisk.level) + ";" + String(roleToShort(best.role)) + ";" +
    tempIdHex(best.tempId) + ";" + String(best.rssi) + ";" + String(bestRisk.distanceM, 1) + ";" +
    String(bestRisk.relativeBearingDeg, 0) + ";" + msg;
  setCharacteristicAndNotify(warningEventChar, warning);

  String detected = "D;" + tempIdHex(best.tempId) + ";" + String(roleToShort(best.role)) + ";" +
    String(best.rssi) + ";" + String(best.speedKmh, 1) + ";" + String(best.headingDeg) + ";" +
    String(bestRisk.distanceM, 1);
  setCharacteristicAndNotify(detectedVruChar, detected);

  Serial.println("[RISK] " + warning + " | " + detected);
}

static void sendStatus() {
  pruneOldVrus();
  String status = "S;clients=" + String(connectedClients) +
    ";vru=" + String(vruList.size()) +
    ";veh=" + String(vehicleFresh() ? 1 : 0) +
    ";speed=" + String(vehicle.speedKmh, 0) +
    ";gpsAcc=" + String(vehicle.accuracyM, 0) +
    ";mode=gps-no-blinker";
  setCharacteristicAndNotify(gatewayStatusChar, status);
  statusLedOn = !statusLedOn;
  digitalWrite(STATUS_LED_PIN, statusLedOn ? HIGH : LOW);
  Serial.println("[STATUS] " + status);
}

static void setupGattServer() {
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* service = server->createService(ST_GATEWAY_SERVICE_UUID);

  vehicleStateChar = service->createCharacteristic(
    ST_VEHICLE_STATE_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  vehicleStateChar->setCallbacks(new VehicleStateCallbacks());

  vruUpdateChar = service->createCharacteristic(
    ST_VRU_UPDATE_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  vruUpdateChar->setCallbacks(new VruUpdateCallbacks());

  gatewayStatusChar = service->createCharacteristic(
    ST_GATEWAY_STATUS_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  gatewayStatusChar->setValue("S;clients=0;vru=0;veh=0;speed=0;gpsAcc=255;mode=gps-no-blinker");

  warningEventChar = service->createCharacteristic(
    ST_WARNING_EVENT_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  warningEventChar->setValue("W;0;none;0000;-127;0;0;OK");

  detectedVruChar = service->createCharacteristic(
    ST_DETECTED_VRU_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  detectedVruChar->setValue("D;none;unk;-127;0;0;0");

  settingsChar = service->createCharacteristic(
    ST_SETTINGS_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
  );
  settingsChar->setValue("mode=gps;noBlinker=1;buzzerPin=25;vehicleSpeedMax=40");

  service->start();

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(ST_GATEWAY_SERVICE_UUID);
  advertising->setName("SafeTurn-LKW");
  advertising->start();

  Serial.println("[BLE] GATT server advertising as SafeTurn-LKW");
}

static void setupScanner() {
  bleScan = NimBLEDevice::getScan();
  bleScan->setScanCallbacks(new ScanCallbacks(), false);
  bleScan->setActiveScan(true);
  bleScan->setInterval(80);
  bleScan->setWindow(50);
  bleScan->start(0, false, true);
  lastScanRestartMs = millis();
  Serial.println("[BLE] Scanner started for Android/SafetyTag advertisers");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("SafeTurn ESP32 Gateway v0.3 GPS + local buzzer/LED");
  Serial.println("No cloud, no mobile network, no blinker input, one in-cab gateway.");

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(WARN_LED_PIN, OUTPUT);
  pinMode(TEST_BUTTON_PIN, INPUT);
  buzzerWrite(false);
  digitalWrite(STATUS_LED_PIN, LOW);
  digitalWrite(WARN_LED_PIN, LOW);

  NimBLEDevice::init("SafeTurn-LKW");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(false, false, false);

  setupGattServer();
  setupScanner();

  // Startup beep.
  requestBuzzer(1);
}

void loop() {
  uint32_t now = millis();
  updateBuzzer();
  if (lastWarnLedMs > 0 && now - lastWarnLedMs > 1400) {
    digitalWrite(WARN_LED_PIN, LOW);
  }

  if (now - lastRiskMs >= RISK_INTERVAL_MS) {
    lastRiskMs = now;
    evaluateRisks();
  }

  if (now - lastStatusMs >= STATUS_INTERVAL_MS) {
    lastStatusMs = now;
    sendStatus();
  }

  if (bleScan && now - lastScanRestartMs > 60000) {
    lastScanRestartMs = now;
    bleScan->stop();
    bleScan->start(0, false, true);
  }

  delay(20);
}

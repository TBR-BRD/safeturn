#pragma once

#include <Arduino.h>
#include <math.h>

// SafeTurn BLE UUIDs
static const char* ST_GATEWAY_SERVICE_UUID  = "b5c45f00-9b7a-4e0d-85d4-9d6f5db8a001";
static const char* ST_VEHICLE_STATE_UUID    = "b5c45f00-9b7a-4e0d-85d4-9d6f5db8a002"; // iPhone LKW -> ESP32
static const char* ST_GATEWAY_STATUS_UUID   = "b5c45f00-9b7a-4e0d-85d4-9d6f5db8a003"; // ESP32 -> Apps
static const char* ST_WARNING_EVENT_UUID    = "b5c45f00-9b7a-4e0d-85d4-9d6f5db8a004"; // ESP32 -> Apps
static const char* ST_DETECTED_VRU_UUID     = "b5c45f00-9b7a-4e0d-85d4-9d6f5db8a005"; // ESP32 -> Apps
static const char* ST_SETTINGS_UUID         = "b5c45f00-9b7a-4e0d-85d4-9d6f5db8a006";
static const char* ST_VRU_UPDATE_UUID       = "b5c45f00-9b7a-4e0d-85d4-9d6f5db8a007"; // Fahrrad-iPhone -> ESP32

// Android / dedicated SafetyTag advertising service.
static const char* ST_VRU_SERVICE_UUID      = "b5c45f00-9b7a-4e0d-85d4-9d6f5db8b001";

// Test manufacturer ID. Replace with a real Bluetooth SIG Company ID before product use.
static constexpr uint16_t ST_TEST_COMPANY_ID = 0xFFFF;
static constexpr size_t ST_VRU_PAYLOAD_LEN = 12;

struct VehicleState {
  bool valid = false;
  double lat = 0.0;
  double lon = 0.0;
  float speedKmh = 0.0f;
  uint16_t headingDeg = 0;
  float accuracyM = 255.0f;
  String vehicleType = "truck";
  uint32_t lastUpdateMs = 0;
};

struct VruData {
  bool valid = false;
  uint16_t tempId = 0;
  uint8_t role = 1; // 1 cyclist, 2 pedestrian, 3 scooter
  uint8_t flags = 0;
  uint8_t seq = 0;
  double lat = 0.0;
  double lon = 0.0;
  float speedKmh = 0.0f;
  uint16_t headingDeg = 0;
  float accuracyM = 255.0f;
  uint8_t battery = 255;
  int rssi = -127;
  bool hasGps = false;
  uint32_t lastSeenMs = 0;
};

struct RiskResult {
  int level = 0; // 0 none, 1 low, 2 medium, 3 high
  float score = 0;
  float distanceM = 9999;
  float relativeBearingDeg = 0;
  bool onRight = false;
  bool sameDirection = false;
};

inline const char* roleToShort(uint8_t role) {
  switch (role) {
    case 1: return "cyc";
    case 2: return "ped";
    case 3: return "sco";
    default: return "unk";
  }
}

inline String tempIdHex(uint16_t id) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%04X", id);
  return String(buf);
}

static constexpr double ST_PI = 3.14159265358979323846;
inline double deg2rad(double deg) { return deg * ST_PI / 180.0; }
inline double rad2deg(double rad) { return rad * 180.0 / ST_PI; }

inline float normalize360(float deg) {
  while (deg < 0) deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;
  return deg;
}

inline float normalize180(float deg) {
  float d = normalize360(deg);
  return d > 180.0f ? d - 360.0f : d;
}

inline float headingDeltaAbs(float a, float b) {
  return fabsf(normalize180(a - b));
}

inline float distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  static constexpr double R = 6371000.0;
  double dLat = deg2rad(lat2 - lat1);
  double dLon = deg2rad(lon2 - lon1);
  double rLat1 = deg2rad(lat1);
  double rLat2 = deg2rad(lat2);
  double h = sin(dLat / 2.0) * sin(dLat / 2.0) +
             cos(rLat1) * cos(rLat2) * sin(dLon / 2.0) * sin(dLon / 2.0);
  return static_cast<float>(2.0 * R * asin(sqrt(h)));
}

inline float bearingDeg(double lat1, double lon1, double lat2, double lon2) {
  double rLat1 = deg2rad(lat1);
  double rLat2 = deg2rad(lat2);
  double dLon = deg2rad(lon2 - lon1);
  double y = sin(dLon) * cos(rLat2);
  double x = cos(rLat1) * sin(rLat2) - sin(rLat1) * cos(rLat2) * cos(dLon);
  return normalize360(static_cast<float>(rad2deg(atan2(y, x))));
}

inline bool parseVruManufacturerPayload(const std::string& data, VruData& out) {
  // Legacy Android advertiser payload without GPS.
  // Accept both formats: [companyLow,companyHigh,'S','T',...] or ['S','T',...].
  if (data.size() < ST_VRU_PAYLOAD_LEN) return false;

  size_t offset = 0;
  if (data.size() >= ST_VRU_PAYLOAD_LEN + 2) {
    uint16_t company = static_cast<uint8_t>(data[0]) | (static_cast<uint8_t>(data[1]) << 8);
    if (company == ST_TEST_COMPANY_ID) offset = 2;
  }

  if (data.size() < offset + ST_VRU_PAYLOAD_LEN) return false;
  const uint8_t* p = reinterpret_cast<const uint8_t*>(data.data() + offset);
  if (p[0] != 'S' || p[1] != 'T') return false;
  if (p[2] != 1) return false;

  out.valid = true;
  out.hasGps = false;
  out.role = p[3];
  out.flags = p[4];
  out.seq = p[5];
  out.speedKmh = static_cast<float>(p[6]) / 2.0f;
  out.headingDeg = static_cast<uint16_t>(p[7]) * 2;
  out.accuracyM = p[8];
  out.battery = p[9];
  out.tempId = static_cast<uint16_t>(p[10]) | (static_cast<uint16_t>(p[11]) << 8);
  return true;
}

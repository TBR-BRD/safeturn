import Foundation
import CoreBluetooth
import CoreLocation
import AVFoundation

/// iPhone cyclist MVP.
/// Preferred for iOS background tests: iPhone scans for the truck ESP32, connects, and writes GPS updates.
/// This avoids depending on iOS background advertising being visible to an ESP32.
final class CyclistBackgroundService: NSObject, ObservableObject {
    private let gatewayServiceUUID = CBUUID(string: "B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A001")
    private let vruUpdateUUID = CBUUID(string: "B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A007")
    private let warningEventUUID = CBUUID(string: "B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A004")

    @Published var connectionText = "Startet ..."
    @Published var locationText = "GPS wartet"
    @Published var warningText = "-"
    @Published var sentPackets = 0

    private var central: CBCentralManager!
    private var gateway: CBPeripheral?
    private var vruUpdateChar: CBCharacteristic?
    private var warningEventChar: CBCharacteristic?
    private let locationManager = CLLocationManager()
    private var latestLocation: CLLocation?
    private var lastCourse: CLLocationDirection = 0
    private var sendTimer: Timer?
    private var reconnectTimer: Timer?
    private let speaker = AVSpeechSynthesizer()
    private var lastSpoken = Date.distantPast

    private let tempId: String = {
        let key = "SafeTurnCyclistTempId"
        if let existing = UserDefaults.standard.string(forKey: key) { return existing }
        let value = UUID().uuidString.prefix(8).description
        UserDefaults.standard.set(value, forKey: key)
        return value
    }()

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: .main, options: [
            CBCentralManagerOptionRestoreIdentifierKey: "de.safeturn.cyclist.central"
        ])
        locationManager.delegate = self
        locationManager.desiredAccuracy = kCLLocationAccuracyBestForNavigation
        locationManager.distanceFilter = 1
        locationManager.activityType = .fitness
        locationManager.pausesLocationUpdatesAutomatically = false
        if #available(iOS 9.0, *) {
            locationManager.allowsBackgroundLocationUpdates = true
        }
        sendTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.sendVruUpdate()
        }
        reconnectTimer = Timer.scheduledTimer(withTimeInterval: 6.0, repeats: true) { [weak self] _ in
            self?.ensureConnection()
        }
    }

    func start() {
        locationManager.requestWhenInUseAuthorization()
        locationManager.requestAlwaysAuthorization()
        locationManager.startUpdatingLocation()
        locationManager.startUpdatingHeading()
        ensureConnection()
    }

    private func ensureConnection() {
        guard central.state == .poweredOn else {
            connectionText = "Bluetooth nicht bereit: \(central.state.rawValue)"
            return
        }
        guard gateway == nil || gateway?.state == .disconnected else { return }
        connectionText = "Suche SafeTurn-LKW ..."
        central.scanForPeripherals(withServices: [gatewayServiceUUID], options: [CBCentralManagerScanOptionAllowDuplicatesKey: false])
    }

    private func sendVruUpdate() {
        guard let gateway = gateway, let char = vruUpdateChar else { return }
        guard let location = latestLocation else { return }
        guard abs(location.timestamp.timeIntervalSinceNow) < 5 else { return }

        let speedKmh = max(0.0, location.speed) * 3.6
        let heading = location.course >= 0 ? location.course : lastCourse
        let accuracy = max(0.0, location.horizontalAccuracy)
        let text = String(format: "C;%@;cyc;%.7f;%.7f;%.1f;%.0f;%.1f",
                          tempId,
                          location.coordinate.latitude,
                          location.coordinate.longitude,
                          speedKmh,
                          heading,
                          accuracy)
        guard let data = text.data(using: .utf8) else { return }
        let writeType: CBCharacteristicWriteType = char.properties.contains(.writeWithoutResponse) ? .withoutResponse : .withResponse
        gateway.writeValue(data, for: char, type: writeType)
        sentPackets += 1
    }

    private func speakIfNeeded(_ text: String) {
        guard Date().timeIntervalSince(lastSpoken) > 8 else { return }
        lastSpoken = Date()
        let utterance = AVSpeechUtterance(string: text)
        utterance.voice = AVSpeechSynthesisVoice(language: "de-DE")
        utterance.rate = 0.52
        speaker.speak(utterance)
    }
}

extension CyclistBackgroundService: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn { ensureConnection() }
        else { connectionText = "Bluetooth Status: \(central.state.rawValue)" }
    }

    func centralManager(_ central: CBCentralManager, willRestoreState dict: [String : Any]) {
        connectionText = "Bluetooth-State wiederhergestellt"
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        connectionText = "LKW-Gateway gefunden, verbinde ... RSSI \(RSSI)"
        gateway = peripheral
        central.stopScan()
        central.connect(peripheral, options: nil)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        connectionText = "Verbunden mit LKW-Gateway"
        peripheral.delegate = self
        peripheral.discoverServices([gatewayServiceUUID])
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        connectionText = "Getrennt, reconnect laeuft"
        gateway = nil
        vruUpdateChar = nil
        warningEventChar = nil
        ensureConnection()
    }
}

extension CyclistBackgroundService: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let error = error {
            connectionText = "Service-Fehler: \(error.localizedDescription)"
            return
        }
        peripheral.services?.forEach { service in
            peripheral.discoverCharacteristics([vruUpdateUUID, warningEventUUID], for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        if let error = error {
            connectionText = "Characteristic-Fehler: \(error.localizedDescription)"
            return
        }
        service.characteristics?.forEach { char in
            if char.uuid == vruUpdateUUID { vruUpdateChar = char }
            if char.uuid == warningEventUUID {
                warningEventChar = char
                peripheral.setNotifyValue(true, for: char)
            }
        }
        connectionText = "Bereit, sendet GPS an LKW"
        sendVruUpdate()
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil, characteristic.uuid == warningEventUUID,
              let data = characteristic.value,
              let text = String(data: data, encoding: .utf8) else { return }
        warningText = text
        if text.contains("W;3") {
            speakIfNeeded("Achtung LKW in der Naehe")
        }
    }
}

extension CyclistBackgroundService: CLLocationManagerDelegate {
    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        guard let location = locations.last else { return }
        latestLocation = location
        if location.course >= 0 { lastCourse = location.course }
        let kmh = max(0.0, location.speed) * 3.6
        locationText = String(format: "GPS %.1f km/h, Kurs %.0f°, Genauigkeit %.0f m", kmh, lastCourse, location.horizontalAccuracy)
    }

    func locationManager(_ manager: CLLocationManager, didUpdateHeading newHeading: CLHeading) {
        if newHeading.trueHeading >= 0 { lastCourse = newHeading.trueHeading }
    }
}

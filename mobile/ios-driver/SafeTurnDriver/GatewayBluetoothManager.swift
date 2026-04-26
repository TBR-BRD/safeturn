import Foundation
import CoreBluetooth
import CoreLocation
import AVFoundation
import SwiftUI

/// Background-oriented LKW app service.
/// No blinker UI. No driver interaction during the drive.
/// It only keeps the iPhone connected to the ESP32 gateway and sends GPS-based vehicle state.
final class GatewayBluetoothManager: NSObject, ObservableObject {
    private let serviceUUID = CBUUID(string: "B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A001")
    private let vehicleStateUUID = CBUUID(string: "B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A002")
    private let gatewayStatusUUID = CBUUID(string: "B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A003")
    private let warningEventUUID = CBUUID(string: "B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A004")

    @Published var connectionText = "Startet ..."
    @Published var locationText = "GPS wartet"
    @Published var gatewayStatus = "-"
    @Published var lastWarning = "-"
    @Published var sentPackets = 0

    private var central: CBCentralManager!
    private var gateway: CBPeripheral?
    private var vehicleStateChar: CBCharacteristic?
    private var gatewayStatusChar: CBCharacteristic?
    private var warningEventChar: CBCharacteristic?

    private let locationManager = CLLocationManager()
    private var latestLocation: CLLocation?
    private var lastCourse: CLLocationDirection = 0
    private var sendTimer: Timer?
    private var reconnectTimer: Timer?
    private let speaker = AVSpeechSynthesizer()
    private var lastSpoken = Date.distantPast

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: .main, options: [
            CBCentralManagerOptionRestoreIdentifierKey: "de.safeturn.driver.central"
        ])
        locationManager.delegate = self
        locationManager.desiredAccuracy = kCLLocationAccuracyBestForNavigation
        locationManager.distanceFilter = 1
        locationManager.activityType = .automotiveNavigation
        locationManager.pausesLocationUpdatesAutomatically = false
        if #available(iOS 9.0, *) {
            locationManager.allowsBackgroundLocationUpdates = true
        }

        sendTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.sendVehicleState()
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
        connectionText = "Suche ESP32-Gateway ..."
        central.scanForPeripherals(withServices: [serviceUUID], options: [CBCentralManagerScanOptionAllowDuplicatesKey: false])
    }

    private func sendVehicleState() {
        guard let gateway = gateway, let char = vehicleStateChar else { return }
        guard let location = latestLocation else { return }
        guard abs(location.timestamp.timeIntervalSinceNow) < 5 else { return }

        let speedKmh = max(0.0, location.speed) * 3.6
        let heading = location.course >= 0 ? location.course : lastCourse
        let accuracy = max(0.0, location.horizontalAccuracy)
        let text = String(format: "V;%.7f;%.7f;%.1f;%.0f;%.1f;truck",
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
        guard Date().timeIntervalSince(lastSpoken) > 6 else { return }
        lastSpoken = Date()
        let utterance = AVSpeechUtterance(string: text)
        utterance.voice = AVSpeechSynthesisVoice(language: "de-DE")
        utterance.rate = 0.52
        speaker.speak(utterance)
    }
}

extension GatewayBluetoothManager: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn { ensureConnection() }
        else { connectionText = "Bluetooth Status: \(central.state.rawValue)" }
    }

    func centralManager(_ central: CBCentralManager, willRestoreState dict: [String : Any]) {
        connectionText = "Bluetooth-State wiederhergestellt"
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        connectionText = "Gateway gefunden, verbinde ... RSSI \(RSSI)"
        gateway = peripheral
        central.stopScan()
        central.connect(peripheral, options: nil)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        connectionText = "Verbunden mit ESP32-Gateway"
        peripheral.delegate = self
        peripheral.discoverServices([serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        connectionText = "Getrennt, reconnect läuft"
        gateway = nil
        vehicleStateChar = nil
        gatewayStatusChar = nil
        warningEventChar = nil
        ensureConnection()
    }
}

extension GatewayBluetoothManager: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let error = error {
            connectionText = "Service-Fehler: \(error.localizedDescription)"
            return
        }
        peripheral.services?.forEach { service in
            peripheral.discoverCharacteristics([vehicleStateUUID, gatewayStatusUUID, warningEventUUID], for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        if let error = error {
            connectionText = "Characteristic-Fehler: \(error.localizedDescription)"
            return
        }
        service.characteristics?.forEach { char in
            switch char.uuid {
            case vehicleStateUUID:
                vehicleStateChar = char
            case gatewayStatusUUID:
                gatewayStatusChar = char
                peripheral.setNotifyValue(true, for: char)
                peripheral.readValue(for: char)
            case warningEventUUID:
                warningEventChar = char
                peripheral.setNotifyValue(true, for: char)
                peripheral.readValue(for: char)
            default:
                break
            }
        }
        connectionText = "Gateway bereit, App kann in den Hintergrund"
        sendVehicleState()
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil, let data = characteristic.value, let text = String(data: data, encoding: .utf8) else { return }
        if characteristic.uuid == gatewayStatusUUID {
            gatewayStatus = text
        } else if characteristic.uuid == warningEventUUID {
            lastWarning = text
            if text.contains("W;3") || text.contains("W;2") {
                let message = text.split(separator: ";").last.map(String.init) ?? "Achtung"
                speakIfNeeded(message)
            }
        }
    }
}

extension GatewayBluetoothManager: CLLocationManagerDelegate {
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

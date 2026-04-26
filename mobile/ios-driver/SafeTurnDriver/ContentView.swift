import SwiftUI

/// Minimal status screen. The MVP driver app is intentionally "headless":
/// no blinker button, no operational GUI required while driving.
struct ContentView: View {
    @EnvironmentObject var bluetooth: GatewayBluetoothManager

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("SafeTurn LKW Background")
                .font(.title.bold())
            Text("Diese App sendet nur GPS/Fahrzeugdaten an das ESP32-Gateway. Warnung erfolgt lokal ueber den Summer am Gateway.")
                .foregroundColor(.secondary)
            Divider()
            Text("BLE: \(bluetooth.connectionText)")
            Text("GPS: \(bluetooth.locationText)")
            Text("Pakete: \(bluetooth.sentPackets)")
            Text("Gateway: \(bluetooth.gatewayStatus)")
            Text("Warnung: \(bluetooth.lastWarning)")
                .font(.footnote)
                .foregroundColor(.secondary)
            Spacer()
            Text("Nach dem Start und den Berechtigungen kann die App in den Hintergrund. Fuer Produktivbetrieb sind Hardwaretests erforderlich.")
                .font(.footnote)
                .foregroundColor(.secondary)
        }
        .padding()
        .onAppear { bluetooth.start() }
    }
}

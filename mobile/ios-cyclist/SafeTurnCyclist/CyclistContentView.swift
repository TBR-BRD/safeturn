import SwiftUI

/// Minimal screen. The app acts as a background BLE client, not as a classic beacon.
/// It finds a SafeTurn ESP32 gateway and writes GPS updates to it.
struct CyclistContentView: View {
    @EnvironmentObject var service: CyclistBackgroundService

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("SafeTurn Fahrrad")
                .font(.title.bold())
            Text("iPhone-Modus fuer Hintergrundtests: scannt nach ESP32-Gateway und sendet GPS-Daten lokal per BLE GATT.")
                .foregroundColor(.secondary)
            Divider()
            Text("BLE: \(service.connectionText)")
            Text("GPS: \(service.locationText)")
            Text("Pakete: \(service.sentPackets)")
            Text("Warnung: \(service.warningText)")
                .foregroundColor(.orange)
            Spacer()
            Text("Hinweis: iPhone als echtes BLE-Beacon im Hintergrund ist fuer ESP32-Scanner nicht zuverlaessig. Diese App nutzt daher GATT-Connect statt Beacon-only.")
                .font(.footnote)
                .foregroundColor(.secondary)
        }
        .padding()
        .onAppear { service.start() }
    }
}

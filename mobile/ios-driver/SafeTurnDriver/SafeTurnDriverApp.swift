import SwiftUI

@main
struct SafeTurnDriverApp: App {
    @StateObject private var bluetooth = GatewayBluetoothManager()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(bluetooth)
        }
    }
}

import SwiftUI

@main
struct SafeTurnCyclistApp: App {
    @StateObject private var service = CyclistBackgroundService()

    var body: some Scene {
        WindowGroup {
            CyclistContentView()
                .environmentObject(service)
        }
    }
}

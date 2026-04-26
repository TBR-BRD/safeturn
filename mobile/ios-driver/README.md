# iOS Fahrer-App

Minimal-App fuer das iPhone im LKW.

## Aufgabe

- ESP32-Gateway `SafeTurn-LKW` per BLE finden.
- Per BLE GATT verbinden.
- GPS, Geschwindigkeit, Fahrtrichtung und Fahrzeugtyp an das ESP32-Gateway senden.
- Status und Warnungen empfangen.
- Warnung optional per Sprachausgabe melden.
- Nach dem Start im Hintergrund weiterlaufen.

## Projekt erzeugen mit XcodeGen

```bash
cd mobile/ios-driver
xcodegen generate
open SafeTurnDriver.xcodeproj
```

Danach in Xcode:

1. Team fuer Signing setzen.
2. Bundle Identifier bei Bedarf anpassen.
3. Auf echtem iPhone starten.
4. Standort und Bluetooth erlauben.

## Ohne XcodeGen

Alternativ in Xcode ein neues iOS-Projekt mit SwiftUI anlegen und die Dateien aus `SafeTurnDriver/` uebernehmen.

## Wichtige Capabilities

Die `Info.plist` enthaelt:

```text
NSBluetoothAlwaysUsageDescription
NSLocationWhenInUseUsageDescription
NSLocationAlwaysAndWhenInUseUsageDescription
UIBackgroundModes: bluetooth-central, location
```

## Datenformat

```text
V;lat;lon;speedKmh;headingDeg;accuracyM;truck
```

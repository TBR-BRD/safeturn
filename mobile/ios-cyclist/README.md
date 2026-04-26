# iOS Fahrradfahrer-App

Minimal-App fuer das iPhone am Fahrrad.

## Aufgabe

- ESP32-Gateway `SafeTurn-LKW` per BLE finden.
- Per BLE GATT verbinden.
- GPS, Geschwindigkeit und Fahrtrichtung an das ESP32-Gateway senden.
- Warnungen vom ESP32 empfangen.
- Nach dem Start im Hintergrund weiterlaufen.

## Warum kein reines iPhone-Beacon?

Die App nutzt bewusst keinen reinen Beacon-only-Modus. Stattdessen arbeitet sie als BLE Central/GATT Client. Das ist fuer diesen ESP32-MVP realistischer, weil das iPhone aktiv eine Verbindung zum ESP32 herstellt und GPS-Daten schreibt.

## Projekt erzeugen mit XcodeGen

```bash
cd mobile/ios-cyclist
xcodegen generate
open SafeTurnCyclist.xcodeproj
```

Danach in Xcode:

1. Team fuer Signing setzen.
2. Bundle Identifier bei Bedarf anpassen.
3. Auf echtem iPhone starten.
4. Standort und Bluetooth erlauben.

## Ohne XcodeGen

Alternativ in Xcode ein neues iOS-Projekt mit SwiftUI anlegen und die Dateien aus `SafeTurnCyclist/` uebernehmen.

## Datenformat

```text
C;tempId;cyc;lat;lon;speedKmh;headingDeg;accuracyM
```

# Mobile Apps

Dieser Ordner enthaelt die Smartphone-Komponenten des SafeTurn-MVP.

## Haupt-Apps

```text
ios-driver/   iPhone im LKW
ios-cyclist/  iPhone am Fahrrad
```

Beide Apps nutzen:

- SwiftUI fuer minimale Statusanzeige,
- CoreBluetooth fuer BLE GATT,
- CoreLocation fuer GPS,
- Background Modes fuer Tests im Hintergrund.

## Optionale App

```text
android-cyclist-optional/
```

Die Android-App ist optional und kann fuer spaetere Tests als BLE-Advertiser genutzt werden. Die aktuelle Hauptarchitektur nutzt zwei iPhones per BLE GATT.

## Wichtiger Hinweis

Bluetooth und GPS-Hintergrundtests muessen auf echten Geraeten stattfinden. Simulatoren sind hier nicht ausreichend.

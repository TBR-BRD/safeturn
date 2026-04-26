# 02 Technische Beschreibung

## Architektur

SafeTurn MVP besteht aus drei aktiven Komponenten:

1. **ESP32-Gateway im LKW**
   - BLE GATT Server
   - empfaengt GPS-Daten des LKW-Smartphones
   - empfaengt GPS-Daten des Fahrradfahrer-Smartphones
   - berechnet Risiko lokal
   - steuert Summer und LED

2. **iPhone Fahrer-App**
   - nutzt Core Location fuer GPS, Geschwindigkeit und Fahrtrichtung
   - verbindet sich per Core Bluetooth mit dem ESP32
   - schreibt zyklisch den Fahrzeugstatus in eine BLE Characteristic
   - zeigt optional Statusmeldungen an
   - kann nach dem Start im Hintergrund laufen

3. **iPhone Fahrradfahrer-App**
   - nutzt Core Location fuer GPS, Geschwindigkeit und Fahrtrichtung
   - verbindet sich per Core Bluetooth mit dem ESP32
   - schreibt zyklisch den VRU-Status in eine BLE Characteristic
   - kann nach dem Start im Hintergrund laufen

## Datenfluss

```text
LKW-iPhone -> ESP32:
V;lat;lon;speedKmh;headingDeg;accuracyM;truck

Fahrrad-iPhone -> ESP32:
C;tempId;cyc;lat;lon;speedKmh;headingDeg;accuracyM

ESP32 -> Apps:
S;clients=...;vru=...;veh=...;mode=gps-no-blinker
W;level;role;tempId;rssi;distanceM;relativeBearingDeg;message
D;tempId;role;rssi;speedKmh;headingDeg;distanceM
```

## Betriebsmodus

Der MVP nutzt keinen klassischen Beacon-Modus des iPhones. Stattdessen verbindet sich das iPhone per BLE GATT aktiv mit dem ESP32 und sendet Daten. Das ist fuer iOS robuster als ein iPhone als dauerhaftes Hintergrund-Beacon zu verwenden.

## Verbindung

Das ESP32-Gateway bewirbt folgenden BLE-Service:

```text
Gateway Service UUID:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A001
```

Die Apps scannen nach diesem Service und verbinden sich direkt mit dem ESP32.

## Hintergrundbetrieb

Die iOS-Apps enthalten die notwendigen Info.plist-Eintraege fuer:

```text
UIBackgroundModes:
- bluetooth-central
- location
```

Fuer echte Tests muss die App auf einem realen iPhone mit korrektem Signing ausgefuehrt werden. Der Simulator ist fuer Bluetooth/GPS-Hintergrundtests nicht ausreichend.

## Lokale Warnung

Der ESP32 warnt unabhaengig von einer App-Anzeige:

- Level 1: kurzer Hinweis
- Level 2: mittlere Warnung
- Level 3: hohe Warnung

Dadurch kann der Fahrer auch gewarnt werden, wenn die Fahrer-App keine aktive GUI zeigt.

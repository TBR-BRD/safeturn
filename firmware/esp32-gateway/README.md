# ESP32 Gateway v0.3

Firmware fuer das SafeTurn-LKW-Gateway.

## Funktionen

- BLE GATT Server `SafeTurn-LKW`.
- Empfaengt Fahrzeug-GPS vom iPhone im LKW.
- Empfaengt Fahrrad-GPS vom iPhone am Fahrrad.
- Berechnet Risiko lokal nur ueber GPS-Geometrie.
- Warnt lokal ueber aktiven Summer an GPIO 25.
- Zeigt Betriebsstatus ueber gruene LED an GPIO 26.
- Zeigt Warnung ueber rote LED an GPIO 27.
- Keine Cloud, kein Mobilfunk, kein Blinker, kein GPS-Modul am ESP32.

## Hardware

```text
ESP32 DevKit
USB 5 V vom Zigarettenanzuender-Adapter
GPIO 25 -> aktiver Buzzer Signal
GPIO 26 -> Status LED gruen ueber 330 Ohm
GPIO 27 -> Warn LED rot ueber 330 Ohm
GND     -> gemeinsame Masse
```

Fuer einen lauteren Summer den Summer nicht direkt am GPIO betreiben, sondern ueber Transistor/MOSFET und geeignete Versorgung.

## Build mit PlatformIO

```bash
cd firmware/esp32-gateway
pio run
pio run -t upload
pio device monitor
```

## Konfiguration

Die wichtigsten Pins stehen am Anfang von `src/main.cpp`:

```cpp
static constexpr int BUZZER_PIN = 25;
static constexpr int STATUS_LED_PIN = 26;
static constexpr int WARN_LED_PIN = 27;
```

## BLE-Protokoll

Siehe `../../protocol/PROTOCOL.md` und `../../docs/04_BLE_PROTOKOLL.md`.

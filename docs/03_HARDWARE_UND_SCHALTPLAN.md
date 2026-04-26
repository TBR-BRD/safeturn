# 03 Hardware und Schaltplan

## Hardware fuer MVP

| Bauteil | Zweck |
|---|---|
| ESP32 DevKit V1 / ESP-WROOM-32 | BLE Gateway und lokale Risikoentscheidung |
| USB-Zigarettenanzuenderadapter | Versorgung im LKW |
| Aktiver Summer 3.3 V oder 5 V | akustische Warnung |
| Gruene LED + 330 Ohm | Betriebsanzeige |
| Rote LED + 330 Ohm | Warnanzeige |
| Optionaler Taster | Reset/Testeingang |
| Kunststoffgehaeuse | mechanischer Schutz |

## Kein GPS-Modul am ESP32

Der ESP32 hat im aktuellen MVP kein GPS-Modul. GPS-Daten kommen von:

- iPhone im LKW,
- iPhone am Fahrrad.

Dadurch entfallen GPS-Antenne, UART-Verkabelung und GPS-Spannungsversorgung.

## Standard-Pinbelegung

| Funktion | Pin | Hinweis |
|---|---:|---|
| 5 V Versorgung | VIN / 5V | vom USB-Adapter |
| Masse | GND | gemeinsame Masse |
| Summer | GPIO 25 | aktiver Summer, nicht direkt grosser Lautsprecher |
| Status-LED | GPIO 26 | ueber 330 Ohm nach GND |
| Warn-LED | GPIO 27 | ueber 330 Ohm nach GND |
| Taster optional | GPIO 34 | gegen GND, interner Pull-up je nach Board pruefen |

## Schaltplan

![ESP32 Gateway Schaltplan](images/schematic-esp32-no-gps.png)

## Hinweise zum Einbau

- Der ESP32 sollte nicht lose im Fahrerhaus liegen.
- Das Gehaeuse sollte gegen Kurzschluss geschuetzt sein.
- Der Summer muss akustisch wahrnehmbar, aber nicht erschreckend laut sein.
- Kabel muessen so verlegt werden, dass sie den Fahrer nicht behindern.
- Fuer einen dauerhaften Fahrzeugeinbau spaeter eine abgesicherte 12/24-V-auf-5-V-Versorgung verwenden.

## Wichtiger Hinweis

Der MVP-Aufbau ist fuer Tischtest, Labor und Betriebshof gedacht. Fuer einen festen Fahrzeugeinbau sind EMV, Absicherung, mechanische Befestigung, Brandschutz und Fahrzeugzulassung gesondert zu pruefen.

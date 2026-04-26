# 06 Testplan

## Ziel

Der Testplan prueft, ob die lokale Kette funktioniert:

```text
LKW-iPhone + Fahrrad-iPhone -> BLE -> ESP32 -> Risiko -> Summer/LED
```

## Test 1: ESP32 Hardwaretest

1. ESP32 flashen.
2. Serial Monitor oeffnen.
3. Startup-Beep pruefen.
4. Statusmeldungen pruefen.
5. Rote und gruene LED pruefen.

Erwartung:

```text
ESP32 startet als SafeTurn-LKW
GATT Server aktiv
Scanner aktiv
Summer gibt kurzen Startton aus
```

## Test 2: LKW-iPhone verbindet sich

1. SafeTurnDriver auf echtem iPhone starten.
2. Bluetooth- und Standortberechtigungen erteilen.
3. ESP32 einschalten.
4. Verbindung beobachten.

Erwartung:

```text
App findet SafeTurn-LKW
App verbindet sich
ESP32 zeigt clients=1
vehicle state wird gesendet
```

## Test 3: Fahrrad-iPhone verbindet sich

1. SafeTurnCyclist auf zweitem iPhone starten.
2. Berechtigungen erteilen.
3. Verbindung beobachten.

Erwartung:

```text
App findet SafeTurn-LKW
App verbindet sich
ESP32 zeigt clients=2
VRU updates kommen an
```

## Test 4: GPS-Datenfluss

Beide iPhones nach draussen bringen oder auf Testgelaende nutzen.

Erwartung:

```text
Fahrzeugdaten: V;...
Fahrraddaten: C;...
ESP32 Status: veh=1, vru=1
```

## Test 5: Warnlogik auf Testgelaende

Nur auf abgesperrtem Gelaende.

Szenarien:

1. Fahrrad rechts neben stehendem LKW.
2. Fahrrad rechts hinter LKW.
3. Fahrrad links vom LKW.
4. Fahrrad weit entfernt.
5. GPS schlecht / unter Dach.

Erwartung:

- rechts nah: Warnung,
- links: keine oder niedrigere Warnung,
- weit entfernt: keine Warnung,
- schlechtes GPS: nur Hinweis oder keine Warnung.

## Test 6: Hintergrundbetrieb

1. Beide Apps starten.
2. Verbindung herstellen.
3. Apps in den Hintergrund legen.
4. 10 Minuten laufen lassen.
5. ESP32 Serial Monitor pruefen.

Erwartung:

```text
Daten laufen weiter
ESP32 bekommt aktuelle Updates
bei Risiko Summer aktiv
```

## Messprotokoll

Bei jedem Test notieren:

| Test | GPS LKW m | GPS Fahrrad m | Abstand real m | Warnlevel | Bemerkung |
|---|---:|---:|---:|---:|---|
| Beispiel | 5 | 6 | 12 | 2 | rechte Seite korrekt |

## Abbruchkriterien

Test abbrechen, wenn:

- Fahrer abgelenkt wird,
- Verkehrsteilnehmer gefaehrdet werden,
- Verbindung instabil wird,
- GPS-Genauigkeit dauerhaft schlecht ist,
- Warnungen deutlich falsch sind.

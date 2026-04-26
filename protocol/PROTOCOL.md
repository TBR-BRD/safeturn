# SafeTurn BLE Protocol v0.3

Lokale BLE-Loesung ohne Cloud und ohne Mobilfunk.

## UUIDs

```text
Gateway Service:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A001

Vehicle State, iPhone LKW -> ESP32:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A002

Gateway Status, ESP32 -> Apps:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A003

Warning Event, ESP32 -> Apps:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A004

Detected VRU, ESP32 -> Apps:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A005

Settings:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A006

VRU GPS Update, Fahrrad-iPhone -> ESP32:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A007

Legacy Android/SafetyTag Advertiser Service:
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8B001
```

## LKW-iPhone -> ESP32

```text
V;lat;lon;speedKmh;headingDeg;accuracyM;truck
```

Beispiel:

```text
V;52.5201000;13.4050000;18.2;91;4.8;truck
```

## Fahrradfahrer-iPhone -> ESP32

```text
C;tempId;role;lat;lon;speedKmh;headingDeg;accuracyM
```

Beispiel:

```text
C;A7F3C901;cyc;52.5200800;13.4051700;17.1;89;5.2
```

Rollen:

```text
cyc = Fahrradfahrer
ped = Fussgaenger
sco = Scooter
```

## ESP32 -> Apps: Warnung

```text
W;risk;role;tempId;rssi;distanceM;relativeBearingDeg;message
```

Beispiel:

```text
W;3;cyc;A7F3;-40;9.8;72;Achtung, Radfahrer rechts nahe am LKW
```

Risk:

```text
0 = keine Warnung
1 = Hinweis
2 = Warnung
3 = hohes Risiko / Summer mit starker Sequenz
```

## ESP32 -> Apps: Status

```text
S;clients=2;vru=1;veh=1;speed=18;gpsAcc=5;mode=gps-no-blinker
```

## Risiko-Logik v0.3

Eingaben:

- LKW-GPS
- LKW-Geschwindigkeit
- LKW-Fahrtrichtung
- LKW-GPS-Genauigkeit
- Fahrrad-GPS
- Fahrrad-Geschwindigkeit
- Fahrrad-Fahrtrichtung
- Fahrrad-GPS-Genauigkeit

Keine Eingaben:

- kein Blinker
- kein CAN
- kein OBD
- keine Cloud
- kein Mobilfunk
- keine Aussen-ESP32

Die Rechtslage wird ueber relative Geometrie berechnet:

```text
relativeBearing = bearing(LKW -> Fahrrad) - heading(LKW)
rechts = relativeBearing zwischen 10 und 170 Grad
```

Die Logik ist fuer einen MVP bewusst konservativ und muss mit echten Messfahrten kalibriert werden.

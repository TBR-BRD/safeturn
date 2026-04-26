# 04 BLE-Protokoll

Dieses Dokument beschreibt das lokale SafeTurn-BLE-Protokoll zwischen Smartphone-Apps und ESP32-Gateway.

## BLE-Service

```text
SafeTurn Gateway Service
B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A001
```

## Characteristics

| Name | UUID | Richtung | Eigenschaften |
|---|---|---|---|
| Vehicle State | B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A002 | LKW-iPhone -> ESP32 | Write / WriteWithoutResponse |
| Gateway Status | B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A003 | ESP32 -> Apps | Read / Notify |
| Warning Event | B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A004 | ESP32 -> Apps | Read / Notify |
| Detected VRU | B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A005 | ESP32 -> Apps | Read / Notify |
| Settings | B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A006 | Apps <-> ESP32 | Read / Write |
| VRU Update | B5C45F00-9B7A-4E0D-85D4-9D6F5DB8A007 | Fahrrad-iPhone -> ESP32 | Write / WriteWithoutResponse |

## Fahrzeugstatus

Format:

```text
V;lat;lon;speedKmh;headingDeg;accuracyM;vehicleType
```

Beispiel:

```text
V;52.5201000;13.4050000;12.4;91;4.8;truck
```

## Fahrradfahrerstatus

Format:

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
sco = E-Scooter
```

## Warnereignis

Format:

```text
W;level;role;tempId;rssi;distanceM;relativeBearingDeg;message
```

Beispiel:

```text
W;3;cyc;A7F3;-40;9.8;72;Achtung, Radfahrer rechts nahe am LKW
```

Warnstufen:

| Level | Bedeutung |
|---:|---|
| 0 | keine Warnung |
| 1 | Hinweis |
| 2 | Warnung |
| 3 | hohe Warnung |

## Gateway-Status

Beispiel:

```text
S;clients=2;vru=1;veh=1;speed=12;gpsAcc=5;mode=gps-no-blinker
```

## Datenschutz

- `tempId` ist eine temporaere ID.
- Keine Namen, Telefonnummern oder Nutzerkonten.
- Keine Cloud-Speicherung.
- ESP32 haelt Daten nur wenige Sekunden im RAM.

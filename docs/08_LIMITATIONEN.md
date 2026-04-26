# 08 Limitierungen

## GPS-Genauigkeit

Smartphone-GPS ist nicht zentimetergenau. Gerade in Staedten kann es durch Haeuser, Baeume, Bruecken und Mehrwegeeffekte mehrere Meter daneben liegen.

Folge:

```text
Ein Fahrradfahrer rechts neben dem LKW kann geometrisch falsch eingeordnet werden.
```

## Keine Blinkerdaten

Die erste Version weiss nicht, ob der LKW wirklich rechts abbiegen will. Daher warnt sie allgemeiner vor einer Person rechts neben dem LKW.

## Keine rechten Aussensensoren

Mit nur einem Gateway im Fahrerhaus gibt es keine robuste Funk-Seitenbestimmung. Die aktuelle Version basiert hauptsaechlich auf GPS-Geometrie.

## Hintergrundbetrieb iOS

iOS kann Hintergrundaktivitaet begrenzen. Fuer stabile Tests muessen die Apps korrekt signiert, mit Berechtigungen versehen und auf echten Geraeten getestet werden.

## BLE-Verbindungen

Ein ESP32 kann mehrere BLE-Verbindungen verwalten, aber fuer den MVP sollte zuerst mit zwei iPhones getestet werden:

```text
1 LKW-iPhone
1 Fahrrad-iPhone
```

Spaeter koennen mehrere Fahrradfahrer ergaenzt werden.

## Kein Produktivsystem

Der MVP ist keine zugelassene Fahrzeugtechnik. Er ist nur fuer Tests in sicherer Umgebung gedacht.

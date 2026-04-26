# 05 Risikoalgorithmus

Der ESP32 berechnet das Risiko lokal aus GPS-Daten.

## Eingangsdaten

Vom LKW-iPhone:

- GPS-Breitengrad,
- GPS-Laengengrad,
- Geschwindigkeit,
- Fahrtrichtung,
- GPS-Genauigkeit,
- Fahrzeugtyp.

Vom Fahrrad-iPhone:

- temporaere ID,
- Rolle,
- GPS-Breitengrad,
- GPS-Laengengrad,
- Geschwindigkeit,
- Fahrtrichtung,
- GPS-Genauigkeit.

## Berechnete Werte

Der ESP32 berechnet:

1. Abstand zwischen LKW und Fahrrad.
2. Peilwinkel vom LKW zum Fahrrad.
3. Relative Position zum LKW-Heading.
4. Ob das Fahrrad rechts vom LKW liegt.
5. Ob beide grob in gleicher Richtung fahren.
6. Ob die GPS-Genauigkeit ausreichend ist.

## Rechts-Erkennung

```text
relativeBearing = bearing(LKW -> Fahrrad) - heading(LKW)
```

Im MVP gilt:

```text
10 Grad < relativeBearing < 170 Grad => rechts vom LKW
```

Das ist eine geometrische Naeherung und muss im Feldtest kalibriert werden.

## Score-Modell

Vereinfachter MVP-Score:

```text
Abstand < 10 m     +55 Punkte
Abstand < 20 m     +42 Punkte
Abstand < 35 m     +24 Punkte
Abstand < 60 m     + 8 Punkte
Fahrrad rechts     +28 Punkte
Gleiche Richtung   +14 Punkte
LKW sehr langsam   +12 Punkte
LKW langsam        + 6 Punkte
Fahrrad bewegt     + 6 Punkte
GPS schlecht       -25 bis -40 Punkte
```

## Warnstufen

```text
Score >= 78 => Level 3 / hohe Warnung
Score >= 58 => Level 2 / Warnung
Score >= 38 => Level 1 / Hinweis
sonst       => keine Warnung
```

## Ohne Blinkerdaten

Da die erste Version keine Blinkerkopplung nutzt, darf der Text nicht behaupten, dass der LKW abbiegt. Korrekte Warntexte sind:

```text
Radfahrer in der Naehe
Radfahrer rechts / im Nahbereich
Achtung, Radfahrer rechts nahe am LKW
```

Nicht korrekt im MVP:

```text
LKW biegt rechts ab
```

## Grenzen

GPS kann in urbanen Gebieten mehrere Meter abweichen. Deshalb gilt:

```text
accuracyM > 25 m => Score-Abzug
accuracyM > 50 m => starker Score-Abzug
```

Die erste Version ist nur fuer kontrollierte Tests gedacht.

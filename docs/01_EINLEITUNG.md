# 01 Einleitung

SafeTurn ist ein Konzept und technischer Prototyp fuer eine lokale Warnung vor gefaehrdeten Verkehrsteilnehmern rechts neben einem LKW. Der erste MVP ist bewusst einfach gehalten:

- ein ESP32-Gateway im LKW,
- ein Smartphone im LKW,
- ein Smartphone beim Fahrradfahrer,
- Kommunikation nur lokal per Bluetooth Low Energy,
- keine Cloud,
- kein Mobilfunk,
- kein Webserver,
- keine Blinkerkopplung,
- GPS-Daten kommen von den Smartphones,
- Warnung im LKW ueber Summer und Warn-LED.

## Motivation

Beim Rechtsabbiegen und Anfahren koennen Fahrradfahrer und Fussgaenger im toten Winkel oder im seitlichen Gefahrenbereich des LKW sein. SafeTurn soll fruehzeitig einen Hinweis geben, wenn sich ein Fahrradfahrer rechts neben dem Fahrzeug befindet oder eine riskante Konstellation entsteht.

## MVP-Ziel

Der MVP soll nachweisen:

```text
Kann ein ESP32 im LKW GPS-Daten von zwei Smartphones lokal empfangen,
daraus Abstand und relative Lage berechnen und den Fahrer akustisch warnen?
```

## Nicht-Ziele der ersten Version

Die erste Version will nicht sofort ein serienreifes Sicherheitssystem sein. Bewusst nicht enthalten:

- keine Zulassung fuer den Strassenverkehr,
- keine direkte Fahrzeugbus-/CAN-Anbindung,
- keine Blinkerinformation,
- keine Karten- oder Ampelerkennung,
- kein Cloud-Dashboard,
- keine dauerhafte Speicherung von Fahrdaten,
- keine UWB- oder externen Fahrzeugscanner.

## Zielbild

```text
Fahrrad-iPhone  ---> BLE GATT ---> ESP32 Gateway <--- BLE GATT <--- LKW-iPhone
                                      |
                                      v
                                 Buzzer/LED
```

Der ESP32 ist damit die lokale Logik- und Warneinheit im Fahrzeug.

# 07 Datenschutz und Sicherheit

## Grundprinzipien

Der MVP ist lokal aufgebaut:

- keine Cloud,
- kein Mobilfunk,
- keine Datenbank,
- keine Nutzerkonten,
- keine Namen,
- keine Telefonnummern,
- keine dauerhafte Speicherung von Standortdaten.

## Temporäre IDs

Das Fahrrad-iPhone sendet eine temporaere ID. In der MVP-App wird diese lokal erzeugt. Fuer spaetere Versionen sollte die ID regelmaessig rotieren, zum Beispiel alle 5 bis 15 Minuten.

## Daten im ESP32

Der ESP32 haelt Daten nur im RAM. VRU-Daten werden nach wenigen Sekunden ohne Update entfernt.

Aktuell:

```text
VRU_TTL_MS = 4500 ms
VEHICLE_TTL_MS = 3000 ms
```

## Funkbereich

BLE ist lokal und kurzreichweitig. Trotzdem koennen BLE-Signale in der Naehe empfangen werden. Deshalb sollten keine personenbezogenen Daten im BLE-Protokoll enthalten sein.

## Missbrauchsschutz fuer spaetere Versionen

Der MVP enthaelt noch keine produktionsreife Absicherung. Spaeter noetig:

- rotierende IDs,
- Pairing/Autorisierung zwischen LKW-App und Gateway,
- Signatur oder MAC fuer Datenpakete,
- Schutz gegen gefaelschte Positionen,
- Rate-Limiting,
- robuste Fehlerbehandlung,
- sichere Firmware-Updates.

## Sicherheitswarnung

SafeTurn darf nicht so gestaltet werden, dass Fahrer sich auf das System verlassen. Die Warnung ist nur ein zusaetzlicher Hinweis. Spiegel, Schulterblick, Verkehrsregeln und zugelassene Assistenzsysteme bleiben massgeblich.

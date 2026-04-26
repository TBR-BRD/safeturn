# Dateien in GitHub hochladen

Das Repository ist unter folgendem Namen vorgesehen:

```text
https://github.com/TBR-BRD/safeturn
```

## Variante A: per Git-Kommandozeile

```bash
git clone https://github.com/TBR-BRD/safeturn.git
cd safeturn

# Inhalt dieses Pakets in den Repo-Ordner kopieren
# Danach:

git add .
git commit -m "Initial SafeTurn MVP: ESP32 gateway and iOS BLE apps"
git push origin main
```

Wenn der Branch noch `master` heisst:

```bash
git branch -M main
git push -u origin main
```

## Variante B: per GitHub Web-Oberflaeche

1. ZIP entpacken.
2. GitHub Repository oeffnen.
3. `Add file` -> `Upload files`.
4. Alle Ordner und Dateien hochladen.
5. Commit-Nachricht setzen.

Fuer groessere Projekte ist Variante A stabiler.

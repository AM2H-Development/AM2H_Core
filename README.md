# AM2H_Core
AM2H_Core Library for Arduino

A library to connect various sensors and devices to a home server via MQTT. It supports ESP8266 boards, especially the AM2H wlan dots.

Dokumentation (work in progress) can be found here: https://am2h-core.readthedocs.io/en/latest/

---

## Build & Firmware Download

[![PlatformIO Build](https://github.com/AM2H-Development/AM2H_Core/actions/workflows/build.yml/badge.svg)](https://github.com/AM2H-Development/AM2H_Core/actions/workflows/build.yml)

Die Firmware wird bei jedem Push automatisch per GitHub Actions gebaut (PlatformIO, Board: ESP-01 1M).

### Firmware herunterladen

**Option A – Aktueller Build (90 Tage gespeichert):**
1. [Actions-Tab](https://github.com/AM2H-Development/AM2H_Core/actions/workflows/build.yml) öffnen
2. Letzten erfolgreichen Run anklicken
3. Unter **Artifacts** → `firmware-esp01-vX.Y.Z.zip` herunterladen
4. ZIP entpacken → `firmware.bin`

**Option B – Stabiler Release (empfohlen, kein Ablaufdatum):**
1. [Releases](https://github.com/AM2H-Development/AM2H_Core/releases) öffnen
2. `firmware.bin` beim gewünschten Release herunterladen

### Neuen Release erstellen

Die Versionsnummer wird aus `src/AM2H_Core.h` (`#define VERSION`) gelesen.
Ein Release entsteht vollautomatisch sobald ein Tag mit `v`-Prefix gepusht wird:

```bash
# Versionsnummer in src/AM2H_Core.h anpassen, committen & pushen, dann:
git tag v1.9.0
git push origin v1.9.0
```

GitHub Actions baut daraufhin die Firmware und legt sie als Release-Asset ab —
inklusive Flash-Anleitung im Release-Text.

---

## Firmware auf den ESP-01 flashen (ohne Software-Installation)

Voraussetzung: Chrome oder Edge Browser (kein Firefox), USB-Serial-Adapter

### Schritt-für-Schritt

1. **ESP-01 in den Flash-Modus versetzen**
   - `GPIO0` mit `GND` verbinden
   - ESP-01 per USB-Serial-Adapter anschließen (3,3V, GND, TX→RX, RX→TX)
   - Reset-Taste drücken (oder kurz VCC trennen) → ESP startet im Flash-Modus

2. **Web-Flasher öffnen**
   - [esp.huhn.me](https://esp.huhn.me) im Chrome oder Edge aufrufen

3. **Firmware hochladen**
   - **CONNECT** → Serial-Port des USB-Adapters auswählen
   - Zeile 1: Adresse `0x0`, Datei `firmware.bin` auswählen
   - **PROGRAM** klicken und warten

4. **ESP neu starten**
   - `GPIO0` von `GND` trennen
   - Reset → ESP startet mit neuer Firmware

> **Hinweis:** Funktioniert nur auf Desktop-Systemen (Windows, Mac, Linux).
> Android/iOS unterstützen die Web Serial API nicht.

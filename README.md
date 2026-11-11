# ESP32-CAM + AI · Beispiele

PlatformIO-Projekte und Arduino-Sketche für den **AI-Thinker ESP32-CAM** (und XIAO ESP32S3 Sense), die ein Foto aufnehmen und es per WLAN an ein KI-Modell schicken. Die Antwort kann eine LED, einen Servo, ein Display oder beliebige andere Aktoren steuern.

Entwickelt für einen Design-Kurs als Einstieg in physische KI-Interfaces.

---

## Übersicht

```
Welt → [Kamera] → ESP32-CAM → [WLAN] → KI-API → Antwort → LED / Servo / Display
```

Das Board läuft autonom — Powerbank reicht, kein Laptop nötig. Jede OpenAI-kompatible API funktioniert (OpenAI, Ollama, LM Studio, eigener Server).

---

## Voraussetzungen

- **Hardware:** ESP32-CAM (AI-Thinker), Arduino Uno (als USB-Adapter für ersten Flash)
- **Software:** [VS Code](https://code.visualstudio.com) + [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) Extension
- **API:** Zugangsdaten zu einer OpenAI-kompatiblen API

---

## Projekte (PlatformIO · ESP32-CAM AI-Thinker)

| Ordner | Was | Schwierigkeit |
|--------|-----|--------------|
| `examples/00_ota_flash/` | Starter: AP + OTA aktivieren — **hier anfangen!** | ● |
| `examples/01_blink/` | LED-Test — Board läuft? | ● |
| `examples/02_hello_ai/` | Textfrage an KI → Serial Monitor | ●● |
| `examples/03_web_ui/` | KI-Chat im Browser (kein Kabel) | ●● |
| `examples/04_camera_ai/` | Foto → KI → bei Person blinkt LED | ●●● |

Jedes Projekt ist ein eigenständiges PlatformIO-Projekt — direkt in VS Code öffnen.

---

## Schnellstart

### 1. Erster Flash (einmalig, per Kabel)

Verdrahtung: Arduino Uno als UART-Adapter, `IO0 → GND` für Flash-Modus.

```
Arduino Uno        ESP32-CAM
(RESET → GND!)     ─────────
TX  (Pin 1) ─────▶  IO3 / RX  (Pin 14)
RX  (Pin 0) ◀─────  IO1 / TX  (Pin 15)
GND         ──────  GND  (Pin 7, 12 oder 16)
5V          ──────  5V   (Pin 8 oder 13)
GND         ──────  IO0  (Pin 11)  ← Flash-Modus
```

1. `examples/00_ota_flash/` in VS Code öffnen
2. `BOARD_NR` in `src/main.cpp` anpassen
3. Environment `esp32cam_cable` → Upload

Vollständige Anleitung: [docs/platformio-setup.md](docs/platformio-setup.md)

### 2. Ab jetzt: kabellos flashen

```
Laptop mit ESP32-CAM-01 verbinden → Environment esp32cam_ota_espWlan → Upload
```

### 3. API-Key eintragen

In jedem Sketch (`src/main.cpp`) oben:

```cpp
#define API_KEY  "YOUR_API_KEY"              // ← eigenen Key eintragen
#define API_URL  "http://your-server/api/chat/completions"
#define MODEL    "gpt-4o-mini"
```

WLANs eintragen:

```cpp
wifiMulti.addAP("dein-netzwerk", "dein-passwort");
```

---

## Arduino IDE · XIAO ESP32S3 Sense

Einfachere Sketche für die Arduino IDE — für das **Seeed XIAO ESP32S3 Sense**:

| Ordner | Was |
|--------|-----|
| `arduino/01_describe_image/` | Bild beschreiben |
| `arduino/02_yes_no/` | Ja/Nein-Frage + LED |
| `arduino/03_count_objects/` | Objekte zählen |
| `arduino/04_detect_color/` | Farbe erkennen |
| `arduino/xiao_esp32s3/` | Referenz-Sketch mit `config.h` |

Anleitung: [arduino/README.md](arduino/README.md)

---

## Kompatible APIs

Jeder **OpenAI-kompatible** Endpunkt funktioniert:

| Anbieter | URL | Key |
|----------|-----|-----|
| OpenAI | `https://api.openai.com/v1/chat/completions` | OpenAI API Key |
| LM Studio (lokal) | `http://localhost:1234/v1/chat/completions` | `lm-studio` |
| Ollama (lokal) | `http://localhost:11434/v1/chat/completions` | `ollama` |
| Eigener Server | `http://your-server/api/chat/completions` | je nach Config |

> Für `04_camera_ai` und die Arduino-Sketche: das Modell muss **Vision** (Bildeingabe) unterstützen, z. B. `gpt-4o`, `gpt-4o-mini`, `llava`, `qwen-vl`.

---

## Dokumentation

| Datei | Inhalt |
|-------|--------|
| [CHEATSHEET.md](CHEATSHEET.md) | Schnellreferenz: Pins, Flash-Ablauf, Troubleshooting |
| [docs/hardware.md](docs/hardware.md) | Hardware-Übersicht, technische Eckdaten |
| [docs/pinouts.md](docs/pinouts.md) | Vollständige Pin-Tabelle, ADC, PWM, Spannungen |
| [docs/platformio-setup.md](docs/platformio-setup.md) | PlatformIO installieren, flashen, OTA |

---

## Zugangsdaten (Standard)

| | |
|---|---|
| Board-WLAN | `ESP32-CAM-01` (Nummer je Board anpassen) |
| WLAN-Passwort | `flash1234` |
| OTA-Passwort | `ota1234` |
| Board-AP-IP | `192.168.4.1` |

---

## Lizenz

MIT

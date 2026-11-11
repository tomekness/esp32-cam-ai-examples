# Arduino IDE · XIAO ESP32S3 Sense Sketche

Einfache Sketche für die **Arduino IDE** — für das **Seeed XIAO ESP32S3 Sense** Board mit eingebauter OV2640-Kamera. Weniger Setup als PlatformIO, direkt nutzbar.

---

## Voraussetzungen

### Board-Package (einmalig in Arduino IDE)

`File → Preferences → Additional Boards Manager URLs` — diese URL eintragen:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Dann: `Tools → Board → Boards Manager → "esp32" by Espressif Systems` installieren.

### Externe Bibliothek

| Bibliothek | Autor | Version |
| :--- | :--- | :--- |
| **ArduinoJson** | Benoit Blanchon | **v6.x** (nicht v7!) |

> `Tools → Manage Libraries → "ArduinoJson"` → explizit eine 6.x Version auswählen (z.B. 6.21.5).

### Board-Einstellung in Arduino IDE

`Tools → Board → esp32 → XIAO_ESP32S3`

---

## Konfiguration

Jeder Sketch hat oben einen Konfigurations-Block:

```cpp
#define WIFI_SSID      "dein-netzwerk"
#define WIFI_PASSWORD  "dein-passwort"
#define API_URL        "http://192.168.1.100:4000/chat/completions"
#define API_KEY        "dein-key"
#define MODELL_NAME    "dein-modell"
```

Oder bei `xiao_esp32s3/`: Alles in `config.h` eintragen.

### Kompatible API-Endpunkte

| Anbieter | URL | Key |
|----------|-----|-----|
| OpenAI | `https://api.openai.com/v1/chat/completions` | OpenAI API Key |
| LM Studio (lokal) | `http://localhost:1234/v1/chat/completions` | `lm-studio` |
| Ollama (lokal) | `http://localhost:11434/v1/chat/completions` | `ollama` |

---

## Sketche

| Ordner | Was |
|--------|-----|
| `01_describe_image/` | Bild aufnehmen, Beschreibung im Serial Monitor |
| `02_yes_no/` | Ja/Nein-Frage + LED-Feedback (intern LED) |
| `03_count_objects/` | Periodisch Objekte zählen (alle 10 Sekunden) |
| `04_detect_color/` | Farbanalyse: dominante Farbe, Stimmung, Palette |
| `xiao_esp32s3/` | Referenz-Sketch mit vollständiger `config.h` |

---

## Schnellstart

1. Board-Package installieren (siehe oben)
2. ArduinoJson v6 installieren (siehe oben)
3. Sketch öffnen, WLAN-Daten und API-Endpunkt eintragen
4. Board in Arduino IDE: `XIAO_ESP32S3`
5. Upload, Serial Monitor öffnen (115200 Baud)
6. Enter im Serial Monitor → Bild → API → Antwort

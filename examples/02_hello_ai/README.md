# 02 — Hello AI · Erster API-Call

> ESP32-CAM verbindet sich mit dem WLAN und schickt eine Textfrage an ein KI-Modell. Antwort erscheint im Serial Monitor. Kein Browser, kein Bild — reiner API-Test.

Upload-Anleitung: → **[../../docs/platformio-setup.md](../../docs/platformio-setup.md)**

---

## Was macht dieser Sketch?

1. Öffnet eigenes WLAN (`ESP32-CAM-01`) — für OTA und Direktzugriff
2. Verbindet sich mit dem eingetragenen externen Netz (WiFiMulti)
3. Schickt beim Start automatisch: *„Sag Hallo auf Deutsch in einem Satz."*
4. Gibt die Antwort im Serial Monitor aus
5. Bei jedem Enter im Serial Monitor: Witz-Anfrage

---

## Konfiguration

```cpp
#define BOARD_NR   "01"                            // ← Board-Nummer
#define API_KEY    "YOUR_API_KEY"                  // ← eigenen Key eintragen
#define API_URL    "http://your-server/api/chat/completions"  // ← Endpunkt
#define MODEL      "gpt-4o-mini"                   // ← Modell anpassen
```

WLANs hinzufügen (in `src/main.cpp`):

```cpp
wifiMulti.addAP("dein-netzwerk", "dein-passwort");
```

### Kompatible API-Endpunkte

Jeder OpenAI-kompatible Endpunkt funktioniert:

| Anbieter | URL | Key |
|----------|-----|-----|
| OpenAI | `https://api.openai.com/v1/chat/completions` | OpenAI API Key |
| LM Studio (lokal) | `http://localhost:1234/v1/chat/completions` | `lm-studio` |
| Ollama (lokal) | `http://localhost:11434/v1/chat/completions` | `ollama` |

---

## IP-Adresse finden

Das Board hat immer **zwei IPs**:

| Netz | IP | wann |
|------|----|------|
| Eigener AP | `192.168.4.1` | immer |
| Externes WLAN | dynamisch | wenn verbunden |

**Im Serial Monitor:**
```
AP aktiv:  ESP32-CAM-01  →  192.168.4.1
STA IP:    192.168.0.42  (mein-netz)
```

**Ohne Kabel — TCP-Log:**
```bash
nc ESP32-CAM-01.local 1234
```

---

## Beispielausgabe

```
AP aktiv:  ESP32-CAM-01  →  192.168.4.1
Verbinde mit externem WLAN ...
STA IP:    192.168.0.42  (mein-netz)
OTA bereit  →  ESP32-CAM-01

Frage API...
── Antwort ───────────────────
Hallo! Schön, dass du da bist.
──────────────────────────────
```

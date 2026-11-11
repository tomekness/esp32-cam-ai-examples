# 04 — Camera AI · Foto aufnehmen und KI analysieren lassen

> ESP32 macht ein Foto, schickt es an ein Vision-KI-Modell und reagiert auf die Antwort — bei einer erkannten Person blinkt die Blitz-LED.

Upload-Anleitung: → **[../../docs/platformio-setup.md](../../docs/platformio-setup.md)**

---

## Was macht dieser Sketch?

1. Öffnet Webseite im Browser
2. „Foto & Analyse"-Button drücken → Kamera macht JPEG, schickt es an KI
3. KI antwortet auf die Frage (Standard: *„Ist eine Person zu sehen?"*)
4. Antwort erscheint in der Webseite — grün bei JA, gelb bei NEIN
5. Wenn Person erkannt: **Blitz-LED blinkt 5×** (IO4)
6. Letztes Foto wird in der Webseite angezeigt

Die Frage ist im Browser-Textfeld frei änderbar — z. B. *„Welche Farben siehst du?"* oder *„Wie viele Personen sind zu sehen?"*

---

## Konfiguration

```cpp
#define BOARD_NR          "01"                            // ← Board-Nummer
#define API_KEY           "YOUR_API_KEY"                  // ← eigenen Key eintragen
#define API_URL           "http://your-server/api/chat/completions"  // ← Endpunkt
#define MODEL             "gpt-4o-mini"                   // ← Vision-Modell!
#define KI_FRAGE_DEFAULT  "Ist eine Person zu sehen? …"   // ← Standard-Prompt
```

WLANs hinzufügen (in `src/main.cpp`):

```cpp
wifiMulti.addAP("dein-netzwerk", "dein-passwort");
```

> **Wichtig:** Das Modell muss Vision können (Bildeingabe unterstützen). Beispiele: `gpt-4o`, `gpt-4o-mini`, `claude-3-haiku`, `qwen-vl`, `llava`.

---

## Webseite öffnen

**Option A — über Board-WLAN:**
Laptop mit `ESP32-CAM-01` verbinden → `http://192.168.4.1`

**Option B — mDNS (wenn Board im gleichen Netz):**
`http://ESP32-CAM-01.local`

---

## Routen

| Route | Methode | Funktion |
|-------|---------|---------|
| `/` | GET | HTML-Seite |
| `/foto` | POST | Foto aufnehmen + KI-Analyse, gibt Antwort zurück |
| `/snapshot` | GET | letztes aufgenommenes Foto als JPEG |
| `/log` | GET | aktueller Debug-Puffer |

---

## LEDs

| LED | GPIO | Verhalten |
|-----|------|-----------|
| Rote Status-LED | IO33 (invertiert) | Leuchtet während Verarbeitung |
| Weiße Blitz-LED | IO4 | Blinkt 5× wenn Person erkannt |

---

## Kamera-Einstellungen

Auflösung: **QVGA (320×240)** — klein genug für schnelle JPEG-Übertragung an die API.  
JPEG-Qualität: **10–15** (je nach PSRAM-Verfügbarkeit).

Das erste Frame nach dem Aufruf wird verworfen — der Sensor braucht einen Moment für die Belichtungsanpassung.

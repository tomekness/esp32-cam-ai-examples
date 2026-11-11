# 03 — Web UI · KI-Chat im Browser

> ESP32-CAM hostet eine Webseite. Fragen werden im Browser eingegeben, Antworten erscheinen direkt — kein Serial Monitor nötig.

Upload-Anleitung: → **[../../docs/platformio-setup.md](../../docs/platformio-setup.md)**

---

## Was macht dieser Sketch?

1. Öffnet eigenes WLAN (`ESP32-CAM-01`)
2. Hostet eine Webseite auf Port 80
3. Frage eingeben → Senden → KI antwortet im Browser
4. Debug-Log aktualisiert sich live im Browser

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
| `/ask` | POST | Frage an KI stellen, gibt Antwort zurück |
| `/log` | GET | aktueller Debug-Puffer |

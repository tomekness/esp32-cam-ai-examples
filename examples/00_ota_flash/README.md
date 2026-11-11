# 00 — OTA Flash · Starter-Sketch

> Öffnet einen eigenen WLAN-Access-Point und aktiviert OTA (Over-the-Air-Update). Nach diesem ersten Flash per Kabel können alle weiteren Sketche kabellos hochgeladen werden.

Upload-Anleitung: → **[../../docs/platformio-setup.md](../../docs/platformio-setup.md)**

---

## Was macht dieser Sketch?

1. Öffnet eigenes WLAN (`ESP32-CAM-01`) — für OTA und Direktzugriff
2. Blinkt die rote Status-LED alle 500 ms — Zeichen dass alles läuft
3. Wartet auf OTA-Updates

## Konfiguration

```cpp
#define BOARD_NR     "01"   // ← Board-Nummer anpassen
```

## Nach dem Flash

Ab jetzt kannst du per OTA flashen — Kabel wird nicht mehr gebraucht.

Environment wechseln auf `esp32cam_ota_espWlan`:
1. Laptop mit `ESP32-CAM-01` verbinden (Passwort: `flash1234`)
2. Dropdown unten links in VS Code → `esp32cam_ota_espWlan`
3. Upload starten

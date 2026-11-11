# 01 — Blink · LED-Test

> Einfachster Funktionstest: rote und weiße LED blinken abwechselnd. Zeigt dass das Board läuft und OTA funktioniert.

Upload-Anleitung: → **[../../docs/platformio-setup.md](../../docs/platformio-setup.md)**

---

## Was macht dieser Sketch?

1. Öffnet eigenes WLAN (`ESP32-CAM-01`)
2. Blinkt die rote LED zweimal (Doppelblink) mit 700 ms Pause
3. Blitzt beim Start kurz weiß auf

## Konfiguration

```cpp
#define BOARD_NR     "01"   // ← Board-Nummer anpassen
```

## LEDs

| LED | GPIO | Verhalten |
|-----|------|-----------|
| Rot (Status) | IO33 | Doppelblink, LOW = an |
| Weiß (Flash) | IO4 | Einmaliger Blitz beim Start, HIGH = an |

# ESP32-CAM — Schnellreferenz

---

## Board-Nummer: `ESP32-CAM-___`

| | |
|---|---|
| **AP-WLAN** | `ESP32-CAM-___` |
| **AP-Passwort** | `flash1234` |
| **AP-IP** | `192.168.4.1` |
| **OTA-Passwort** | `ota1234` |
| **Internet-WLAN** | `________________` |
| **Internet-Passwort** | `________________` |

---

## Flash per Kabel (PlatformIO)

**Projekt:** `examples/00_ota_flash/`

### Verdrahtung — Arduino Uno als USB-UART Adapter

```
Arduino Uno        ESP32-CAM
(RESET → GND!)     ─────────
───────────────
TX  (Pin 1) ─────▶  IO3 / RX  (Pin 14)
RX  (Pin 0) ◀─────  IO1 / TX  (Pin 15)
GND         ──────  GND  (Pin 7, 12 oder 16)
5V          ──────  5V   (Pin 8 oder 13)
GND         ──────  IO0  (Pin 11)  ← Flash-Modus!
```

> **GND-Falle!** Der Pin direkt neben der weißen LED ist **nicht GND**.  
> Immer Pin 7, 12 oder 16 verwenden.

### Upload-Ablauf

1. `RESET → GND` auf dem Uno brücken (ATmega deaktivieren)
2. `IO0 → GND` am ESP32-CAM brücken (Flash-Modus)
3. PlatformIO Environment `esp32cam_cable` wählen
4. Upload starten (→ Pfeil)
5. Upload abwarten (`[SUCCESS]`)
6. IO0-Brücke entfernen, Reset → Board läuft

---

## Flash per OTA (WLAN)

Sobald OTA einmal auf dem Board ist:

**Option A — Board-AP (`esp32cam_ota_espWlan`):**
1. Laptop mit `ESP32-CAM-___` verbinden (Passwort: `flash1234`)
2. Environment `esp32cam_ota_espWlan` wählen
3. Port unten in VS Code auf **Auto** stellen
4. Upload starten — OTA-Passwort: `ota1234` (automatisch aus `platformio.ini`)

**Option B — externes Netz (`esp32cam_ota_internet`):**
1. Laptop und Board im gleichen WLAN
2. Environment `esp32cam_ota_internet` wählen → Upload starten

---

## PlatformIO Environments

| Environment | Verbindung | Wann nutzen |
|-------------|-----------|-------------|
| `esp32cam_cable` | USB/Kabel | immer — zuverlässig, kein WLAN nötig |
| `esp32cam_ota_espWlan` | WLAN — Board-AP | wenn OTA aktiv ist, Laptop mit `ESP32-CAM-___` verbinden |
| `esp32cam_ota_internet` | WLAN — externes Netz | wenn Board und Laptop im gleichen Netz |

---

## Pin-Belegung

```
              ESP32-CAM (Draufsicht, Antenne oben)
            ┌──────────────────┐
      5V  8 │●               ●│  9  3.3V  ← AUSGANG! Hier nichts einspeisen
     GND  7 │●               ●│ 10  IO16 (PSRAM)
     IO12 6 │●               ●│ 11  IO0  ← BOOT!
     IO13 5 │●   [ESP32-S]   ●│ 12  GND
     IO15 4 │●               ●│ 13  5V   ← Eingang (= Pin 8)
     IO14 3 │●               ●│ 14  IO3 (RX)
      IO2 2 │●               ●│ 15  IO1 (TX)
      IO4 1 │● (Blitz-LED)   ●│ 16  GND
            └──────────────────┘
            [Kamera + SD-Karte]
```

> **5V anlegen: immer an Pin 8 oder 13** — niemals an Pin 9 (3.3V)!  
> IO33 (rote LED) ist kein Header-Pin — per `digitalWrite(33, ...)` nutzbar.

---

## Spannung & Strom

| | |
|---|---|
| Versorgung | **5V** (nicht 3.3V!) |
| Logikpegel | **3.3V** — GPIOs max. 3.6V! |
| GPIO max. Strom | 12 mA (empfohlen) / 40 mA (absolut) |
| Stromverbrauch | ~180 mA idle / ~310 mA beim Foto |

> **Niemals 5V an einen GPIO!** Direkter 5V-Anschluss zerstört den Chip sofort.

---

## Freie Pins auf einen Blick

| Ich brauche… | Nimm… |
|--------------|-------|
| Taster / digitaler Input | `IO13` oder `IO14` mit `INPUT_PULLUP` |
| LED / digitaler Output | `IO15`, `IO14`, oder `IO33` (rot, schon drauf) |
| PWM (Servo, Dimmen) | jeden freien Output-Pin — z. B. `IO15` |
| Analog lesen | `IO13`–`IO15` — **nur ohne aktives WLAN zuverlässig!** |

---

## Log ohne Kabel lesen

```bash
nc ESP32-CAM-01.local 4210    # TCP-Logserver (Sketche 03/04)
nc ESP32-CAM-01.local 1234    # 02_hello_ai
```

Oder in der Webseite: das Debug-Feld aktualisiert sich automatisch.

---

## Troubleshooting

| Problem | Lösung |
|---------|--------|
| `Connecting…` (endlos) | IO0 nicht auf GND oder Uno RESET-Brücke fehlt |
| `Error Uploading` bei 0 % (OTA, macOS) | Systemeinstellungen → Firewall → Python erlauben |
| `Authentication Failed` (OTA) | OTA-Passwort in `platformio.ini` und Sketch müssen gleich sein |
| Board bootet nicht normal | IO0 noch auf GND? Brücke entfernen, Reset |
| Kein Bild von der Kamera | Kamera nach WiFi initialisieren (wie in `04_camera_ai`) |

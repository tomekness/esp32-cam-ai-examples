# ESP32-CAM · Hardware-Übersicht

## Grundidee

```
Welt  ──[Kamera]──▶  ESP32-CAM  ──[WLAN]──▶  KI-API  ──▶  Antwort
                      (Foto als                (OpenAI,      (Text,
                       JPEG)                   Ollama…)       Zahl,
                                                              ja/nein)
```

Das Board macht ein Foto, schickt es per WLAN an eine KI-API und reagiert auf die Antwort —
mit einer LED, einem Servo, einem Display, einem Ton, oder was auch immer euer Konzept braucht.

Die KI läuft nicht auf dem Board. Das Board ist die **Schnittstelle zwischen der physischen Welt und der API**.

---

## Was ist das eigentlich?

Ein winziger Computer (27 × 41 mm) mit eingebauter WLAN-Antenne und Kamerastecker.  
Kein Betriebssystem, kein Bildschirm – er macht genau das, was ihr programmiert.

```
┌─────────────────────────────┐
│  [Kamera OV2640]            │  ← 2 Megapixel, 65° Blickwinkel
│                             │
│  ESP32 Chip  [PSRAM 4MB]    │  ← Prozessor + Arbeitsspeicher
│  [Flash 4MB]                │  ← Programmspeicher
│                             │
│  [WLAN Antenne]  [SD Slot]  │  ← WLAN integriert, SD-Karte optional
│  ○ Flash-LED (weiß, IO4)    │  ← helles weißes LED, auch als Blitz nutzbar
│  · Status-LED (rot, IO33)   │  ← blinkt im OTA-Sketch
└─────────────────────────────┘
```

---

## Technische Eckdaten

| Eigenschaft | Wert |
| :--- | :--- |
| Maße | 27 × 41 × 5 mm |
| Gewicht | ~10 g |
| Spannung | **5V** (USB-Netzteil, Powerbank) |
| Stromverbrauch | ~180 mA idle / ~310 mA beim Fotografieren |
| CPU | ESP32, Dual-Core 240 MHz |
| RAM | 520 KB intern + 4 MB PSRAM |
| Flash | 4 MB (Programmspeicher) |
| Kamera | OV2640, 2 MP, JPEG/BMP/Graustufen |
| Auflösungen | bis 1600 × 1200 px (UXGA) |
| WLAN | 802.11 b/g/n, 2.4 GHz |
| Bluetooth | 4.2 BLE |
| SD-Karte | bis 4 GB |
| Betriebstemperatur | −20 °C bis +85 °C |

---

## Was kann das Board – und was nicht

**Kann:**
- Foto aufnehmen und als JPEG in den Speicher laden
- Das Foto per HTTPS an eine KI-API schicken (OpenAI Vision, Claude, Ollama, LM Studio…)
- Die Text-Antwort der API auslesen und darauf reagieren: LED, Servo, Display, Ton
- Autonom laufen — Powerbank reicht, kein Laptop nötig
- Per WLAN drahtlos neu programmiert werden (OTA), nach einmaligem Einrichten

**Kann nicht:**
- Video streamen und gleichzeitig API-Calls machen (zu wenig RAM)
- Mit einem USB-Kabel direkt programmiert werden (kein eingebauter USB-Chip)
- Gut mit MicroPython umgehen (Kamera-Support sehr begrenzt)
- Komplexe ML-Modelle lokal ausführen (kein NPU)

---

## Programmier-Workflow

```
Schritt 1 (einmalig):  Kabel + Arduino Uno  →  00_ota_flash per PlatformIO flashen
Schritt 2 (danach):    Kein Kabel mehr      →  alle Sketche per WLAN (OTA)
```

### Warum braucht es einen Adapter?

Der ESP32-CAM hat **keinen eigenen USB-Chip** — er kann nicht direkt per USB-Kabel programmiert werden. Er braucht einen Vermittler (UART-Adapter), der USB in das serielle Protokoll übersetzt.

Wir nutzen einen **Arduino Uno mit deaktiviertem ATmega** (RESET-Pin → GND):

```
Ein Arduino Uno hat zwei Chips:
  ATmega328P      → der Mikrocontroller  (steckt im DIP-Sockel)
  CH340 / 16U2   → kümmert sich nur um USB ↔ Serial

Wenn RESET auf GND liegt, ist der ATmega dauerhaft deaktiviert.
Der USB-Chip arbeitet dann allein als reiner USB-UART-Adapter.
```

### Warum PlatformIO statt Arduino IDE?

Der ESP32-CAM hat PSRAM auf demselben SPI-Bus wie den Flash-Chip. Der Stub-Flasher der Arduino IDE scheitert daran — erkennbar an dieser Warnung:

```
WARNING: Failed to communicate with the flash chip, read/write operations will fail.
```

PlatformIO erlaubt `upload_flags = --no-stub` direkt in der `platformio.ini`, was das Problem umgeht.

---

## Was brauche ich zusätzlich?

### Zum ersten Programmieren (einmalig)

- **Arduino Uno** (als USB-UART Adapter)
- Jumper-Kabel (5×)
- USB-Kabel für den Uno

> Nach dem ersten Flash per Kabel läuft alles per WLAN — der Uno wird danach nicht mehr gebraucht.

### Für euer Projekt (je nach Konzept)

- Taster / Schalter
- LED (externe, farbig)
- Servo oder Schrittmotor
- I2C-Display (z. B. OLED 0.96")
- Powerbank (5V, min. 1A)

---

## Kamera-Auflösungen

| Kürzel | Pixel | Typischer Einsatz |
| :--- | :--- | :--- |
| QQVGA | 160 × 120 | schnell, wenig Speicher |
| QVGA | 320 × 240 | Standard für API-Calls |
| VGA | 640 × 480 | gute Qualität, noch handhabbar |
| SVGA | 800 × 600 | — |
| UXGA | 1600 × 1200 | max. Auflösung, langsam |

Für API-Calls (OpenAI Vision etc.) ist **QVGA oder VGA** ideal — kleines JPEG, schnelle Übertragung, gute Bildqualität für KI-Analyse.

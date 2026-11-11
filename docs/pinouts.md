# ESP32-CAM · Pin-Belegung (AI-Thinker)

## Übersicht

```
              ESP32-CAM (Draufsicht, Antenne oben)
            ┌──────────────────┐
      5V  8 │●               ●│  9  3.3V  ← AUSGANG des Reglers, nichts einspeisen!
     GND  7 │●               ●│ 10  IO16 (PSRAM)
     IO12 6 │●               ●│ 11  IO0  ← BOOT!
     IO13 5 │●   [ESP32-S]   ●│ 12  GND
     IO15 4 │●               ●│ 13  5V   ← Eingang (identisch mit Pin 8)
     IO14 3 │●               ●│ 14  IO3 (RX)
      IO2 2 │●               ●│ 15  IO1 (TX)
      IO4 1 │● (Blitz-LED)   ●│ 16  GND
            └──────────────────┘
            [Kamera + SD-Karte]
```

> **Stromversorgung:** 5V immer an **Pin 8 oder 13**. Pin 9 (3.3V) ist der Ausgang des AMS1117-Reglers — hier wird nichts eingespeist, sonst geht der Regler kaputt.  
> **IO33** (rote Status-LED) ist kein Header-Pin — intern verlötet, per `digitalWrite(33, LOW/HIGH)` nutzbar.

---

## Pin-Tabelle

| Pin | GPIO | Funktion | Nutzbar? | Hinweis |
| :--- | :--- | :--- | :--- | :--- |
| 1 | IO4 | Weiße LED / SD D1 | bedingt | `HIGH` = LED an (sehr hell!) |
| 2 | IO2 | GPIO / SD D0 | bedingt | wird auch für SD genutzt |
| 3 | IO14 | GPIO / SD CLK | ja | frei verwendbar |
| 4 | IO15 | GPIO / SD CMD | ja | frei verwendbar |
| 5 | IO13 | GPIO / SD D3 | ja | frei verwendbar |
| 6 | IO12 | GPIO / SD D2 | bedingt | LOW beim Boot nötig |
| 7 | — | GND | — | Masse |
| 8 | — | 5V | **Eingang** | Stromversorgung — hier 5V anlegen |
| 9 | — | 3.3V | **Ausgang** | Ausgang des AMS1117 — niemals hier einspeisen! |
| 10 | IO16 | PSRAM CS | nein | intern reserviert |
| 11 | IO0 | Boot-Modus | Sonderrolle | `GND` = Flash-Modus |
| 12 | — | GND | — | Masse |
| 13 | — | 5V | Eingang | Stromversorgung (= Pin 8) |
| 14 | IO3 | UART RX | Adapter | Daten empfangen (vom Uno TX) |
| 15 | IO1 | UART TX | Adapter | Serial.print() (zum Uno RX) |
| 16 | — | GND | — | Masse |
| intern | IO33 | Rote Status-LED | ja (Code) | `LOW` = an — kein Header-Pin! |

---

## Erstes Programmieren: Arduino Uno als USB-UART Adapter

RESET-Pin des Uno mit GND brücken — ATmega bleibt im Sockel, ist nur deaktiviert.
Der USB-Chip (CH340 / ATmega16U2) fungiert dann als reiner USB-Serial-Adapter.

```
Arduino Uno           ESP32-CAM
(RESET → GND!)        ──────────
──────────────
RESET → GND  ←── Jumper auf dem Uno (ATmega deaktivieren)
TX  (Pin 1)  ──────────▶  IO3 (RX, Pin 14)
RX  (Pin 0)  ◀──────────  IO1 (TX, Pin 15)
GND          ───────────  GND (Pin 7, 12 oder 16)
5V           ───────────  5V  (Pin 8 oder 13)

Beim Flashen zusätzlich:
GND          ───────────  IO0 (Pin 11)  ← Flash-Modus
```

> **GND-Falle:** Pin direkt neben der weißen LED (IO4-Seite) ist kein GND — Pin 7, 12 oder 16 nutzen.

---

## Freie GPIOs (empfohlen für Projekte)

| GPIO | Empfehlung |
| :--- | :--- |
| IO13 | Taster, Sensor, digitaler Input |
| IO14 | Taster, Sensor, digitaler Input |
| IO15 | LED, Servo, PWM-Output |
| IO33 | bereits rote LED (LOW = an) |
| IO4 | bereits weiße LED (HIGH = an, sehr hell) |
| IO2 | Vorsicht: SD-Karte teilt sich diesen Pin |

---

## Kamera-Pins (intern, nicht frei nutzbar)

`IO32, IO26, IO27, IO35, IO34, IO39, IO36, IO21, IO19, IO18, IO5, IO25, IO23`

---

## Spannungen

| Was | Wert |
| :--- | :--- |
| Versorgungsspannung | **5V** |
| Interne Logikspannung | 3.3V |
| GPIO Output HIGH | 3.3V |
| GPIO Output LOW | 0V |
| Maximale Eingangsspannung an einem GPIO | **3.6V absolut** |
| Maximaler Strom pro GPIO-Pin | 12 mA (empfohlen) / 40 mA (absolut max.) |
| Stromverbrauch gesamt | ~180 mA idle / ~310 mA beim Foto |

> **Niemals 5V direkt an einen GPIO anlegen.** Für 5V-Sensoren → Spannungsteiler oder Level Shifter.

---

## PWM

```cpp
// Kanal 0 ist von der Kamera belegt — ab Kanal 1 starten!
ledcSetup(1, 5000, 8);    // Kanal 1, 5000 Hz, 8 Bit
ledcAttachPin(15, 1);     // IO15 → Kanal 1
ledcWrite(1, 128);        // 50% Duty Cycle
```

---

## Analog-Eingang (ADC)

**Wichtig:** Alle freien Pins (IO13, IO14, IO15, IO2, IO4) gehören zu **ADC2** — der **nicht funktioniert wenn WLAN aktiv ist**.

Für analoge Messungen im WLAN-Betrieb: externen ADC über I2C nutzen (z. B. ADS1115).

---

## Boot-Strapping Pins

| GPIO | Muss beim Boot | Folge wenn falsch |
| :--- | :--- | :--- |
| IO0 | HIGH (floatend = ok) | LOW = Flash-Modus, startet nicht normal |
| IO2 | LOW oder floatend | HIGH kann Boot blockieren |
| IO12 | LOW | HIGH = falsche Flash-Spannung, Chip kann sich selbst zerstören |
| IO15 | HIGH | LOW = kein Serial-Output beim Boot |

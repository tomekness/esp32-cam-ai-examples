# ESP32-CAM · PlatformIO Setup

Schritt-für-Schritt-Referenz für Setup, ersten Flash und alle Upload-Methoden.

---

## PlatformIO installieren

1. **VS Code** installieren → [code.visualstudio.com](https://code.visualstudio.com)
2. In VS Code: Erweiterungen (Würfel-Icon links) → `PlatformIO IDE` suchen → **Installieren**
3. VS Code neu starten — PlatformIO-Icon erscheint in der linken Leiste

> Beim ersten Öffnen eines Projekts lädt PlatformIO die ESP32-Platform automatisch herunter (~500 MB). Das dauert einige Minuten.

---

## Projekt öffnen

Den **Projektordner** direkt öffnen — nicht den übergeordneten Ordner.

```
VS Code → Datei → Ordner öffnen → examples/01_blink/   ← genau diesen Ordner
```

PlatformIO erkennt die `platformio.ini` und initialisiert alles automatisch.

---

## Upload-Methoden

Jedes Projekt hat drei Environments in der `platformio.ini`. Environment wechseln: **Dropdown unten links** in VS Code (neben dem Pfeil-Icon).

| Environment | Verbindung | Wann nutzen |
|-------------|-----------|-------------|
| `esp32cam_cable` | USB — Arduino Uno als UART-Adapter | immer, zuverlässig |
| `esp32cam_ota_espWlan` | WLAN — direkt über eigenes Board-AP | wenn Board in Reichweite |
| `esp32cam_ota_internet` | WLAN — Laptop und Board im gleichen Netz | flexibel, kabellos |

---

## Erster Flash — per Kabel

Beim allerersten Flash (frisches Board) muss der Flash-Modus per Kabel aktiviert werden.

### Verdrahtung — Arduino Uno als UART-Adapter

```
Arduino Uno                 ESP32-CAM
(RESET → GND!)              ──────────
─────────────────
TX  (Pin 1)  ──────────▶  IO3 / RX  (Pin 14)
RX  (Pin 0)  ◀──────────  IO1 / TX  (Pin 15)
GND          ───────────  GND  (Pin 7, 12 oder 16)
5V           ───────────  5V   (Pin 8 oder 13)
GND          ───────────  IO0  (Pin 11)   ← Flash-Modus!
```

> **GND-Falle:** Der Pin direkt neben der weißen LED ist **nicht GND** — Pin 7, 12 oder 16 nutzen.

### Ablauf

1. `RESET → GND` auf dem Arduino Uno brücken
2. `IO0 → GND` am ESP32-CAM brücken  
3. Environment `esp32cam_cable` wählen (Dropdown unten links)
4. Upload starten (→ Pfeil-Icon)
5. `[SUCCESS]` abwarten
6. IO0-Brücke entfernen → Reset → Board läuft

Nach dem ersten Flash ist OTA aktiv und alle weiteren Sketche können kabellos hochgeladen werden.

---

## OTA-Flash

Sobald OTA einmal aktiv ist (durch irgendeinen Sketch):

**Option A — über Board-WLAN (`esp32cam_ota_espWlan`):**
1. Laptop mit `ESP32-CAM-01` verbinden (Passwort: `flash1234`)
2. Environment `esp32cam_ota_espWlan` wählen
3. Upload starten

**Option B — über externes Netz (`esp32cam_ota_internet`):**
1. Laptop und Board ins gleiche Netz einbuchen
2. Environment `esp32cam_ota_internet` wählen
3. Upload starten

> **Kabel und OTA sind gleichwertig** — Kabel funktioniert immer, auch wenn OTA mal nicht klappt.

### macOS: OTA schlägt bei 0 % fehl

espota sendet eine Einladung ans Board (UDP), das Board verbindet sich dann **zurück** zum Laptop (TCP). Wenn macOS diese eingehende Verbindung blockiert, schlägt der Upload sofort bei 0 % fehl.

**Fix:** Systemeinstellungen → Datenschutz & Sicherheit → Firewall → Firewall-Optionen → Python-Einträge suchen → **„Eingehende Verbindungen erlauben"**

> macOS fragt beim ersten OTA-Upload einmalig nach — „Ablehnen" speichert die Sperre dauerhaft.

---

## Troubleshooting

### Upload schlägt fehl — Kabel (`esp32cam_cable`)

| Problem | Lösung |
|---------|--------|
| `Connecting…` (endlos) | IO0 nicht auf GND oder Uno RESET-Brücke fehlt |
| `No serial data received` | IO0 nicht auf GND / kein Reset |
| `Invalid head of packet` | Uno RESET-Brücke fehlt → ATmega stört die Leitung |
| `Serial data stream stopped` | `upload_flags = --no-stub` fehlt in `platformio.ini` (PSRAM-Bug), oder Kabelkontakt prüfen |
| Port busy | Anderen Serial Monitor schließen, der den Port belegt |

### Upload schlägt fehl — OTA (`esp32cam_ota_espWlan`)

| Problem | Lösung |
|---------|--------|
| `Error Uploading` bei 0 % (macOS) | macOS blockiert Python-Rückverbindung → Systemeinstellungen → Datenschutz & Sicherheit → Firewall → Python erlauben |
| `Error Uploading` bei 0 % (falsches WLAN) | Laptop muss mit `ESP32-CAM-___` verbunden sein, nicht mit Heim-/Uni-WLAN |
| `Authentication Failed` | OTA-Passwort stimmt nicht — `platformio.ini` (`--auth=…`) und Sketch (`setPassword(…)`) müssen identisch sein |
| OTA klappt gar nicht | Kein Problem — einfach per Kabel flashen (`esp32cam_cable`) |

### Keine LED leuchtet

1. **IO0 noch auf GND?** → Board bootet in Flash-Modus, kein Sketch läuft → IO0-Brücke entfernen, Reset
2. **Noch kein Sketch geflasht** → `00_ota_flash` per Kabel flashen
3. **Falscher GND** → Pin neben weißer LED ist kein GND (s. o.)
4. **Zu wenig Strom** → Powerbank direkt an 5V/GND (nicht über FTDI-Adapter)

### ⚡ Achtung: 5V-Schaden

Der Arduino Uno sendet **5V** auf TX. Der ESP32-CAM RX-Pin verträgt max. **3.6V**.  
Kurzfristig funktioniert das meistens — für häufiges Flashen: Spannungsteiler (1 kΩ + 2 kΩ) oder CH340-Adapter (3.3V).

// ── Konfiguration ─────────────────────────────────────────────────────────────
#define BOARD_NR      "01"           // ← anpassen: 01, 02, 03 …

#define AP_SSID       "ESP32-CAM-" BOARD_NR
#define AP_PASSWORD   "flash1234"

// ── API-Zugangsdaten ──────────────────────────────────────────────────────────
// OpenAI-kompatibler Endpunkt mit Vision-Fähigkeit (z.B. gpt-4o, qwen-vl, …)
#define API_KEY       "YOUR_API_KEY"              // ← eigenen Key eintragen
#define API_URL       "http://your-server/api/chat/completions"  // ← Endpunkt eintragen
#define MODEL         "gpt-4o-mini"               // ← Modell anpassen (muss Vision können!)
// ─────────────────────────────────────────────────────────────────────────────

#define OTA_PASSWORD  "ota1234"
#define LOG_PORT      4210

#define KI_FRAGE_DEFAULT \
    "Ist eine Person zu sehen? Antworte auf Deutsch, beginne mit JA oder NEIN, " \
    "dann kurze Beschreibung in einem Satz."
// ──────────────────────────────────────────────────────────────────────────────

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiServer.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "lwip/dns.h"
#include "esp_camera.h"
#include "mbedtls/base64.h"
#include "soc/rtc_cntl_reg.h"   // für WRITE_PERI_REG (Brownout-Disable)

// ── GPIO ──────────────────────────────────────────────────────────────────────
#define LED_STATUS  33   // kleine rote LED (LOW = an, Logik invertiert)
#define LED_FLASH    4   // Blitz-LED (HIGH = an)

// ── Kamera-Pins (AI-Thinker ESP32-CAM) ───────────────────────────────────────
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiMulti  wifiMulti;
WebServer  server(80);
WiFiServer logServer(LOG_PORT);
WiFiClient logClient;
String     logBuffer = "";

// letztes Foto für /snapshot
uint8_t  *letztesBild    = nullptr;
size_t    letztesBildLen = 0;

bool sta_verbinden();  // forward declaration

// ── Log ───────────────────────────────────────────────────────────────────────
void logln(const String &msg) {
    Serial.println(msg);
    logBuffer += msg + "\n";
    if (logBuffer.length() > 3000)
        logBuffer = logBuffer.substring(logBuffer.length() - 3000);
    if (logServer.hasClient()) {
        if (logClient) logClient.stop();
        logClient = logServer.available();
    }
    if (logClient && logClient.connected())
        logClient.println(msg);
}

// ── Kamera init ───────────────────────────────────────────────────────────────
bool kamera_init() {
    camera_config_t cfg;
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.pin_d0       = Y2_GPIO_NUM;
    cfg.pin_d1       = Y3_GPIO_NUM;
    cfg.pin_d2       = Y4_GPIO_NUM;
    cfg.pin_d3       = Y5_GPIO_NUM;
    cfg.pin_d4       = Y6_GPIO_NUM;
    cfg.pin_d5       = Y7_GPIO_NUM;
    cfg.pin_d6       = Y8_GPIO_NUM;
    cfg.pin_d7       = Y9_GPIO_NUM;
    cfg.pin_xclk     = XCLK_GPIO_NUM;
    cfg.pin_pclk     = PCLK_GPIO_NUM;
    cfg.pin_vsync    = VSYNC_GPIO_NUM;
    cfg.pin_href     = HREF_GPIO_NUM;
    cfg.pin_sccb_sda = SIOD_GPIO_NUM;
    cfg.pin_sccb_scl = SIOC_GPIO_NUM;
    cfg.pin_pwdn     = PWDN_GPIO_NUM;
    cfg.pin_reset    = RESET_GPIO_NUM;
    cfg.xclk_freq_hz = 20000000;
    cfg.pixel_format = PIXFORMAT_JPEG;
    // QVGA (320×240) = klein genug für Base64-Übertragung zur KI
    cfg.frame_size   = FRAMESIZE_QVGA;
    if (psramFound()) {
        cfg.jpeg_quality = 10;
        cfg.fb_count     = 2;
        cfg.grab_mode    = CAMERA_GRAB_LATEST;
        cfg.fb_location  = CAMERA_FB_IN_PSRAM;
    } else {
        cfg.jpeg_quality = 15;
        cfg.fb_count     = 1;
        cfg.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
        cfg.fb_location  = CAMERA_FB_IN_DRAM;
    }

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        logln("Kamera-Fehler: 0x" + String(err, HEX));
        return false;
    }
    logln("Kamera bereit | PSRAM: " + String(psramFound() ? "ja" : "nein") +
          " | Heap: " + String(ESP.getFreeHeap()) + " B");
    return true;
}

// ── Blitz blinken (zur Rückmeldung) ──────────────────────────────────────────
void flash_blinken(int mal) {
    for (int i = 0; i < mal; i++) {
        digitalWrite(LED_FLASH, HIGH); delay(120);
        digitalWrite(LED_FLASH, LOW);  delay(120);
    }
}

// ── Foto aufnehmen + Base64 + KI-Analyse ─────────────────────────────────────
String foto_analysieren(const String &frage) {
    logln("Foto aufnehmen … | Heap: " + String(ESP.getFreeHeap()) +
          " B | PSRAM: " + String(ESP.getFreePsram()) + " B");

    // 2 Warm-up-Frames verwerfen — OV2640 braucht einen Moment für AEC/AGC
    camera_fb_t *fb = nullptr;
    for (int i = 0; i < 2; i++) {
        fb = esp_camera_fb_get();
        if (fb) { esp_camera_fb_return(fb); fb = nullptr; }
        delay(100);
    }

    fb = esp_camera_fb_get();
    if (!fb) { logln("Kein Kamerabild!"); return ""; }
    logln("Bild: " + String(fb->len) + " Bytes");

    // Letztes Bild für /snapshot speichern
    if (letztesBild) { free(letztesBild); letztesBild = nullptr; }
    letztesBild = (uint8_t*)ps_malloc(fb->len);
    if (!letztesBild) letztesBild = (uint8_t*)malloc(fb->len);
    if (letztesBild) {
        memcpy(letztesBild, fb->buf, fb->len);
        letztesBildLen = fb->len;
    }

    // Base64-Kodierung
    size_t b64_max = ((fb->len + 2) / 3) * 4 + 1;
    uint8_t *b64 = (uint8_t*)ps_malloc(b64_max);
    if (!b64) b64 = (uint8_t*)malloc(b64_max);
    if (!b64) {
        logln("Kein Speicher für Base64!");
        esp_camera_fb_return(fb);
        return "";
    }
    size_t b64_len = 0;
    mbedtls_base64_encode(b64, b64_max, &b64_len, fb->buf, fb->len);
    b64[b64_len] = '\0';
    esp_camera_fb_return(fb);   // Kamera-Puffer sofort freigeben
    logln("Base64: " + String(b64_len) + " Bytes");

    // JSON-Request manuell aufbauen (zu groß für ArduinoJson)
    // Frage JSON-sicher machen (nur " escapen reicht für unsere Prompts)
    String frageEsc;
    frageEsc.reserve(frage.length() + 10);
    for (char c : frage) {
        if (c == '"')       frageEsc += "\\\"";
        else if (c == '\\') frageEsc += "\\\\";
        else                frageEsc += c;
    }

    String body;
    body.reserve(b64_len + frageEsc.length() + 200);
    body  = "{\"model\":\"" MODEL "\","
            "\"messages\":[{\"role\":\"user\",\"content\":["
            "{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:image/jpeg;base64,";
    body += (char*)b64;
    body += "\"}},"
            "{\"type\":\"text\",\"text\":\"";
    body += frageEsc;
    body += "\"}]}]}";
    free(b64);
    logln("Request: " + String(body.length()) + " Bytes → KI …");

    if (!sta_verbinden()) { return ""; }

    HTTPClient http;
    WiFiClientSecure secureClient;
    WiFiClient       plainClient;

    if (String(API_URL).startsWith("https://")) {
        secureClient.setInsecure();
        http.begin(secureClient, API_URL);
    } else {
        http.begin(plainClient, API_URL);
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " API_KEY);
    http.setTimeout(45000);

    int code = http.POST(body);
    body = "";   // Speicher freigeben bevor Antwort geparsed wird

    if (code <= 0) {
        logln("Verbindungsfehler — HTTP " + String(code));
        http.end();
        return "";
    }
    if (code != 200) {
        logln("HTTP " + String(code));
        http.end();
        return "";
    }

    DynamicJsonDocument res(8192);
    DeserializationError jerr = deserializeJson(res, http.getString());
    http.end();
    if (jerr) { logln("JSON-Fehler: " + String(jerr.c_str())); return ""; }

    String antwort = res["choices"][0]["message"]["content"].as<String>();
    logln("← " + antwort.substring(0, 120) + (antwort.length() > 120 ? "…" : ""));
    return antwort;
}

// ── HTML ──────────────────────────────────────────────────────────────────────
const char HTML[] PROGMEM = R"html(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Kamera-KI</title>
  <style>
    * { box-sizing: border-box; }
    body { font-family: sans-serif; max-width: 640px; margin: 2rem auto; padding: 1rem; }
    h2 { margin: 0 0 1.5rem; }
    .label { display: block; margin-top: 1.2rem; font-size: .8rem; font-weight: bold;
             color: #666; text-transform: uppercase; letter-spacing: .05em; }
    input[type=text] { width: 100%; padding: .6rem .8rem; font-size: 1rem;
                       border: 1px solid #ccc; border-radius: 6px; margin-top: .3rem; }
    input[type=text]:focus { outline: none; border-color: #333; }
    .row { display: flex; gap: .5rem; margin-top: .6rem; }
    button { padding: .6rem 1.4rem; font-size: 1rem; background: #222; color: #fff;
             border: none; border-radius: 6px; cursor: pointer; }
    button:hover { background: #444; }
    button:disabled { background: #999; cursor: default; }
    #foto { display: block; width: 100%; border-radius: 6px; margin-top: .8rem;
            background: #eee; min-height: 140px; object-fit: contain; }
    #antwort { margin-top: .3rem; min-height: 60px; padding: .8rem;
               background: #f7f7f7; border-radius: 6px; white-space: pre-wrap;
               line-height: 1.5; font-size: 1rem; border-left: 4px solid transparent; }
    #antwort.person  { background: #d4edda; border-color: #28a745; }
    #antwort.kein    { background: #fff3cd; border-color: #ffc107; }
    #status { font-size: .9rem; color: #888; min-height: 1.3em; margin-top: .4rem; }
    #debug { margin-top: .3rem; min-height: 60px; max-height: 200px; overflow-y: auto;
             padding: .7rem; background: #111; color: #4f4; font-family: monospace;
             font-size: .78rem; border-radius: 6px; white-space: pre-wrap; }
  </style>
</head>
<body>
  <h2>ESP32 · Kamera-KI</h2>

  <span class="label">Frage an die KI</span>
  <input type="text" id="q"
    value="Ist eine Person zu sehen? Antworte auf Deutsch, beginne mit JA oder NEIN, dann kurze Beschreibung in einem Satz.">

  <div class="row">
    <button id="btn" onclick="scan()">Foto &amp; Analyse</button>
    <button onclick="vorschau()">Vorschau</button>
  </div>

  <div id="status"></div>
  <img id="foto" alt="Kamerabild">

  <span class="label">KI-Antwort</span>
  <div id="antwort">—</div>

  <span class="label">Debug-Log</span>
  <div id="debug"></div>

  <script>
    const btn = document.getElementById('btn');

    async function scan() {
      const q = document.getElementById('q').value.trim();
      if (!q) return;
      btn.disabled = true;
      btn.textContent = '…';
      document.getElementById('status').textContent = 'Foto aufnehmen und analysieren …';
      const el = document.getElementById('antwort');
      el.textContent = '';
      el.className = '';
      try {
        const r = await fetch('/foto', { method: 'POST', body: q });
        const text = await r.text();
        el.textContent = text;
        const upper = text.trim().toUpperCase();
        if (upper.startsWith('JA'))   el.className = 'person';
        else if (upper.startsWith('NEIN')) el.className = 'kein';
        // Letztes Foto nachladen
        document.getElementById('foto').src = '/snapshot?t=' + Date.now();
      } catch(e) {
        el.textContent = 'Fehler: ' + e;
      }
      document.getElementById('status').textContent = '';
      btn.disabled = false;
      btn.textContent = 'Foto & Analyse';
    }

    function vorschau() {
      document.getElementById('foto').src = '/snapshot?t=' + Date.now();
    }

    document.getElementById('q').addEventListener('keydown', e => {
      if (e.key === 'Enter') scan();
    });

    async function pollLog() {
      try {
        const r = await fetch('/log');
        const el = document.getElementById('debug');
        el.textContent = await r.text();
        el.scrollTop = el.scrollHeight;
      } catch(_) {}
    }
    setInterval(pollLog, 2000);
    pollLog();
  </script>
</body>
</html>
)html";

// ── Web-Server ─────────────────────────────────────────────────────────────────
void server_einrichten() {
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", HTML);
    });

    server.on("/foto", HTTP_POST, []() {
        String frage = server.arg("plain");
        if (frage.isEmpty()) frage = KI_FRAGE_DEFAULT;

        digitalWrite(LED_STATUS, LOW);   // Status-LED an (Verarbeitung läuft)
        String antwort = foto_analysieren(frage);
        digitalWrite(LED_STATUS, HIGH);

        if (antwort.isEmpty()) {
            server.send(500, "text/plain; charset=utf-8", "Fehler beim Analysieren.");
            return;
        }

        // Person erkannt? → Blitz blinken
        String check = antwort;
        check.trim();
        bool person = check.length() >= 2 &&
                      (check[0] == 'J' || check[0] == 'j') &&
                      (check[1] == 'A' || check[1] == 'a');
        if (person) {
            logln("→ Person erkannt — Blitz!");
            flash_blinken(5);
        }

        server.send(200, "text/plain; charset=utf-8", antwort);
    });

    // Letztes aufgenommenes Foto als JPEG ausliefern
    server.on("/snapshot", HTTP_GET, []() {
        if (letztesBild && letztesBildLen > 0) {
            server.send_P(200, PSTR("image/jpeg"),
                          (const char*)letztesBild, letztesBildLen);
        } else {
            // Kein gespeichertes Bild → frisch aufnehmen
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) { server.send(503, "text/plain", "Kein Bild"); return; }
            server.send_P(200, PSTR("image/jpeg"),
                          (const char*)fb->buf, fb->len);
            esp_camera_fb_return(fb);
        }
    });

    server.on("/log", HTTP_GET, []() {
        server.send(200, "text/plain; charset=utf-8", logBuffer);
    });

    server.begin();
    String url = "http://" + (WiFi.status() == WL_CONNECTED
        ? WiFi.localIP().toString()
        : WiFi.softAPIP().toString());
    logln("Webseite: " + url);
}

// ── WiFi ──────────────────────────────────────────────────────────────────────
void wlan_ap_starten() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    logln(String("AP:  ") + AP_SSID + "  →  http://" + WiFi.softAPIP().toString());

    // ── WLANs eintragen — Board verbindet sich mit dem stärksten verfügbaren ──
    // wifiMulti.addAP("dein-netzwerk", "dein-passwort");
    // wifiMulti.addAP("anderes-netz",  "anderes-passwort");
    // ─────────────────────────────────────────────────────────────────────────
}

// Verbindet STA-WiFi falls nötig — wird vor jedem API-Call gerufen.
bool sta_verbinden() {
    if (WiFi.status() == WL_CONNECTED) return true;
    logln("WLAN verbinden …");
    if (wifiMulti.run(10000) != WL_CONNECTED) {
        logln("Kein WLAN — API nicht erreichbar");
        return false;
    }
    ip_addr_t dns;
    IP_ADDR4(&dns, 8, 8, 8, 8);
    dns_setserver(0, &dns);
    logServer.begin();
    logln("WLAN: " + WiFi.localIP().toString() + "  (" + WiFi.SSID() + ")");
    return true;
}

// ── Setup ──────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // Brownout-Detektor ausschalten — ESP32-CAM + WiFi-Spitzen übersteigen
    // die USB-Stromgrenze des Arduino Uno (~500 mA). Echte Lösung: ESP32-CAM
    // separat mit Powerbank/USB-Ladegerät versorgen, Uno nur für TX/RX/GND.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    pinMode(LED_STATUS, OUTPUT);
    pinMode(LED_FLASH,  OUTPUT);
    digitalWrite(LED_STATUS, HIGH);  // aus (invertierte Logik)
    digitalWrite(LED_FLASH,  LOW);   // aus

    // WiFi (AP) ZUERST — danach Kamera initialisieren.
    // STA-Verbindung wird erst beim ersten API-Call aufgebaut.
    wlan_ap_starten();

    ArduinoOTA.setHostname("ESP32-CAM-" BOARD_NR);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
    logln("OTA bereit");

    // Kamera NACH WiFi — sonst überschreibt WiFi-Init die I2S-DMA-Konfiguration
    if (!kamera_init()) {
        for (int i = 0; i < 10; i++) {
            digitalWrite(LED_STATUS, LOW);  delay(80);
            digitalWrite(LED_STATUS, HIGH); delay(80);
        }
    }

    server_einrichten();
    logln("Bereit — Webseite öffnen!");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    ArduinoOTA.handle();
    server.handleClient();
    if (logServer.hasClient()) {
        if (logClient) logClient.stop();
        logClient = logServer.available();
    }
}

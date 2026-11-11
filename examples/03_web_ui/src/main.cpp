// ── Konfiguration ─────────────────────────────────────────────────────────────
#define BOARD_NR      "01"           // ← anpassen: 01, 02, 03 …

#define AP_SSID       "ESP32-CAM-" BOARD_NR
#define AP_PASSWORD   "flash1234"

// ── API-Zugangsdaten ──────────────────────────────────────────────────────────
// OpenAI-kompatibler Endpunkt — z.B. OpenAI, LM Studio, Ollama, eigener Server
#define API_KEY       "YOUR_API_KEY"              // ← eigenen Key eintragen
#define API_URL       "http://your-server/api/chat/completions"  // ← Endpunkt eintragen
#define MODEL         "gpt-4o-mini"               // ← Modell anpassen
// ─────────────────────────────────────────────────────────────────────────────

#define OTA_PASSWORD  "ota1234"

#define LOG_PORT      4210   // nc ESP32-CAM-01.local 4210  auf dem Laptop
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

#define LED 33

WiFiMulti  wifiMulti;
WebServer  server(80);
WiFiServer logServer(LOG_PORT);
WiFiClient logClient;
String     logBuffer = "";

// ── Log: Serial + TCP-Client + Puffer für Webseite ───────────────────────────
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

// ── WiFi (AP immer an + STA für externes Netz) ───────────────────────────────
void wlan_verbinden() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    logln(String("AP:  ") + AP_SSID + "  →  http://" + WiFi.softAPIP().toString());

    // ── WLANs eintragen — Board verbindet sich mit dem stärksten verfügbaren ──
    // wifiMulti.addAP("dein-netzwerk", "dein-passwort");
    // wifiMulti.addAP("anderes-netz",  "anderes-passwort");
    // ─────────────────────────────────────────────────────────────────────────

    logln("Verbinde mit externem WLAN ...");
    if (wifiMulti.run(15000) == WL_CONNECTED) {
        // DNS direkt per LWIP setzen — WiFi.config() stört die Verbindung
        ip_addr_t dns;
        IP_ADDR4(&dns, 8, 8, 8, 8);
        dns_setserver(0, &dns);
        logServer.begin();
        logln("STA IP:    " + WiFi.localIP().toString() + "  (" + WiFi.SSID() + ")");
        logln("Log:       nc ESP32-CAM-" BOARD_NR ".local " + String(LOG_PORT));
    } else {
        logln("Kein externes WLAN — nur AP aktiv");
    }
}

// ── API-Call ──────────────────────────────────────────────────────────────────
String api_fragen(const String &frage) {
    logln("→ " + frage);

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
    http.setTimeout(30000);

    DynamicJsonDocument req(512);
    req["model"] = MODEL;
    JsonArray msgs = req.createNestedArray("messages");
    JsonObject msg = msgs.createNestedObject();
    msg["role"]    = "user";
    msg["content"] = frage;

    String body;
    serializeJson(req, body);

    int code = http.POST(body);
    if (code <= 0) {
        logln("Verbindungsfehler (DNS/WLAN?) — HTTP " + String(code));
        http.end();
        return "";
    }
    if (code != 200) {
        logln("HTTP " + String(code) + ": " + http.getString());
        http.end();
        return "";
    }

    DynamicJsonDocument res(2048);
    deserializeJson(res, http.getString());
    http.end();

    String antwort = res["choices"][0]["message"]["content"].as<String>();
    logln("← " + antwort.substring(0, 100) + (antwort.length() > 100 ? "…" : ""));
    return antwort;
}

// ── HTML (im Flash gespeichert) ───────────────────────────────────────────────
const char HTML[] PROGMEM = R"html(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 AI</title>
  <style>
    * { box-sizing: border-box; }
    body { font-family: sans-serif; max-width: 640px; margin: 2rem auto; padding: 1rem; background: #fff; }
    h2 { margin: 0 0 1.5rem; }
    label { display: block; margin-top: 1.2rem; font-size: .8rem; font-weight: bold; color: #666; text-transform: uppercase; letter-spacing: .05em; }
    input[type=text] { width: 100%; padding: .6rem .8rem; font-size: 1rem; border: 1px solid #ccc; border-radius: 6px; margin-top: .3rem; }
    input[type=text]:focus { outline: none; border-color: #333; }
    button { margin-top: .6rem; padding: .6rem 1.6rem; font-size: 1rem; background: #222; color: #fff; border: none; border-radius: 6px; cursor: pointer; }
    button:hover { background: #444; }
    button:disabled { background: #999; cursor: default; }
    #reply { margin-top: .3rem; min-height: 80px; padding: .7rem; background: #f7f7f7; border-radius: 6px; white-space: pre-wrap; line-height: 1.5; }
    #debug { margin-top: .3rem; min-height: 80px; max-height: 200px; overflow-y: auto; padding: .7rem; background: #111; color: #4f4; font-family: monospace; font-size: .78rem; border-radius: 6px; white-space: pre-wrap; }
  </style>
</head>
<body>
  <h2>ESP32 · AI</h2>

  <label>Frage</label>
  <input type="text" id="q" placeholder="Was möchtest du wissen?">
  <button id="btn" onclick="ask()">Senden</button>

  <label>Antwort</label>
  <div id="reply">—</div>

  <label>Debug</label>
  <div id="debug"></div>

  <script>
    const btn = document.getElementById('btn');

    async function ask() {
      const q = document.getElementById('q').value.trim();
      if (!q) return;
      btn.disabled = true;
      btn.textContent = '…';
      document.getElementById('reply').textContent = '';
      try {
        const r = await fetch('/ask', { method: 'POST', body: q });
        document.getElementById('reply').textContent = await r.text();
      } catch(e) {
        document.getElementById('reply').textContent = 'Fehler: ' + e;
      }
      btn.disabled = false;
      btn.textContent = 'Senden';
    }

    document.getElementById('q').addEventListener('keydown', e => {
      if (e.key === 'Enter') ask();
    });

    async function pollLog() {
      try {
        const r = await fetch('/log');
        const el = document.getElementById('debug');
        el.textContent = await r.text();
        el.scrollTop = el.scrollHeight;
      } catch(_) {}
    }
    setInterval(pollLog, 1500);
    pollLog();
  </script>
</body>
</html>
)html";

// ── Web-Server einrichten ─────────────────────────────────────────────────────
void server_einrichten() {
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", HTML);
    });

    server.on("/ask", HTTP_POST, []() {
        String frage = server.arg("plain");
        if (frage.isEmpty()) { server.send(400, "text/plain", "leer"); return; }
        digitalWrite(LED, LOW);
        String antwort = api_fragen(frage);
        digitalWrite(LED, HIGH);
        server.send(200, "text/plain; charset=utf-8", antwort);
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

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    wlan_verbinden();

    ArduinoOTA.setHostname("ESP32-CAM-" BOARD_NR);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
    logln("OTA bereit");

    server_einrichten();
    logln("Bereit.");
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

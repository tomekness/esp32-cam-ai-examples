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

#define LOG_PORT      1234   // nc ESP32-CAM-01.local 1234  auf dem Laptop
// ──────────────────────────────────────────────────────────────────────────────

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "lwip/dns.h"

#define LED 33

WiFiMulti   wifiMulti;
WiFiServer  logServer(LOG_PORT);
WiFiClient  logClient;

// ── Log: Serial + TCP-Client (wenn verbunden) ─────────────────────────────────
void logln(const String &msg) {
    Serial.println(msg);
    if (logServer.hasClient()) {
        if (logClient) logClient.stop();
        logClient = logServer.available();
    }
    if (logClient && logClient.connected())
        logClient.println(msg);
}

// ── WiFi verbinden (AP immer an + STA für externes Netz) ─────────────────────
void wlan_verbinden() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    logln(String("AP aktiv:  ") + AP_SSID + "  →  " + WiFi.softAPIP().toString());

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

// ── OTA einrichten ────────────────────────────────────────────────────────────
void ota_einrichten() {
    ArduinoOTA.setHostname("ESP32-CAM-" BOARD_NR);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
    logln("OTA bereit  →  ESP32-CAM-" BOARD_NR);
}

// ── API-Call ──────────────────────────────────────────────────────────────────
String api_fragen(const String &frage) {
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

    DynamicJsonDocument anfrage(512);
    anfrage["model"] = MODEL;
    JsonArray nachrichten = anfrage.createNestedArray("messages");
    JsonObject msg = nachrichten.createNestedObject();
    msg["role"]    = "user";
    msg["content"] = frage;

    String body;
    serializeJson(anfrage, body);

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

    DynamicJsonDocument antwort(2048);
    deserializeJson(antwort, http.getString());
    http.end();

    return antwort["choices"][0]["message"]["content"].as<String>();
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    wlan_verbinden();
    ota_einrichten();

    logln("\nFrage API...");
    digitalWrite(LED, LOW);

    String antwort = api_fragen("Sag Hallo auf Deutsch in einem Satz.");
    if (antwort.length() > 0) {
        logln("── Antwort ───────────────────");
        logln(antwort);
        logln("──────────────────────────────\n");
    }

    digitalWrite(LED, HIGH);
    logln("Bereit. Enter im Serial Monitor für neue Anfrage.");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    ArduinoOTA.handle();

    if (logServer.hasClient()) {
        if (logClient) logClient.stop();
        logClient = logServer.available();
    }

    if (Serial.available()) {
        while (Serial.available()) Serial.read();

        digitalWrite(LED, LOW);
        logln("\nFrage API...");

        String antwort = api_fragen("Sag mir einen kurzen Witz.");
        if (antwort.length() > 0) {
            logln("── Antwort ───────────────────");
            logln(antwort);
            logln("──────────────────────────────\n");
        }

        digitalWrite(LED, HIGH);
    }
}

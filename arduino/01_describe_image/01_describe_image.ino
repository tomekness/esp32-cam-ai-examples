// ── Konfiguration ────────────────────────────────────────────────
#define WIFI_SSID      "dein-netzwerk"
#define WIFI_PASSWORD  "dein-passwort"
#define API_URL        "http://192.168.1.100:4000/chat/completions"
#define API_KEY        "dummy"
#define MODELL_NAME    "qwen3-vl-30b-instruct"
#define PROMPT         "Beschreibe kurz was du auf dem Bild siehst."
// ─────────────────────────────────────────────────────────────────

#include "esp_camera.h"
#include "mbedtls/base64.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Kamera-Pins XIAO ESP32S3 Sense
#define XCLK_PIN 10
#define SIOD_PIN 40
#define SIOC_PIN 39
#define Y2_PIN   15
#define Y3_PIN   17
#define Y4_PIN   18
#define Y5_PIN   16
#define Y6_PIN   14
#define Y7_PIN   12
#define Y8_PIN   11
#define Y9_PIN   48
#define VSYNC_PIN 38
#define HREF_PIN  47
#define PCLK_PIN  13

void wlan_verbinden() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("WLAN verbinden");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" verbunden.");
}

void kamera_starten() {
    camera_config_t cfg = {};
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.pin_d0 = Y2_PIN;  cfg.pin_d1 = Y3_PIN;
    cfg.pin_d2 = Y4_PIN;  cfg.pin_d3 = Y5_PIN;
    cfg.pin_d4 = Y6_PIN;  cfg.pin_d5 = Y7_PIN;
    cfg.pin_d6 = Y8_PIN;  cfg.pin_d7 = Y9_PIN;
    cfg.pin_xclk     = XCLK_PIN;
    cfg.pin_pclk     = PCLK_PIN;
    cfg.pin_vsync    = VSYNC_PIN;
    cfg.pin_href     = HREF_PIN;
    cfg.pin_sccb_sda = SIOD_PIN;
    cfg.pin_sccb_scl = SIOC_PIN;
    cfg.pin_pwdn     = -1;
    cfg.pin_reset    = -1;
    cfg.xclk_freq_hz = 20000000;
    cfg.pixel_format = PIXFORMAT_JPEG;
    cfg.frame_size   = FRAMESIZE_QVGA;
    cfg.jpeg_quality = 12;
    cfg.fb_count     = 1;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;
    esp_camera_init(&cfg);
}

String bild_aufnehmen() {
    camera_fb_t *bild = esp_camera_fb_get();
    size_t laenge;
    mbedtls_base64_encode(nullptr, 0, &laenge, bild->buf, bild->len);
    uint8_t *puffer = (uint8_t *)ps_malloc(laenge + 1);
    mbedtls_base64_encode(puffer, laenge + 1, &laenge, bild->buf, bild->len);
    puffer[laenge] = '\0';
    esp_camera_fb_return(bild);
    String ergebnis = String((char *)puffer);
    free(puffer);
    return ergebnis;
}

String vlm_fragen(const String &bild_base64, const String &prompt) {
    DynamicJsonDocument anfrage(512);
    anfrage["model"] = MODELL_NAME;
    JsonArray nachrichten = anfrage.createNestedArray("messages");
    JsonObject nachricht  = nachrichten.createNestedObject();
    nachricht["role"]     = "user";
    JsonArray inhalt      = nachricht.createNestedArray("content");
    JsonObject bild_teil  = inhalt.createNestedObject();
    bild_teil["type"]     = "image_url";
    bild_teil.createNestedObject("image_url")["url"] =
        "data:image/jpeg;base64," + bild_base64;
    JsonObject text_teil  = inhalt.createNestedObject();
    text_teil["type"]     = "text";
    text_teil["text"]     = prompt;
    anfrage["max_tokens"] = 300;

    String body;
    serializeJson(anfrage, body);

    HTTPClient http;
    http.begin(API_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " API_KEY);
    http.setTimeout(30000);
    int code = http.POST(body);

    if (code != 200) {
        http.end();
        return "Fehler: HTTP " + String(code);
    }
    DynamicJsonDocument antwort(2048);
    deserializeJson(antwort, http.getString());
    http.end();
    return antwort["choices"][0]["message"]["content"].as<String>();
}

// ── Sketch-Logik ─────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    wlan_verbinden();
    kamera_starten();
    Serial.println("Bereit. Enter drücken für Aufnahme.");
}

void loop() {
    if (!Serial.available()) return;
    Serial.read();

    Serial.println("Nehme Bild auf...");
    String bild_base64 = bild_aufnehmen();

    Serial.println("Frage Modell...");
    String antwort = vlm_fragen(bild_base64, PROMPT);

    Serial.println("\n── Antwort ──────────────────");
    Serial.println(antwort);
    Serial.println("─────────────────────────────\n");
}

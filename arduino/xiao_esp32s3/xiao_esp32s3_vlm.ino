#include "config.h"
#include "esp_camera.h"
#include "mbedtls/base64.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Kamera-Pins XIAO ESP32S3 Sense
#define PWDN_GPIO_NUM   -1
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   10
#define SIOD_GPIO_NUM   40
#define SIOC_GPIO_NUM   39
#define Y9_GPIO_NUM     48
#define Y8_GPIO_NUM     11
#define Y7_GPIO_NUM     12
#define Y6_GPIO_NUM     14
#define Y5_GPIO_NUM     16
#define Y4_GPIO_NUM     18
#define Y3_GPIO_NUM     17
#define Y2_GPIO_NUM     15
#define VSYNC_GPIO_NUM  38
#define HREF_GPIO_NUM   47
#define PCLK_GPIO_NUM   13

void initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size   = CAM_FRAMESIZE;
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_PSRAM;

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Kamera-Init fehlgeschlagen");
        while (true) delay(1000);
    }
}

String captureAndEncode() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Kein Bild erhalten");
        return "";
    }

    size_t encoded_len;
    // mbedtls gibt 0 zurück um die benötigte Puffergröße zu ermitteln
    mbedtls_base64_encode(nullptr, 0, &encoded_len, fb->buf, fb->len);

    uint8_t *encoded = (uint8_t *)ps_malloc(encoded_len + 1);
    if (!encoded) {
        Serial.println("Kein PSRAM für Base64");
        esp_camera_fb_return(fb);
        return "";
    }

    mbedtls_base64_encode(encoded, encoded_len + 1, &encoded_len, fb->buf, fb->len);
    encoded[encoded_len] = '\0';
    esp_camera_fb_return(fb);

    String result = String((char *)encoded);
    free(encoded);
    return result;
}

String queryVLM(const String &b64image) {
    String url = String(USE_HTTPS ? "https" : "http") + "://" +
                 API_HOST + ":" + API_PORT + API_PATH;

    // JSON-Body bauen
    // StaticJsonDocument reicht nicht für das Base64-Bild – DynamicJsonDocument nutzen
    DynamicJsonDocument doc(512);
    doc["model"] = MODEL_NAME;
    JsonArray messages = doc.createNestedArray("messages");
    JsonObject msg = messages.createNestedObject();
    msg["role"] = "user";
    JsonArray content = msg.createNestedArray("content");

    JsonObject imgPart = content.createNestedObject();
    imgPart["type"] = "image_url";
    JsonObject imgUrl = imgPart.createNestedObject("image_url");
    imgUrl["url"] = "data:image/jpeg;base64," + b64image;

    JsonObject textPart = content.createNestedObject();
    textPart["type"] = "text";
    textPart["text"] = PROMPT_TEXT;

    doc["max_tokens"] = 300;

    String body;
    serializeJson(doc, body);

    HTTPClient http;
#if USE_HTTPS
    WiFiClientSecure client;
    client.setInsecure(); // Zertifikat nicht prüfen – für Tests ausreichend
    http.begin(client, url);
#else
    http.begin(url);
#endif

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + API_KEY);
    http.setTimeout(30000);

    int code = http.POST(body);
    if (code != 200) {
        Serial.printf("HTTP Fehler: %d\n", code);
        http.end();
        return "";
    }

    String response = http.getString();
    http.end();

    // Antwort parsen
    DynamicJsonDocument resp(2048);
    deserializeJson(resp, response);
    return resp["choices"][0]["message"]["content"].as<String>();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Verbinde mit WLAN...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nVerbunden: %s\n", WiFi.localIP().toString().c_str());

    initCamera();
    Serial.println("Bereit. Taste drücken (Enter im Serial Monitor) für Aufnahme.");
}

void loop() {
    if (Serial.available()) {
        Serial.read();
        Serial.println("Nehme Bild auf...");

        String b64 = captureAndEncode();
        if (b64.isEmpty()) return;

        Serial.printf("Bild encoded (%d Zeichen), sende an API...\n", b64.length());
        String antwort = queryVLM(b64);

        Serial.println("\n=== Antwort ===");
        Serial.println(antwort);
        Serial.println("===============\n");
    }
}

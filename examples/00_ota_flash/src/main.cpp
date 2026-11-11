#include <WiFi.h>
#include <ArduinoOTA.h>

// ── Board-Nummer anpassen: 01, 02, 03 … ──────────────────────────────────────
#define BOARD_NR     "01"

#define AP_SSID      "ESP32-CAM-" BOARD_NR
#define AP_PASSWORD  "flash1234"
#define OTA_PASSWORD "ota1234"

#define LED 33   // rote LED — LOW = an, HIGH = aus

unsigned long letzter = 0;
bool led_an = false;

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    Serial.printf("\nAP gestartet: %s\n", AP_SSID);
    Serial.printf("AP-IP:        %s\n", WiFi.softAPIP().toString().c_str());

    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
    Serial.println("OTA bereit\n");
}

void loop() {
    ArduinoOTA.handle();

    if (millis() - letzter > 500) {
        letzter = millis();
        led_an = !led_an;
        digitalWrite(LED, led_an ? LOW : HIGH);
    }
}

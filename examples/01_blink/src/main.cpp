#include <WiFi.h>
#include <ArduinoOTA.h>

// ── Board-Nummer anpassen: 01, 02, 03 … ──────────────────────────────────────
#define BOARD_NR     "01"

#define AP_SSID      "ESP32-CAM-" BOARD_NR
#define AP_PASSWORD  "flash1234"
#define OTA_PASSWORD "ota1234"

#define LED_ROT   33   // LOW = an
#define LED_WEISS  4   // HIGH = an (sehr hell!)

void blink(int pin, int an_ms, int aus_ms, int mal) {
    for (int i = 0; i < mal; i++) {
        digitalWrite(pin, pin == LED_ROT ? LOW : HIGH);
        delay(an_ms);
        digitalWrite(pin, pin == LED_ROT ? HIGH : LOW);
        delay(aus_ms);
    }
}

void setup() {
    pinMode(LED_ROT, OUTPUT);
    pinMode(LED_WEISS, OUTPUT);
    digitalWrite(LED_ROT, HIGH);
    digitalWrite(LED_WEISS, LOW);

    blink(LED_WEISS, 100, 50, 1);   // kurz weiß aufblitzen beim Start

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();

    // Doppelblink rot: __ . . ___
    blink(LED_ROT, 150, 100, 2);
    delay(700);
}

#pragma once

// WLAN
#define WIFI_SSID     "dein-netzwerk"
#define WIFI_PASSWORD "dein-passwort"

// API-Endpunkt – eines der beiden aktivieren:

// Option A: lokaler LiteLLM-Stack (kein HTTPS nötig)
#define API_HOST    "192.168.1.100"
#define API_PORT    4000
#define API_PATH    "/chat/completions"
#define API_KEY     "dein-litellm-key"
#define USE_HTTPS   false

// Option B: HuggingFace Inference API (HTTPS)
// #define API_HOST  "api-inference.huggingface.co"
// #define API_PORT  443
// #define API_PATH  "/models/Qwen/Qwen3-VL-30B-A3B-Instruct/v1/chat/completions"
// #define API_KEY   "hf_..."
// #define USE_HTTPS true

// Modell (wird im JSON-Body gesetzt, nur relevant für LiteLLM)
#define MODEL_NAME  "qwen3-vl-30b"

// Prompt der mit jedem Bild geschickt wird
#define PROMPT_TEXT "Beschreibe kurz was du auf dem Bild siehst."

// Bild-Auflösung: FRAMESIZE_QVGA (320x240) ist stabil auf ESP32S3
// Optionen: FRAMESIZE_QQVGA, FRAMESIZE_QVGA, FRAMESIZE_VGA
#define CAM_FRAMESIZE FRAMESIZE_QVGA

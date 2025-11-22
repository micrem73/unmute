#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration - Gestito da WiFiManager (portale captive)
// Non serve più modificare queste impostazioni!
// Al primo avvio, l'Atom Echo creerà un hotspot "Bambola_Setup"
// Connettiti con smartphone e configura WiFi del museo tramite portale web

// WiFiManager Settings
#define WIFI_AP_NAME "Bambola_Setup"           // Nome hotspot per configurazione
#define WIFI_AP_PASSWORD ""                     // Password hotspot (vuoto = aperto)
#define WIFI_CONFIG_TIMEOUT 180                 // Timeout portale config (secondi)
#define WIFI_CONNECT_TIMEOUT 30                 // Timeout connessione WiFi (secondi)

// Opzionale: Fallback WiFi (se configurazione captive fallisce)
// Lascia vuoto per disabilitare fallback
#define WIFI_FALLBACK_SSID ""                   // Es: "MuseoWiFi"
#define WIFI_FALLBACK_PASSWORD ""               // Es: "password123"

// Unmute Server Configuration (Lightning.ai VM)
#define UNMUTE_SERVER_HOST "80-01kadnjreajz4dsa06pdm0hkwq.cloudspaces.litng.ai"
#define UNMUTE_SERVER_PORT 443
#define UNMUTE_SERVER_PATH "/api/v1/realtime"  // ⚠️ IMPORTANTE: /api/v1/realtime (non /v1/realtime!)
#define USE_SSL true  // Set to true for wss://, false for ws://

// Audio Configuration
#define SAMPLE_RATE 24000
#define CHANNELS 1
#define BITS_PER_SAMPLE 16
#define AUDIO_BUFFER_SIZE 960  // 40ms at 24kHz
#define DMA_BUF_COUNT 8
#define DMA_BUF_LEN 1024

// Button Configuration
#define BUTTON_PIN 39  // M5Stack Atom Echo button pin
#define BUTTON_DEBOUNCE_MS 50

// Voice Configuration
#define VOICE_NAME "Anne"

// Debug Configuration
#define DEBUG_SERIAL true
#define DEBUG_WEBSOCKET true
#define DEBUG_AUDIO false

#endif // CONFIG_H

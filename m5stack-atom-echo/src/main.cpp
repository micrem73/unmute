#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <base64.h>
#include "config.h"
#include "OpusEncoder.h"
#include "OpusDecoder.h"

// Global objects
WebSocketsClient webSocket;
WiFiManager wifiManager;
Preferences preferences;  // NVS (Non-Volatile Storage) per dati persistenti
OpusEncoder* opusEncoder = nullptr;
OpusDecoder* opusDecoder = nullptr;

// State variables
bool isConnected = false;
bool isRecording = false;
bool buttonPressed = false;
bool lastButtonState = false;
unsigned long lastDebounceTime = 0;

// Audio buffers
int16_t audioInputBuffer[AUDIO_BUFFER_SIZE];
int16_t audioOutputBuffer[AUDIO_BUFFER_SIZE * 4];
uint8_t opusEncodedBuffer[AUDIO_BUFFER_SIZE * 2];

// Task handles
TaskHandle_t audioInputTaskHandle = NULL;
TaskHandle_t audioOutputTaskHandle = NULL;
QueueHandle_t audioOutputQueue = NULL;

// I2S Configuration for M5Stack Atom Echo
void setupI2S() {
    // I2S input configuration (Microphone)
    i2s_config_t i2s_config_in = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config_in = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = 33,  // M5Atom Echo microphone clock pin
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = 23  // M5Atom Echo microphone data pin
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config_in, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config_in);
    i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    // I2S output configuration (Speaker)
    i2s_config_t i2s_config_out = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config_out = {
        .bck_io_num = 19,  // M5Atom Echo speaker BCK
        .ws_io_num = 33,   // M5Atom Echo speaker WS
        .data_out_num = 22,  // M5Atom Echo speaker DATA
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_1, &i2s_config_out, 4, &audioOutputQueue);
    i2s_set_pin(I2S_NUM_1, &pin_config_out);
    i2s_set_clk(I2S_NUM_1, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

// Initialize Opus encoder/decoder
void setupOpus() {
    int error;

    // Create Opus encoder
    opusEncoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK) {
        Serial.printf("Failed to create Opus encoder: %d\n", error);
        return;
    }

    // Configure encoder
    opus_encoder_ctl(opusEncoder, OPUS_SET_BITRATE(24000));
    opus_encoder_ctl(opusEncoder, OPUS_SET_VBR(1));
    opus_encoder_ctl(opusEncoder, OPUS_SET_COMPLEXITY(5));

    // Create Opus decoder
    opusDecoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &error);
    if (error != OPUS_OK) {
        Serial.printf("Failed to create Opus decoder: %d\n", error);
        return;
    }

    Serial.println("Opus encoder/decoder initialized");
}

// LED color management
void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    M5.dis.drawpix(0, CRGB(r, g, b));
}

void setLEDState(const char* state) {
    if (strcmp(state, "disconnected") == 0) {
        setLEDColor(255, 0, 0);  // Red
    } else if (strcmp(state, "config_mode") == 0) {
        setLEDColor(255, 165, 0);  // Orange - WiFi configuration mode
    } else if (strcmp(state, "connecting") == 0) {
        setLEDColor(255, 255, 255);  // White - Connecting to WiFi
    } else if (strcmp(state, "connected") == 0) {
        setLEDColor(0, 255, 0);  // Green
    } else if (strcmp(state, "listening") == 0) {
        setLEDColor(0, 0, 255);  // Blue
    } else if (strcmp(state, "speaking") == 0) {
        setLEDColor(255, 255, 0);  // Yellow
    } else if (strcmp(state, "buffering") == 0) {
        setLEDColor(255, 0, 255);  // Magenta
    }
}

// Send WebSocket message
void sendWebSocketMessage(const char* type, JsonObject& payload) {
    DynamicJsonDocument doc(4096);
    doc["type"] = type;

    // Copy payload
    for (JsonPair kv : payload) {
        doc[kv.key()] = kv.value();
    }

    String jsonString;
    serializeJson(doc, jsonString);

    if (DEBUG_WEBSOCKET) {
        Serial.printf("TX: %s\n", jsonString.c_str());
    }

    webSocket.sendTXT(jsonString);
}

// Send session update to set voice
void sendSessionUpdate() {
    DynamicJsonDocument doc(1024);
    doc["type"] = "session.update";

    JsonObject session = doc.createNestedObject("session");
    session["voice"] = VOICE_NAME;
    session["instructions"] = "You are a helpful voice assistant.";
    session["turn_detection"] = nullptr;  // Disable auto turn detection

    String jsonString;
    serializeJson(doc, jsonString);

    Serial.printf("Sending session update: %s\n", jsonString.c_str());
    webSocket.sendTXT(jsonString);
}

// Send tirato event
void sendTirato() {
    DynamicJsonDocument doc(256);
    doc["type"] = "unmute.bambola.cordino_tirato";

    String jsonString;
    serializeJson(doc, jsonString);

    Serial.println("Sending: cordino_tirato");
    webSocket.sendTXT(jsonString);
    setLEDState("buffering");
}

// Send rilasciato event
void sendRilasciato() {
    DynamicJsonDocument doc(256);
    doc["type"] = "unmute.bambola.cordino_rilasciato";

    String jsonString;
    serializeJson(doc, jsonString);

    Serial.println("Sending: cordino_rilasciato");
    webSocket.sendTXT(jsonString);
    setLEDState("speaking");
}

// Send audio buffer to server
void sendAudioBuffer(int16_t* buffer, size_t samples) {
    if (!isConnected || !isRecording) return;

    // Encode with Opus
    int encodedBytes = opus_encode(opusEncoder, buffer, samples,
                                    opusEncodedBuffer, sizeof(opusEncodedBuffer));

    if (encodedBytes < 0) {
        Serial.printf("Opus encoding error: %d\n", encodedBytes);
        return;
    }

    // Base64 encode
    String base64Audio = base64::encode(opusEncodedBuffer, encodedBytes);

    // Create message
    DynamicJsonDocument doc(base64Audio.length() + 256);
    doc["type"] = "input_audio_buffer.append";
    doc["audio"] = base64Audio;

    String jsonString;
    serializeJson(doc, jsonString);

    webSocket.sendTXT(jsonString);

    if (DEBUG_AUDIO) {
        Serial.printf("Sent audio: %d samples, %d encoded bytes\n", samples, encodedBytes);
    }
}

// Audio input task - captures microphone and sends to server
void audioInputTask(void* parameter) {
    size_t bytesRead = 0;

    while (true) {
        if (isRecording) {
            // Read from microphone
            i2s_read(I2S_NUM_0, audioInputBuffer, sizeof(audioInputBuffer), &bytesRead, portMAX_DELAY);

            if (bytesRead > 0) {
                size_t samples = bytesRead / sizeof(int16_t);
                sendAudioBuffer(audioInputBuffer, samples);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// Audio output task - plays received audio
void audioOutputTask(void* parameter) {
    int16_t* audioBuffer;
    size_t bytesToWrite;
    size_t bytesWritten;

    while (true) {
        // Wait for audio data from queue
        if (xQueueReceive(audioOutputQueue, &audioBuffer, portMAX_DELAY)) {
            // Write to speaker
            i2s_write(I2S_NUM_1, audioBuffer, AUDIO_BUFFER_SIZE * sizeof(int16_t),
                     &bytesWritten, portMAX_DELAY);

            // Free the buffer
            free(audioBuffer);
        }
    }
}

// Decode and play received Opus audio
void playOpusAudio(const char* base64Audio) {
    // Decode base64
    String decoded = base64::decode(base64Audio);

    // Decode Opus
    int decodedSamples = opus_decode(opusDecoder,
                                     (const unsigned char*)decoded.c_str(),
                                     decoded.length(),
                                     audioOutputBuffer,
                                     AUDIO_BUFFER_SIZE * 4,
                                     0);

    if (decodedSamples < 0) {
        Serial.printf("Opus decoding error: %d\n", decodedSamples);
        return;
    }

    // Allocate buffer for output queue
    int16_t* buffer = (int16_t*)malloc(decodedSamples * sizeof(int16_t));
    if (buffer) {
        memcpy(buffer, audioOutputBuffer, decodedSamples * sizeof(int16_t));
        xQueueSend(audioOutputQueue, &buffer, 0);
    }

    if (DEBUG_AUDIO) {
        Serial.printf("Decoded audio: %d samples\n", decodedSamples);
    }
}

// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("WebSocket Disconnected");
            isConnected = false;
            isRecording = false;
            setLEDState("disconnected");
            break;

        case WStype_CONNECTED:
            Serial.printf("WebSocket Connected to: %s\n", payload);
            isConnected = true;
            setLEDState("connected");

            // Send session update to set voice
            delay(100);
            sendSessionUpdate();
            break;

        case WStype_TEXT: {
            if (DEBUG_WEBSOCKET) {
                Serial.printf("RX: %.*s\n", length, payload);
            }

            // Parse JSON message
            DynamicJsonDocument doc(8192);
            DeserializationError error = deserializeJson(doc, payload, length);

            if (error) {
                Serial.printf("JSON parse error: %s\n", error.c_str());
                break;
            }

            const char* eventType = doc["type"];

            if (strcmp(eventType, "session.updated") == 0) {
                Serial.println("Session updated successfully");
                setLEDState("listening");
            }
            else if (strcmp(eventType, "response.audio.delta") == 0) {
                const char* audioData = doc["delta"];
                if (audioData) {
                    playOpusAudio(audioData);
                }
            }
            else if (strcmp(eventType, "unmute.bambola.buffer_ready") == 0) {
                Serial.println("Buffer ready!");
                setLEDState("buffering");
            }
            else if (strcmp(eventType, "unmute.bambola.playback_started") == 0) {
                Serial.println("Playback started");
                setLEDState("speaking");
            }
            else if (strcmp(eventType, "unmute.bambola.playback_completed") == 0) {
                Serial.println("Playback completed");
                setLEDState("listening");
            }
            else if (strcmp(eventType, "response.text.delta") == 0) {
                const char* textDelta = doc["delta"];
                if (textDelta && DEBUG_SERIAL) {
                    Serial.printf("Text: %s", textDelta);
                }
            }
            else if (strcmp(eventType, "error") == 0) {
                const char* errorMsg = doc["error"]["message"];
                Serial.printf("Error from server: %s\n", errorMsg ? errorMsg : "unknown");
            }
            break;
        }

        case WStype_BIN:
            Serial.printf("Received binary data: %u bytes\n", length);
            break;

        case WStype_ERROR:
            Serial.printf("WebSocket Error\n");
            break;

        case WStype_PING:
            Serial.println("WebSocket Ping");
            break;

        case WStype_PONG:
            Serial.println("WebSocket Pong");
            break;
    }
}

// Button handling
void handleButton() {
    bool currentButtonState = (digitalRead(BUTTON_PIN) == LOW);  // Active low

    // Debounce
    if (currentButtonState != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
        if (currentButtonState != buttonPressed) {
            buttonPressed = currentButtonState;

            if (buttonPressed) {
                // Button pressed - send tirato
                Serial.println("Button PRESSED");
                sendTirato();
                isRecording = true;
            } else {
                // Button released - send rilasciato
                Serial.println("Button RELEASED");
                isRecording = false;
                sendRilasciato();
            }
        }
    }

    lastButtonState = currentButtonState;
}

// ===================================================================
// NVS (Non-Volatile Storage) Management
// ===================================================================

// Verifica stato NVS e mostra info
void checkNVSStatus() {
    Serial.println("\n========================================");
    Serial.println("📦 VERIFICA NVS (Non-Volatile Storage)");
    Serial.println("========================================");

    // Info NVS generali
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
    if (err == ESP_OK) {
        Serial.printf("NVS Entries:\n");
        Serial.printf("  Used:  %d\n", nvs_stats.used_entries);
        Serial.printf("  Free:  %d\n", nvs_stats.free_entries);
        Serial.printf("  Total: %d\n", nvs_stats.total_entries);
        Serial.printf("  Namespace count: %d\n", nvs_stats.namespace_count);
    } else {
        Serial.printf("⚠️  Errore lettura stats NVS: %d\n", err);
    }

    // Verifica credenziali WiFi salvate in NVS
    preferences.begin("wifi.sta", true);  // Read-only
    String ssid = preferences.getString("ssid", "");
    preferences.end();

    Serial.println("\n📡 Credenziali WiFi in NVS:");
    if (ssid.length() > 0) {
        Serial.printf("  ✅ SSID salvato: %s\n", ssid.c_str());
        Serial.printf("  ✅ Password salvata: *** (nascosta)\n");
        Serial.println("  📍 Location: NVS Partition 'nvs' → Namespace 'wifi.sta'");
        Serial.println("  💾 Tipo storage: FLASH non-volatile (permanente)");
    } else {
        Serial.println("  ❌ Nessuna credenziale WiFi salvata");
        Serial.println("  ℹ️  Verrà creato hotspot per configurazione");
    }

    // Info partizioni flash
    Serial.println("\n💾 Info Flash ESP32:");
    Serial.printf("  Flash size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("  Flash speed: %d MHz\n", ESP.getFlashChipSpeed() / 1000000);

    Serial.println("========================================\n");
}

// Salva dati custom in NVS (esempio: configurazioni dispositivo)
void saveToNVS(const char* key, const char* value) {
    preferences.begin("bambola", false);  // Read-write
    preferences.putString(key, value);
    preferences.end();
    Serial.printf("💾 NVS: Salvato '%s' = '%s'\n", key, value);
}

String loadFromNVS(const char* key, const char* defaultValue = "") {
    preferences.begin("bambola", true);  // Read-only
    String value = preferences.getString(key, defaultValue);
    preferences.end();
    return value;
}

// Reset completo NVS (solo per debug!)
void eraseNVS() {
    Serial.println("🗑️  Cancellazione completa NVS...");
    nvs_flash_erase();
    nvs_flash_init();
    Serial.println("✅ NVS cancellato e reinizializzato!");
}

// Mostra tutte le info WiFi salvate
void printWiFiCredentials() {
    Serial.println("\n========================================");
    Serial.println("📡 CREDENZIALI WIFI SALVATE IN NVS");
    Serial.println("========================================");

    // Le credenziali WiFi sono salvate da WiFi.begin() nella partizione NVS
    // Namespace: "wifi.sta" (gestito automaticamente da ESP32 WiFi library)

    preferences.begin("wifi.sta", true);  // Read-only

    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");

    if (ssid.length() > 0) {
        Serial.printf("SSID:     %s\n", ssid.c_str());
        Serial.printf("Password: ");
        for (int i = 0; i < password.length(); i++) {
            Serial.print("*");
        }
        Serial.println();
        Serial.println("\n✅ Credenziali trovate in NVS!");
        Serial.println("📍 Namespace: 'wifi.sta'");
        Serial.println("📍 Partizione: 'nvs' (flash non-volatile)");
        Serial.println("💾 Persistenza: PERMANENTE (sopravvive a reset/power-off)");
    } else {
        Serial.println("❌ Nessuna credenziale WiFi trovata in NVS");
    }

    preferences.end();
    Serial.println("========================================\n");
}

// WiFiManager callbacks
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("========================================");
    Serial.println("🔧 MODALITÀ CONFIGURAZIONE ATTIVA");
    Serial.println("========================================");
    Serial.printf("Hotspot: %s\n", WIFI_AP_NAME);
    Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.println("");
    Serial.println("📱 ISTRUZIONI:");
    Serial.println("1. Connetti smartphone a WiFi: " WIFI_AP_NAME);
    Serial.println("2. Apri browser (portale si apre automaticamente)");
    Serial.println("3. Inserisci SSID e password del museo");
    Serial.println("4. Clicca 'Save'");
    Serial.println("========================================");

    // LED arancione indica modalità configurazione
    setLEDState("config_mode");
}

void saveConfigCallback() {
    Serial.println("\n========================================");
    Serial.println("💾 SALVATAGGIO CREDENZIALI WiFi IN NVS");
    Serial.println("========================================");
    Serial.println("✅ WiFiManager ha salvato le credenziali!");
    Serial.println("📍 Location: Flash ESP32 → Partizione NVS → Namespace 'wifi.sta'");
    Serial.println("💾 Persistenza: PERMANENTE (non volatile)");
    Serial.println("🔄 Le credenziali verranno usate ai prossimi riavvii");
    Serial.println("========================================\n");

    setLEDState("connecting");

    // Salva timestamp ultimo salvataggio (esempio uso custom NVS)
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%lu", millis());
    saveToNVS("last_wifi_save", timestamp);
}

// Blinking LED durante configurazione
void blinkConfigLED() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (millis() - lastBlink > 500) {
        lastBlink = millis();
        ledState = !ledState;
        if (ledState) {
            setLEDState("config_mode");
        } else {
            setLEDColor(0, 0, 0);  // Off
        }
    }
}

// Setup WiFi con WiFiManager e portale captive
void setupWiFi() {
    Serial.println("\n========================================");
    Serial.println("📡 Inizializzazione WiFi Manager");
    Serial.println("========================================");

    // Reset LED
    setLEDState("disconnected");

    // Configurazione WiFiManager
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setConfigPortalTimeout(WIFI_CONFIG_TIMEOUT);
    wifiManager.setConnectTimeout(WIFI_CONNECT_TIMEOUT);

    // Info debug
    wifiManager.setDebugOutput(DEBUG_SERIAL);

    // Parametri personalizzati per portale web
    WiFiManagerParameter custom_text(
        "<p><b>Bambola Parlante - Configurazione WiFi</b></p>"
        "<p>Inserisci le credenziali del WiFi del museo:</p>"
    );
    wifiManager.addParameter(&custom_text);

    // LED lampeggiante durante configurazione
    wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
        configModeCallback(myWiFiManager);
    });

    Serial.println("Tentativo di connessione WiFi...");
    setLEDState("connecting");

    // Prova a connettersi con credenziali salvate
    // Se fallisce, crea hotspot "Bambola_Setup" con portale captive
    bool connected = false;

    if (strlen(WIFI_AP_PASSWORD) > 0) {
        // Hotspot con password
        connected = wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD);
    } else {
        // Hotspot aperto (senza password)
        connected = wifiManager.autoConnect(WIFI_AP_NAME);
    }

    if (connected) {
        Serial.println("\n========================================");
        Serial.println("✅ WiFi CONNESSO!");
        Serial.println("========================================");
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
        Serial.println("========================================\n");
        setLEDState("connected");

        // Mostra info credenziali salvate in NVS
        printWiFiCredentials();
    } else {
        Serial.println("\n========================================");
        Serial.println("❌ Connessione WiFi fallita!");
        Serial.println("========================================");

        // Prova fallback WiFi se configurato
        if (strlen(WIFI_FALLBACK_SSID) > 0) {
            Serial.printf("Tentativo fallback: %s\n", WIFI_FALLBACK_SSID);
            WiFi.begin(WIFI_FALLBACK_SSID, WIFI_FALLBACK_PASSWORD);

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                Serial.print(".");
                attempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf("\n✅ Connesso a fallback WiFi: %s\n", WIFI_FALLBACK_SSID);
                Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
                setLEDState("connected");
            } else {
                Serial.println("\n❌ Fallback WiFi fallito!");
                setLEDState("disconnected");
            }
        } else {
            setLEDState("disconnected");
        }
    }

    Serial.println("");
}

// Reset configurazione WiFi (tieni premuto bottone all'avvio)
void checkWiFiReset() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("\n========================================");
        Serial.println("🔄 RESET CONFIGURAZIONE WIFI");
        Serial.println("========================================");
        Serial.println("Rilascia il bottone per confermare reset...");

        unsigned long pressStart = millis();
        while (digitalRead(BUTTON_PIN) == LOW && millis() - pressStart < 3000) {
            // Blink rosso veloce
            setLEDColor(255, 0, 0);
            delay(100);
            setLEDColor(0, 0, 0);
            delay(100);
        }

        if (millis() - pressStart >= 3000) {
            Serial.println("✅ Reset confermato! Cancellazione credenziali...");
            wifiManager.resetSettings();
            delay(1000);
            Serial.println("🔄 Riavvio...");
            ESP.restart();
        } else {
            Serial.println("❌ Reset annullato");
        }
    }
}

// Setup WebSocket connection
void setupWebSocket() {
    // Configure WebSocket
    if (USE_SSL) {
        webSocket.beginSSL(UNMUTE_SERVER_HOST, UNMUTE_SERVER_PORT, UNMUTE_SERVER_PATH);
    } else {
        webSocket.begin(UNMUTE_SERVER_HOST, UNMUTE_SERVER_PORT, UNMUTE_SERVER_PATH);
    }

    // Set subprotocol (required by OpenAI Realtime API)
    webSocket.setSubProtocol("realtime");

    // Set event handler
    webSocket.onEvent(webSocketEvent);

    // Set reconnect interval
    webSocket.setReconnectInterval(5000);

    Serial.println("WebSocket configured");
}

void setup() {
    // Initialize M5Atom
    M5.begin(true, false, true);  // Serial, I2C, Display

    Serial.begin(115200);
    delay(100);  // Stabilizzazione seriale

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("🎭 Bambola Parlante - M5Stack Atom Echo");
    Serial.println("========================================");
    Serial.println("Versione: 1.2 (NVS Storage Edition)");
    Serial.println("========================================\n");

    // Setup button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Initialize LED
    setLEDState("disconnected");

    // Verifica stato NVS e credenziali salvate
    checkNVSStatus();

    // Check per reset WiFi (tieni premuto bottone all'avvio)
    Serial.println("💡 SUGGERIMENTO: Tieni premuto il bottone per 3 secondi");
    Serial.println("   per resettare la configurazione WiFi\n");
    delay(2000);  // Tempo per premere bottone se necessario
    checkWiFiReset();

    // Setup I2S for audio
    Serial.println("🎤 Inizializzazione I2S...");
    setupI2S();

    // Setup Opus codec
    Serial.println("🔊 Inizializzazione codec Opus...");
    setupOpus();

    // Create audio output queue
    audioOutputQueue = xQueueCreate(10, sizeof(int16_t*));

    // Setup WiFi con WiFiManager
    setupWiFi();

    // Setup WebSocket
    setupWebSocket();

    // Create audio tasks
    xTaskCreatePinnedToCore(
        audioInputTask,
        "AudioInput",
        4096,
        NULL,
        2,
        &audioInputTaskHandle,
        0
    );

    xTaskCreatePinnedToCore(
        audioOutputTask,
        "AudioOutput",
        4096,
        NULL,
        2,
        &audioOutputTaskHandle,
        1
    );

    Serial.println("Setup complete!");
}

void loop() {
    // Handle WebSocket
    webSocket.loop();

    // Handle button
    handleButton();

    // Update M5Atom
    M5.update();

    // Small delay
    delay(10);
}

# Quick Start Guide

Get your M5Stack Atom Echo connected to Unmute in **3 minutes** (senza modificare codice)!

## ✨ NOVITÀ: WiFi Manager Edition

**Non serve più modificare il codice per configurare WiFi!**
Tutto si configura tramite smartphone con portale web automatico.

## Prerequisites

- ✅ M5Stack Atom Echo device
- ✅ USB-C cable
- ✅ WiFi network (2.4GHz) del museo
- ✅ Smartphone o tablet
- ✅ Lightning.ai account with Unmute running
- ✅ PlatformIO installed (or Arduino IDE)

## Step 1: Get Your Server URL

1. Go to your Lightning.ai dashboard
2. Navigate to your Studio running Unmute
3. Copy the public URL (e.g., `https://abc123.lightning.ai`)
4. Test it works: Open browser to `https://abc123.lightning.ai/v1/health`
   - Should return JSON with `"ok": true`

## Step 2: Configure Server (Solo 1 Parametro!)

1. Open `src/config.h`

2. Update **SOLO** server URL (WiFi si configura dopo via smartphone!):
```cpp
#define UNMUTE_SERVER_HOST "abc123.lightning.ai"  // NO https://
#define UNMUTE_SERVER_PORT 443
#define USE_SSL true
```

**Important**:
- Remove `https://` from the hostname!
- **NON configurare WiFi qui** - si fa dopo tramite portale captive!

## Step 3: Flash the Device

### Using PlatformIO (Recommended)

```bash
cd m5stack-atom-echo

# Install dependencies and build
pio run

# Connect M5Stack Atom Echo via USB-C
# Flash to device
pio run --target upload

# Open serial monitor to see output
pio device monitor
```

### Using Arduino IDE

1. Install Arduino IDE
2. Add ESP32 board support: File → Preferences → Additional Board URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Install board: Tools → Board → Boards Manager → Search "ESP32" → Install
4. Install libraries: Sketch → Include Library → Manage Libraries
   - Search and install: M5Atom, ArduinoJson, WebSockets
5. Open `src/main.cpp`
6. Select: Tools → Board → M5Stack-ATOM
7. Select: Tools → Port → (your device port)
8. Click Upload ▶

## Step 4: Configure WiFi via Smartphone

### 🟠 Modalità Configurazione (LED Arancione Lampeggiante)

1. **Il dispositivo crea hotspot**:
   - Nome: `Bambola_Setup`
   - Password: nessuna (aperto)
   - LED: 🟠 arancione lampeggiante

2. **Connetti smartphone**:
   - Apri impostazioni WiFi
   - Connetti a: `Bambola_Setup`
   - Portale web si apre **automaticamente**
   - Se non si apre: vai a `http://192.168.4.1`

3. **Configura WiFi museo**:
   - **Lista automatica**: Seleziona WiFi dalla lista
   - **O manuale**: Inserisci SSID e password
   - Clicca **"Save"**

4. **Verifica connessione**:
   - LED diventa ⚪ bianco (connessione...)
   - Poi 🟢 verde (WiFi connesso!)
   - Infine 🔵 blu (WebSocket pronto!)

**⏱️ Timeout**: Se non configuri entro 3 minuti, riavvia dispositivo.

**🔄 Sbagliato WiFi?** Tieni premuto bottone 3s all'avvio → reset WiFi

### 📖 Guida Dettagliata

Per istruzioni complete: [WIFI_SETUP.md](WIFI_SETUP.md)

## Step 5: Test the Connection

1. **Watch Serial Monitor** (115200 baud):
```
========================================
🎭 Bambola Parlante - M5Stack Atom Echo
========================================
Versione: 1.1 (WiFi Manager Edition)
========================================

🎤 Inizializzazione I2S...
🔊 Inizializzazione codec Opus...

========================================
📡 Inizializzazione WiFi Manager
========================================

✅ WiFi CONNESSO!
========================================
SSID: MuseoWiFi
IP: 192.168.1.100
Signal: -45 dBm
========================================

WebSocket Connected to: abc123.lightning.ai
Session updated successfully
```

2. **Check LED Color**:
   - 🔴 Red = Error / Disconnected
   - 🟠 Orange blinking = WiFi config mode
   - 🟢 Green = WiFi connected
   - 🔵 Blue = Ready and listening ✅

## Step 5: First Conversation

1. **Speak your question**:
   ```
   "What is the weather today?"
   ```
   - Your speech is transcribed by Whisper

2. **Press the button**:
   - LED turns 🟣 Magenta
   - Bot generates response and buffers audio

3. **Release the button**:
   - LED turns 🟡 Yellow
   - Bot response plays through speaker

4. **Listen to response**:
   - LED returns to 🔵 Blue when done
   - Ready for next interaction!

## Troubleshooting

### ❌ LED Arancione Non Appare (No WiFi Config Mode)

**Soluzioni:**
1. Attendi 30 secondi dopo accensione
2. Controlla log seriale: deve dire "MODALITÀ CONFIGURAZIONE"
3. Fai reset WiFi: tieni premuto bottone 3s all'avvio
4. Riflasha dispositivo

### ❌ Portale Captive Non Si Apre

**Soluzioni:**
1. Vai manualmente a: `http://192.168.4.1`
2. Prova con browser diverso (Chrome, Safari, Firefox)
3. Alcuni Android non aprono portali automaticamente
4. Disattiva dati cellulare durante configurazione

### ❌ WiFi Configurato Ma Non Connette

**Soluzioni:**
1. **Password sbagliata** (case-sensitive!)
   - Reset: tieni premuto bottone 3s all'avvio
   - Riconfigura tramite portale

2. **WiFi 5GHz invece di 2.4GHz**
   - ESP32 supporta **SOLO 2.4GHz**
   - Verifica banda WiFi museo

3. **Portale captive museo**
   - Alcuni WiFi pubblici richiedono login aggiuntivo
   - Contatta IT museo per whitelist MAC address

### ❌ Red LED (Can't Connect to Server)

**Server Issues:**
```cpp
// Verify server URL
#define UNMUTE_SERVER_HOST "abc123.lightning.ai"  // ← No https://
#define UNMUTE_SERVER_PORT 443                     // ← 443 for SSL
#define USE_SSL true                               // ← true for wss://
```

**Test server manually:**
```bash
curl https://abc123.lightning.ai/v1/health
```

### ❌ No Audio Input

Serial monitor should show:
```
Sent audio: 960 samples, 124 encoded bytes
```

If not:
1. Check microphone pins in code (GPIO 23, 33)
2. Speak louder
3. Enable `DEBUG_AUDIO true` in config.h

### ❌ No Audio Output

When button is released, should see:
```
Playback started
Decoded audio: 960 samples
```

If not:
1. Check speaker volume
2. Check speaker pins in code (GPIO 19, 22, 33)
3. Enable `DEBUG_AUDIO true` in config.h

### ❌ Button Not Working

Serial monitor should show:
```
Button PRESSED
Sending: cordino_tirato
```

If not:
1. Check button pin: `#define BUTTON_PIN 39`
2. Try external button on different GPIO
3. Reduce debounce: `#define BUTTON_DEBOUNCE_MS 20`

## Usage Tips

### Best Practices

✅ **Speak clearly** - Wait ~1 second after speaking before pressing button
✅ **Press quickly** - Press button right after speaking
✅ **Short press** - Release button immediately (don't hold)
✅ **Good WiFi** - Stay within WiFi range for best performance

### LED Status Reference

| Color | Meaning | Action |
|-------|---------|--------|
| 🔴 Red | Error/Disconnected | Check logs |
| 🟠 Orange (blink) | WiFi config mode | Connect to "Bambola_Setup" |
| ⚪ White | Connecting to WiFi | Wait... |
| 🟢 Green | WiFi connected | Wait for Blue |
| 🔵 Blue | Listening | Speak your question |
| 🟣 Magenta | Generating | Wait for response |
| 🟡 Yellow | Playing | Listen to bot |
| 🔴 Red (fast) | WiFi reset | Hold button 3s |

### Button Workflow

```
1. Speak → 2. Press Button → 3. Release Button → 4. Listen
   👄           👇                  👆                👂
```

## Advanced Configuration

### Change Voice

Edit `config.h`:
```cpp
#define VOICE_NAME "YourVoiceName"  // Must exist in voices.yaml
```

### Adjust Audio Quality

Edit `main.cpp` → `setupOpus()`:
```cpp
opus_encoder_ctl(opusEncoder, OPUS_SET_BITRATE(32000));  // Higher = better
opus_encoder_ctl(opusEncoder, OPUS_SET_COMPLEXITY(8));   // Max = 10
```

### Enable Debug Logging

Edit `config.h`:
```cpp
#define DEBUG_SERIAL true       // General logs
#define DEBUG_WEBSOCKET true    // WebSocket messages
#define DEBUG_AUDIO true        // Audio data (very verbose!)
```

## Next Steps

- ✅ Test different voices (edit `VOICE_NAME` in config.h)
- ✅ Add external button for better ergonomics
- ✅ Customize LED colors in `main.cpp` → `setLEDColor()`
- ✅ Monitor serial output to debug issues
- ✅ Read full README.md for advanced features

## Getting Help

1. Check serial monitor output for errors
2. Verify Unmute backend health: `/v1/health`
3. Test WebSocket manually: `wscat -c wss://your-server/v1/realtime`
4. Review full README.md and PINOUT.md
5. Enable debug logging in config.h

## Common Errors

### `WebSocket Disconnected`
→ Check server URL and SSL settings

### `WiFi connection failed`
→ Check SSID/password, ensure 2.4GHz WiFi

### `Opus encoding error`
→ Check sample rate matches (24000 Hz)

### `JSON parse error`
→ Check WebSocket protocol matches backend

### `No response from server`
→ Verify Unmute backend is running and "Anne" voice exists

---

**Success!** 🎉 Your M5Stack Atom Echo is now a voice assistant powered by Unmute!

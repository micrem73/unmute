# M5Stack Atom Echo - Unmute Client

This project implements a voice assistant client for M5Stack Atom Echo that connects to the Unmute backend running on Lightning.ai.

## Features

- ✅ **WiFi Manager con Portale Captive** - Configurazione WiFi senza modificare codice!
- ✅ **NVS (Non-Volatile Storage)** - Credenziali salvate permanentemente in flash ESP32
- ✅ WebSocket communication with Unmute backend
- ✅ Real-time audio capture and streaming (Opus encoded)
- ✅ Real-time audio playback from bot responses
- ✅ Button control for "cordino tirato/rilasciato" events
- ✅ Visual LED status indicators con feedback configurazione
- ✅ "Anne" voice configuration
- ✅ Automatic reconnection on disconnect
- ✅ NVS verification e debug tools integrati
- ✅ Reset WiFi tramite pulsante (hold 3s all'avvio)

## Hardware Requirements

- **M5Stack Atom Echo** (ESP32-PICO-D4)
  - Built-in microphone (SPM1423)
  - Built-in speaker
  - Built-in RGB LED (WS2812)
  - Built-in button (GPIO 39)

- **External Switch Button** (optional)
  - Connect between GPIO pin and GND
  - If not using external button, the built-in button on Atom Echo can be used

### Wiring for External Button

If you want to use an external switch instead of the built-in button:

```
External Switch:
  Pin 1 → GPIO 39 (or any available GPIO)
  Pin 2 → GND

Note: The code uses INPUT_PULLUP, so the button should connect to GND when pressed.
```

The built-in button on M5Stack Atom Echo is already connected to GPIO 39, so no additional wiring is needed if using the built-in button.

## Software Requirements

- **PlatformIO** (recommended) or Arduino IDE
- **ESP32 Board Support** (version 2.0.0 or higher)

### Required Libraries

These are automatically installed via PlatformIO:
- M5Atom (^0.1.0)
- ArduinoJson (^6.21.3)
- WebSockets (^2.4.1)
- Arduino Audio Tools
- Arduino LibOpus

## Installation & Setup

### 1. Clone or Download the Project

```bash
cd unmute/m5stack-atom-echo
```

### 2. Configure Server Settings

Edit `src/config.h` - **SOLO server URL, WiFi si configura via smartphone!**:

```cpp
// Unmute Server Configuration
#define UNMUTE_SERVER_HOST "your-vm.lightning.ai"  // Your Lightning.ai VM hostname
#define UNMUTE_SERVER_PORT 443                      // 443 for wss://, 8000 for ws://
#define UNMUTE_SERVER_PATH "/v1/realtime"
#define USE_SSL true                                 // true for wss://, false for ws://
```

#### Finding Your Lightning.ai Server URL

1. Log into your Lightning.ai dashboard
2. Navigate to your running VM/Studio
3. Look for the public URL (e.g., `https://abc123.lightning.ai`)
4. Use the hostname without `https://` in `UNMUTE_SERVER_HOST`

Example:
```cpp
#define UNMUTE_SERVER_HOST "abc123.lightning.ai"
#define UNMUTE_SERVER_PORT 443
#define USE_SSL true
```

**⚠️ IMPORTANTE: Non serve più configurare WiFi nel codice!**
Il WiFi si configura tramite smartphone usando il portale captive (vedi Step 4).

### 3. Build and Upload

#### Using PlatformIO (Recommended)

```bash
# Build the project
pio run

# Upload to M5Stack Atom Echo
pio run --target upload

# Monitor serial output
pio device monitor
```

#### Using Arduino IDE

1. Install required libraries via Library Manager
2. Open `src/main.cpp`
3. Select board: **M5Stack-ATOM**
4. Select port
5. Click Upload

### 4. Configure WiFi via Smartphone (Portale Captive)

**Prima accensione o dopo reset:**

1. **Accendi il dispositivo**
   - LED diventa 🟠 Arancione lampeggiante
   - Dispositivo crea hotspot: `"Bambola_Setup"`

2. **Connetti smartphone**
   - Apri WiFi su smartphone
   - Connetti a: `Bambola_Setup` (nessuna password)
   - Portale web si apre **automaticamente**
   - Se non si apre, vai a: `http://192.168.4.1`

3. **Configura WiFi museo**
   - Seleziona WiFi del museo dalla lista
   - Inserisci password
   - Clicca **"Save"**

4. **Verifica connessione**
   - LED diventa 🟢 Verde (connesso)
   - Poi 🔵 Blu (pronto per uso)

**📖 Guida completa:** Vedi [WIFI_SETUP.md](WIFI_SETUP.md)

**🔄 Reset WiFi:** Tieni premuto bottone per 3 secondi all'avvio

### 5. Configure Unmute Backend

Make sure your Unmute backend is running and accessible. The backend should:
- Be running on your Lightning.ai VM
- Have the WebSocket endpoint at `/v1/realtime`
- Have the "Anne" voice configured in `voices.yaml`

## Usage

### LED Status Indicators

| Color | Status |
|-------|--------|
| 🔴 Red | Disconnected from server |
| 🟠 Orange (blinking) | WiFi configuration mode (portale captive attivo) |
| ⚪ White | Connecting to WiFi |
| 🟢 Green | Connected to server |
| 🔵 Blue | Listening for user input |
| 🟡 Yellow | Bot is speaking |
| 🟣 Magenta | Buffering bot response |
| 🔴 Red (fast blink) | WiFi reset in progress |

### Operation Flow

1. **Power On**: Device connects to WiFi and WebSocket server (LED: Red → Green → Blue)

2. **Speaking to the Bot**:
   - Speak your question/command (device is always listening)
   - Whisper STT transcribes your speech

3. **Trigger Bot Response**:
   - **Press the button** (cordino tirato) - LED turns Magenta
   - Bot starts generating response and buffering audio

4. **Play Bot Response**:
   - **Release the button** (cordino rilasciato) - LED turns Yellow
   - Buffered audio plays through speaker
   - LED returns to Blue when complete

### Button Behavior

- **Press**: Sends `unmute.bambola.cordino_tirato` → Starts bot response generation
- **Release**: Sends `unmute.bambola.cordino_rilasciato` → Plays buffered response

### Serial Monitor Output

Connect to serial monitor (115200 baud) to see debug information:

```
M5Stack Atom Echo - Unmute Client
==================================
Setting up I2S...
Setting up Opus codec...
Opus encoder/decoder initialized
Connecting to WiFi: MyNetwork
.....
WiFi connected! IP: 192.168.1.100
WebSocket configured
Setup complete!
WebSocket Connected to: your-vm.lightning.ai
Sending session update: {"type":"session.update","session":{"voice":"Anne"}}
Session updated successfully
Button PRESSED
Sending: cordino_tirato
Buffer ready!
Button RELEASED
Sending: cordino_rilasciato
Playback started
Playback completed
```

## Audio Configuration

The device uses the following audio settings (matching Unmute backend):

- **Sample Rate**: 24000 Hz
- **Channels**: Mono (1 channel)
- **Bit Depth**: 16-bit
- **Codec**: Opus
- **Buffer Size**: 960 samples (40ms at 24kHz)

## Troubleshooting

### WiFi Configuration Issues

**Hotspot "Bambola_Setup" non appare:**
- Attendi 30 secondi dopo accensione
- LED deve essere 🟠 arancione lampeggiante
- Fai scan WiFi manuale su smartphone
- Prova reset: tieni premuto bottone 3s all'avvio

**Portale captive non si apre automaticamente:**
- Vai manualmente a: `http://192.168.4.1`
- Alcuni Android non aprono portali automaticamente
- Prova con browser diverso

**Configurazione salvata ma non si connette:**
- Verifica password WiFi corretta (case-sensitive!)
- Controlla che WiFi museo sia **2.4GHz** (non 5GHz)
- Alcuni WiFi pubblici hanno portale captive aggiuntivo
- Resetta e riprova: tieni premuto bottone 3s all'avvio
- Verifica log seriale per dettagli errore

**Reset completo configurazione WiFi:**
```
1. Spegni dispositivo
2. Tieni premuto bottone
3. Accendi dispositivo (mentre tieni premuto)
4. LED lampeggia rosso veloce
5. Continua a tenere per 3 secondi
6. Reset completato!
```

### WiFi Connection Issues

- Check WiFi signal strength
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Verify museum WiFi doesn't have additional captive portal

### WebSocket Connection Issues

- Verify server URL and port
- Check if Unmute backend is running: `curl http://your-server/v1/health`
- Verify SSL settings match (wss:// needs `USE_SSL true`)
- Check firewall settings on Lightning.ai

### Audio Issues

**No audio input (microphone not working)**:
- Check I2S pins configuration
- Verify microphone is working (check serial monitor for audio data)

**No audio output (speaker not working)**:
- Check I2S speaker pins
- Verify volume is not muted
- Check if playback events are received (serial monitor)

### Button Not Working

- Verify button pin configuration (`BUTTON_PIN` in config.h)
- Check if button is properly connected to GND
- Adjust `BUTTON_DEBOUNCE_MS` if button is too sensitive

### Debug Options

Enable debug output in `config.h`:

```cpp
#define DEBUG_SERIAL true      // General debug messages
#define DEBUG_WEBSOCKET true   // WebSocket message logging
#define DEBUG_AUDIO true       // Audio buffer logging (verbose!)
```

## Advanced Configuration

### Changing the Voice

Edit `config.h`:

```cpp
#define VOICE_NAME "YourVoiceName"
```

Make sure the voice exists in your Unmute backend's `voices.yaml`.

### Adjusting Audio Quality

Edit `main.cpp` in the `setupOpus()` function:

```cpp
opus_encoder_ctl(opusEncoder, OPUS_SET_BITRATE(32000));  // Increase bitrate
opus_encoder_ctl(opusEncoder, OPUS_SET_COMPLEXITY(8));   // Increase complexity (1-10)
```

Higher values = better quality but more CPU usage.

### Using Different GPIO for Button

Edit `config.h`:

```cpp
#define BUTTON_PIN 25  // Change to your desired GPIO
```

## Architecture

### System Flow

```
User Speech → Microphone (I2S) → Opus Encoder → Base64 → WebSocket → Unmute Backend
                                                                            ↓
User Hears ← Speaker (I2S) ← Opus Decoder ← Base64 ← WebSocket ← Bot Response
```

### Tasks

The firmware uses FreeRTOS tasks:

- **Main Loop** (Core 1): WebSocket handling, button handling
- **Audio Input Task** (Core 0): Microphone capture and streaming
- **Audio Output Task** (Core 1): Audio playback from queue

### Memory Usage

- **PSRAM**: Enabled for large audio buffers
- **Heap**: ~200KB used
- **Flash**: ~1.2MB program size

## API Reference

### WebSocket Events Sent

- `session.update` - Set voice and configuration
- `input_audio_buffer.append` - Send audio data
- `unmute.bambola.cordino_tirato` - Button pressed
- `unmute.bambola.cordino_rilasciato` - Button released

### WebSocket Events Received

- `session.updated` - Confirmation of session update
- `response.audio.delta` - Audio data from bot
- `response.text.delta` - Text transcription of bot speech
- `unmute.bambola.buffer_ready` - Bot response buffered
- `unmute.bambola.playback_started` - Playback started
- `unmute.bambola.playback_completed` - Playback finished
- `error` - Error messages

## License

This project is part of the Unmute system. See the main Unmute repository for license information.

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review serial monitor output for error messages
3. Verify Unmute backend is running and accessible
4. Check the main Unmute repository documentation

## Credits

- Built for M5Stack Atom Echo
- Uses Kyutai's Moshi STT/TTS models via Unmute backend
- Opus codec for audio compression
- WebSocket communication following OpenAI Realtime API protocol

# M5Stack Atom Echo Pinout Reference

## Built-in Components

### Microphone (SPM1423)
- **Type**: PDM (Pulse Density Modulation) MEMS Microphone
- **Clock Pin**: GPIO 33
- **Data Pin**: GPIO 23
- **I2S Port**: I2S_NUM_0

### Speaker (NS4168)
- **Type**: I2S Digital Amplifier
- **BCK (Bit Clock)**: GPIO 19
- **WS (Word Select)**: GPIO 33
- **DATA**: GPIO 22
- **I2S Port**: I2S_NUM_1

### Built-in Button
- **GPIO Pin**: 39
- **Type**: Active Low (pressed = LOW, released = HIGH)
- **Internal Pull-up**: Enabled in code

### RGB LED (WS2812)
- **Data Pin**: GPIO 27
- **Type**: NeoPixel compatible
- **Count**: 1 LED (25 LEDs in matrix version)

## Available GPIO Pins

M5Stack Atom Echo has limited available GPIOs due to built-in components:

| GPIO | Function | Available | Notes |
|------|----------|-----------|-------|
| 19 | Speaker BCK | ❌ | Used by speaker |
| 21 | Grove SDA | ✅ | I2C Data (if not using speaker) |
| 22 | Speaker DATA | ❌ | Used by speaker |
| 23 | Mic DATA | ❌ | Used by microphone |
| 25 | Grove SCL | ✅ | I2C Clock (if not using speaker) |
| 26 | - | ✅ | General purpose |
| 27 | LED | ❌ | Used by RGB LED |
| 32 | - | ✅ | General purpose |
| 33 | Mic/Speaker CLK | ❌ | Shared clock |
| 39 | Button | ❌ | Used by button (but can share) |

## External Switch Connection

If you want to add an external switch/button:

### Option 1: Use Built-in Button (GPIO 39)
No additional wiring needed - the code uses GPIO 39 by default.

### Option 2: Use Different GPIO

```
External Switch Wiring:
┌─────────────┐
│   Switch    │
│             │
│   Pin 1 ────┼──── GPIO Pin (e.g., GPIO 26, 32)
│             │
│   Pin 2 ────┼──── GND
└─────────────┘

Note: Code uses INPUT_PULLUP mode
```

**Update config.h:**
```cpp
#define BUTTON_PIN 26  // Change to your GPIO
```

### Option 3: Use Grove Port

The M5Stack Atom Echo has a Grove port (4-pin connector):

```
Grove Port Pinout:
┌─────────────────┐
│ 1. GND          │
│ 2. VCC (5V)     │
│ 3. SDA (GPIO21) │
│ 4. SCL (GPIO25) │
└─────────────────┘
```

You can use GPIO 21 or 25 for a button if you're not using I2C devices.

## I2S Configuration

### I2S_NUM_0 (Microphone)
```cpp
Mode: RX, PDM
Sample Rate: 24000 Hz
Bits per Sample: 16
Channel: Mono (Right channel)
WS Pin: 33
Data In: 23
```

### I2S_NUM_1 (Speaker)
```cpp
Mode: TX
Sample Rate: 24000 Hz
Bits per Sample: 16
Channel: Mono (Right channel)
BCK Pin: 19
WS Pin: 33
Data Out: 22
```

## Power Specifications

- **Input Voltage**: 5V via USB-C
- **Operating Current**:
  - Idle: ~80mA
  - Recording: ~120mA
  - Playing: ~150mA
  - Peak (recording + WiFi): ~200mA
- **Speaker Output**: 2W max

## Hardware Modifications

### Adding External Antenna

M5Stack Atom Echo has a U.FL connector for external WiFi antenna:
1. Locate U.FL connector on PCB
2. Remove 0Ω resistor (if present) to disable PCB antenna
3. Connect U.FL antenna

This can improve WiFi range and stability.

### Disabling Speaker/Microphone

If you want to use different audio hardware:

**Disable Speaker:**
- Comment out `i2s_driver_install(I2S_NUM_1, ...)` in code
- Free up GPIOs 19, 22, 33

**Disable Microphone:**
- Comment out `i2s_driver_install(I2S_NUM_0, ...)` in code
- Free up GPIOs 23, 33

## Schematic

```
                    ┌─────────────────────────┐
                    │   M5Stack Atom Echo     │
                    │     ESP32-PICO-D4       │
                    │                         │
  USB-C ───────────►│ 5V                      │
                    │ GND                     │
                    │                         │
  Microphone        │                         │
  (SPM1423)         │  GPIO33 ◄───┐           │
      CLK ◄─────────┤              │           │
      DATA ─────────┤► GPIO23      │           │
                    │              │           │
  Speaker           │              │           │
  (NS4168)          │              └───────────┤ Shared CLK
      BCK ◄─────────┤  GPIO19                  │
      WS ◄──────────┤  GPIO33                  │
      DATA ◄────────┤  GPIO22                  │
                    │                         │
  RGB LED           │                         │
  (WS2812)          │                         │
      DIN ◄─────────┤  GPIO27                  │
                    │                         │
  Button            │                         │
      SW ───────────┤  GPIO39 (INPUT_PULLUP)  │
      GND ──────────┤  GND                    │
                    │                         │
  External Switch   │                         │
  (Optional)        │                         │
      Pin1 ─────────┤  GPIO26/32 (configurable)│
      Pin2 ─────────┤  GND                    │
                    │                         │
                    └─────────────────────────┘
```

## Debugging Tips

### Serial Monitor Pins
- **TX**: GPIO 1 (USB Serial)
- **RX**: GPIO 3 (USB Serial)
- **Baud Rate**: 115200

### JTAG Debugging
M5Stack Atom Echo doesn't expose JTAG pins, use Serial debugging instead.

## References

- [M5Stack Atom Echo Documentation](https://docs.m5stack.com/en/atom/atom_echo)
- [ESP32-PICO-D4 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf)
- [SPM1423 Microphone Datasheet](https://www.knowles.com/docs/default-source/model-downloads/spm1423hm4h-b-datasheet-v1-0.pdf)

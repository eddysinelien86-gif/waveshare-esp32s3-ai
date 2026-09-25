# ELIO AI Assistant - Wi-Fi AI Avatar

Touchscreen AI avatar sketch for Waveshare ESP32-S3 with Wi-Fi cloud AI requests.

## Features

- **LVGL 8.3.x Interface**: Professional modern dark UI
- **Touchscreen Buttons**: Four main action buttons (ASK AI, ESTIMATE, BUSINESS, SETTINGS)
- **Status Bar**: Real-time Wi-Fi and AI status indicators
- **ST7789 Display**: 240x320 pixel LCD with full color support
- **Touch Support**: CST328/CST3530 touchscreen auto-detection
- **ASK AI Flow**: Tap **ASK AI** to run listen → think → answer cycle
- **Cloud AI Hook**: Sends user prompt to your HTTPS AI endpoint
- **Arduino IDE Compatible**: Single .ino sketch, no PlatformIO required

## Hardware

- ESP32-S3
- Waveshare 2.8-inch ST7789 Display (240x320)
- CST328 or CST3530 Touchscreen Controller

## UI Layout

```
┌─────────────────────────┐
│  ELIO AI                │  Header (80px)
│  Your Smart Assistant   │
├─────────────────────────┤
│                         │
│        [Microphone]     │  Center Icon (120px)
│                         │
├─────────────────────────┤
│  [ASK AI]  [ESTIMATE]   │  Buttons (135px)
│ [BUSINESS] [SETTINGS]   │
├─────────────────────────┤
│ Wi-Fi: OFF | AI: OFF Ready│  Status Bar (35px)
└─────────────────────────┘
```

## Arduino IDE Setup

1. **Board**: ESP32-S3 Dev Module
2. **Partition Scheme**: Huge APP (3MB No OTA)
3. **Required Libraries**:
   - LVGL (8.3.x)
   - Arduino SPI
   - Arduino Wire (I2C)

## Configuration

Before uploading, edit these values in `01_AIAssistant.ino`:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `AI_ENDPOINT_URL`
- `AI_API_KEY` (optional)
- `AI_ROOT_CA` (required CA certificate for TLS verification)

The sketch currently captures user prompts from Serial input (type a sentence and press Enter within ~7 seconds **after Wi-Fi is connected and the avatar enters Listening state**). Blank lines are ignored; the listen phase ends only when a non-empty line is received or the timeout expires. The sketch then sends the text to your **HTTPS** AI endpoint as a `text/plain` POST body and expects plain-text response text. If no input is received before timeout, the avatar shows **No input detected**.

## Installation

1. Open `01_AIAssistant.ino` in Arduino IDE
2. Select ESP32-S3 Dev Module board
3. Install LVGL library via Arduino Library Manager
4. Upload to device
5. Monitor serial at 115200 baud

## Button Behavior

- **ASK AI**: Connects Wi-Fi (if needed), listens for input on Serial, calls cloud HTTPS AI endpoint, speaks/prints response
- **ESTIMATE / BUSINESS**: Return avatar to idle state
- **SETTINGS**: Shows configuration reminder

## Memory Usage

- Display Buffer: ~24 KB
- LVGL Objects: ~30 KB
- Free Heap: ~100 KB+

## Touchscreen Calibration

No calibration needed - coordinates map directly 1:1 with display coordinates.

## Next Steps

- Replace serial input with real microphone capture
- Replace `speak_text()` stub with real TTS audio output
- Add JSON parsing and richer API schema support
- Add wake word and continuous listening mode

## Notes

- This version includes Wi-Fi + HTTPS AI request flow
- All hardware configuration from working 00_HardwareTest
- Touch and display stable at 115200 baud
- Display initialized at 80MHz SPI frequency

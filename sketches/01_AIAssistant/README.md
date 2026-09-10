# ELIO AI Assistant - Phase 1

Modern touchscreen AI Assistant UI built with LVGL 8.3.x for Waveshare ESP32-S3.

## Features

- **LVGL 8.3.x Interface**: Professional modern dark UI
- **Touchscreen Buttons**: Four main action buttons (ASK AI, ESTIMATE, BUSINESS, SETTINGS)
- **Status Bar**: Real-time Wi-Fi and AI status indicators
- **ST7789 Display**: 240x320 pixel LCD with full color support
- **Touch Support**: CST328/CST3530 touchscreen auto-detection
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

## Installation

1. Open `01_AIAssistant.ino` in Arduino IDE
2. Select ESP32-S3 Dev Module board
3. Install LVGL library via Arduino Library Manager
4. Upload to device
5. Monitor serial at 115200 baud

## Button Callbacks

Each button prints a message to Serial when clicked:
- **ASK AI**: Opens voice input mode (Phase 2)
- **ESTIMATE**: Opens estimation mode (Phase 2)
- **BUSINESS**: Opens business mode (Phase 2)
- **SETTINGS**: Opens settings screen (Phase 2)

## Memory Usage

- Display Buffer: ~24 KB
- LVGL Objects: ~30 KB
- Free Heap: ~100 KB+

## Touchscreen Calibration

No calibration needed - coordinates map directly 1:1 with display coordinates.

## Next Steps (Phase 2)

- Voice input processing
- AI model integration
- Network connectivity
- Local inference support

## Notes

- Phase 1 is UI-only, no Wi-Fi or AI code
- All hardware configuration from working 00_HardwareTest
- Touch and display stable at 115200 baud
- Display initialized at 80MHz SPI frequency

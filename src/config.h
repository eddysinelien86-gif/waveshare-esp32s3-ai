#ifndef CONFIG_H
#define CONFIG_H

/*
  WAVESHARE ESP32-S3 TOUCH LCD 2.8 V2 - HARDWARE CONFIGURATION
  ============================================================
  
  Configuration extracted from manufacturer demo files.
  DO NOT MODIFY PINS unless you physically verify your board.
*/

// ==================== DISPLAY CONFIGURATION ====================
// ST7789 Display (240x320 native, 320x240 with rotation)
#define DISPLAY_WIDTH           240  // Native width
#define DISPLAY_HEIGHT          320  // Native height
#define DISPLAY_ROTATION        1    // 0°, 90°, 180°, 270° (1 = 90° rotation)

// SPI Pins for ST7789 (from Display_ST7789.h)
#define SPI_CLK                 GPIO_NUM_40   // SCLK
#define SPI_MOSI                GPIO_NUM_45   // MOSI (Master Out Slave In)
#define SPI_MISO                -1            // MISO (not used)
#define DISPLAY_CS              GPIO_NUM_42   // Chip Select
#define DISPLAY_DC              GPIO_NUM_41   // Data/Command
#define DISPLAY_RST             GPIO_NUM_39   // Reset
#define DISPLAY_BL              GPIO_NUM_5    // Backlight (PWM)
#define DISPLAY_BL_ACTIVE_HIGH  true          // Active HIGH for backlight

// SPI Frequency
#define SPI_FREQUENCY           80000000  // 80 MHz (from manufacturer demo)
#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0

// ==================== TOUCH CONFIGURATION ====================
// Dual touch controller support: CST328 (I2C @ 0x1A) or CST3530 (I2C @ 0x58)
// Manufacturer demo auto-detects CST328 first, then falls back to CST3530

#define TOUCH_USE_CST328        1  // Try CST328 first
#define TOUCH_USE_CST3530       1  // Fallback to CST3530

// CST328 Configuration (from Touch_CST328.h)
#define CST328_ADDR             0x1A
#define CST328_SDA_PIN          GPIO_NUM_1
#define CST328_SCL_PIN          GPIO_NUM_3
#define CST328_INT_PIN          GPIO_NUM_4   // Interrupt (optional, used with ISR)
#define CST328_RST_PIN          GPIO_NUM_2   // Reset
#define CST328_I2C_FREQ         400000       // I2C frequency 400kHz
#define CST328_LCD_TOUCH_MAX_POINTS  5       // Max 5 simultaneous touch points

// CST3530 Configuration (from Touch_CST3530.h)
#define CST3530_ADDR            0x58
#define CST3530_SDA_PIN         GPIO_NUM_1   // Same I2C bus as CST328
#define CST3530_SCL_PIN         GPIO_NUM_3
#define CST3530_INT_PIN         GPIO_NUM_4
#define CST3530_RST_PIN         GPIO_NUM_2
#define CST3530_I2C_FREQ_HZ     400000
#define CST3530_LCD_TOUCH_MAX_POINTS  5

// Touch calibration (from manufacturer demo - no inversion needed)
#define TOUCH_SWAP_XY           false        // Don't swap X and Y
#define TOUCH_INVERT_X          false        // Don't invert X
#define TOUCH_INVERT_Y          false        // Don't invert Y
#define TOUCH_MIN_X             0
#define TOUCH_MAX_X             240
#define TOUCH_MIN_Y             0
#define TOUCH_MAX_Y             320

// ==================== SD CARD CONFIGURATION ====================
// SD MMC pins (from SD_Card.h)
#define SD_CLK_PIN              GPIO_NUM_14
#define SD_CMD_PIN              GPIO_NUM_17
#define SD_D0_PIN               GPIO_NUM_16
#define SD_D1_PIN               -1  // Not used
#define SD_D2_PIN               -1  // Not used
#define SD_D3_PIN               GPIO_NUM_21  // Enable pin

// ==================== POWER MANAGEMENT ====================
// PWR pins (from PWR_Key.h)
#define PWR_KEY_INPUT_PIN       GPIO_NUM_6   // Power button input
#define PWR_CONTROL_PIN         GPIO_NUM_7   // Power control (active HIGH)
#define DEVICE_SLEEP_TIME       10
#define DEVICE_RESTART_TIME     15
#define DEVICE_SHUTDOWN_TIME    20

// ==================== BATTERY MONITORING ====================
// ADC pin for battery voltage (from BAT_Driver.h)
#define BAT_ADC_PIN             GPIO_NUM_8
#define BAT_MEASUREMENT_OFFSET  0.990476

// ==================== IMU / ACCELEROMETER ====================
// QMI8658 IMU (from Gyro_QMI8658.h)
#define QMI8658_I2C_ADDR        0x6B  // Address when SD0/SA0 is LOW
#define QMI8658_I2C_ALT_ADDR    0x6A  // Address when SD0/SA0 is HIGH

// ==================== RTC ====================
// PCF85063 Real Time Clock (from RTC_PCF85063.h)
#define PCF85063_I2C_ADDR       0x51

// ==================== AUDIO / I2S ====================
// PCM5101 DAC (from Audio_PCM5101.h)
#define I2S_DOUT                GPIO_NUM_47  // I2S Data Out
#define I2S_BCLK                GPIO_NUM_48  // I2S Bit Clock
#define I2S_LRC                 GPIO_NUM_38  // I2S Word Select (LRCK)
#define AUDIO_SAMPLE_RATE       16000        // 16kHz
#define AUDIO_TICK_PERIOD_MS    20
#define AUDIO_VOLUME_MAX        21           // 0-21 range for PCM5101

// ==================== BACKLIGHT PWM ====================
#define BACKLIGHT_PWM_FREQ      20000        // 20kHz
#define BACKLIGHT_PWM_RES       10           // 10-bit resolution (0-1023)
#define BACKLIGHT_PWM_DUTY      500          // Initial duty factor (50%)
#define BACKLIGHT_MAX           100          // User-facing brightness scale (0-100)

// ==================== LVGL CONFIGURATION ====================
#define LVGL_WIDTH              240
#define LVGL_HEIGHT             320
#define LVGL_BUF_LEN            (LVGL_WIDTH * LVGL_HEIGHT / 20)
#define LVGL_TICK_PERIOD_MS     2

// ==================== UI COLORS (RGB565) ====================
#define COLOR_BLACK             0x0000
#define COLOR_WHITE             0xFFFF
#define COLOR_DARK_BG           0x1082
#define COLOR_DARK_ALT          0x2104
#define COLOR_PRIMARY           0x047F
#define COLOR_PRIMARY_DARK      0x0356
#define COLOR_SUCCESS           0x07E0
#define COLOR_WARNING           0xFE00
#define COLOR_ERROR             0xF800
#define COLOR_TEXT              0xF79E
#define COLOR_TEXT_SECONDARY    0xA514

// ==================== DEBUG CONFIGURATION ====================
#define SERIAL_BAUD             115200
#define DEBUG_LEVEL             3  // 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose
#define DEBUG_DISPLAY           true
#define DEBUG_TOUCH             true
#define DEBUG_WIFI              true
#define DEBUG_MEMORY            true

// ==================== MEMORY CONFIGURATION ====================
#define CHAT_MAX_HISTORY        50
#define MAX_MESSAGE_LENGTH      500
#define SPIFFS_SIZE             1048576  // 1MB

// ==================== TIMING ====================
#define TOUCH_DEBOUNCE_MS       50
#define UI_REFRESH_MS           50
#define WIFI_CHECK_INTERVAL_S   30
#define MEMORY_CHECK_INTERVAL_S 60

#endif // CONFIG_H

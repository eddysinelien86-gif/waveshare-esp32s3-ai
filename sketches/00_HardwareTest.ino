/*
  WAVESHARE ESP32-S3 HARDWARE TEST
  ================================
  
  This minimal test verifies:
  1. Serial communication at 115200 baud
  2. Display (ST7789) initialization via SPI
  3. Display backlight (PWM on GPIO5)
  4. Touch controller (CST328/CST3530) via I2C
  5. Touch coordinate reading
  
  IMPORTANT: Do NOT proceed to full AI assistant until this test passes!
  
  Expected output on Serial Monitor @ 115200:
  - Board information and flash/memory details
  - "Display initialized successfully"
  - "AI ASSISTANT" text on screen
  - "Touch initialized" message
  - X/Y coordinates printed when screen is touched
  - Touch data every 100ms
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "../src/config.h"
#include "../src/utils/Debug.h"

// ==================== GLOBALS ====================
SPIClass spi_display(FSPI);  // Use FSPI (SPI2)
TwoWire i2c_touch(1);         // Use I2C bus 1 (Wire1)

uint8_t touch_controller = 0;
#define TOUCH_NONE      0
#define TOUCH_CST328    1
#define TOUCH_CST3530   2

// ==================== DISPLAY SPI FUNCTIONS ====================

void spi_init() {
  DEBUG_INFO("Initializing SPI for display...");
  spi_display.begin(SPI_CLK, SPI_MISO, SPI_MOSI);
  pinMode(DISPLAY_CS, OUTPUT);
  pinMode(DISPLAY_DC, OUTPUT);
  pinMode(DISPLAY_RST, OUTPUT);
  DEBUG_INFO("SPI initialized");
}

void spi_write_cmd(uint8_t cmd) {
  digitalWrite(DISPLAY_DC, LOW);  // Command mode
  spi_display.write(cmd);
  digitalWrite(DISPLAY_DC, HIGH); // Return to data mode
}

void spi_write_data(uint8_t data) {
  digitalWrite(DISPLAY_DC, HIGH); // Data mode
  spi_display.write(data);
}

void spi_write_data_16(uint16_t data) {
  digitalWrite(DISPLAY_DC, HIGH);
  spi_display.write16(data);
}

void spi_write_bytes(uint8_t* data, uint32_t len) {
  digitalWrite(DISPLAY_DC, HIGH);
  spi_display.writeBytes(data, len);
}

// ==================== DISPLAY INITIALIZATION ====================
// ST7789 initialization sequence from Display_ST7789.cpp

void display_reset() {
  DEBUG_INFO("Resetting display...");
  
  // Pull CS high (deselect)
  digitalWrite(DISPLAY_CS, HIGH);
  delay(50);
  
  // Reset sequence
  digitalWrite(DISPLAY_RST, LOW);
  delay(50);
  digitalWrite(DISPLAY_RST, HIGH);
  delay(50);
  
  DEBUG_INFO("Display reset complete");
}

void display_init() {
  DEBUG_INFO("Initializing ST7789 display (240x320)...");
  
  spi_init();
  display_reset();
  
  // Start SPI transaction
  spi_display.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  digitalWrite(DISPLAY_CS, LOW);
  
  // ST7789 Initialization Sequence (from manufacturer demo)
  delay(120);
  
  spi_write_cmd(0x29);  // Display ON
  delay(120);
  
  spi_write_cmd(0x11);  // Sleep OUT
  delay(120);
  
  spi_write_cmd(0x36);  // MADCTL - Memory Access Control
  spi_write_data(0x00); // No rotation, top-to-bottom, left-to-right
  
  spi_write_cmd(0x3A);  // COLMOD - Interface Pixel Format
  spi_write_data(0x05); // 16-bit RGB565
  
  spi_write_cmd(0xB0);  // RAMCTRL - RAM Control
  spi_write_data(0x00);
  spi_write_data(0xE8);
  
  spi_write_cmd(0xB2);  // FRCTR1 - Frame Rate Control 1
  spi_write_data(0x0C);
  spi_write_data(0x0C);
  spi_write_data(0x00);
  spi_write_data(0x33);
  spi_write_data(0x33);
  
  spi_write_cmd(0xB7);  // GCTRL - Gate Control
  spi_write_data(0x75);
  
  spi_write_cmd(0xBB);  // VCOMS - VCOM Setting
  spi_write_data(0x1A);
  
  spi_write_cmd(0xC0);  // LCMCTRL - LCM Control
  spi_write_data(0x2C);
  
  spi_write_cmd(0xC2);  // VDVVRHEN - VDV and VRH Command Enable
  spi_write_data(0x01);
  spi_write_data(0xFF);
  
  spi_write_cmd(0xC3);  // VRHS - VRH Set
  spi_write_data(0x13);
  
  spi_write_cmd(0xC4);  // VDVS - VDV Set
  spi_write_data(0x20);
  
  spi_write_cmd(0xC6);  // FRCTRL2 - Frame Rate Control 2
  spi_write_data(0x0F);
  
  spi_write_cmd(0xD0);  // PWMFRSEL - Power Control & Pump Freq Select
  spi_write_data(0xA4);
  spi_write_data(0xA1);
  
  spi_write_cmd(0xD6);  // CSCON - Enhance Display Performance
  spi_write_data(0xA1);
  
  // Gamma correction sequence
  spi_write_cmd(0xE0);  // PVGAMCTRL - Positive Voltage Gamma Control
  spi_write_data(0xD0);
  spi_write_data(0x0D);
  spi_write_data(0x14);
  spi_write_data(0x0D);
  spi_write_data(0x0D);
  spi_write_data(0x09);
  spi_write_data(0x38);
  spi_write_data(0x44);
  spi_write_data(0x4E);
  spi_write_data(0x3A);
  spi_write_data(0x17);
  spi_write_data(0x18);
  spi_write_data(0x2F);
  spi_write_data(0x30);
  
  spi_write_cmd(0xE1);  // NVGAMCTRL - Negative Voltage Gamma Control
  spi_write_data(0xD0);
  spi_write_data(0x09);
  spi_write_data(0x0F);
  spi_write_data(0x08);
  spi_write_data(0x07);
  spi_write_data(0x14);
  spi_write_data(0x37);
  spi_write_data(0x44);
  spi_write_data(0x4D);
  spi_write_data(0x38);
  spi_write_data(0x15);
  spi_write_data(0x16);
  spi_write_data(0x2C);
  spi_write_data(0x2E);
  
  spi_write_cmd(0x21);  // INVON - Display Inversion ON
  
  spi_write_cmd(0x29);  // DISPON - Display ON
  
  spi_write_cmd(0x2C);  // RAMWR - RAM Write
  
  digitalWrite(DISPLAY_CS, HIGH);
  spi_display.endTransaction();
  
  DEBUG_INFO("ST7789 initialization complete");
}

// ==================== BACKLIGHT CONTROL ====================

void backlight_init() {
  DEBUG_INFO("Initializing backlight (PWM on GPIO%d)...", DISPLAY_BL);
  
  // Configure PWM on backlight pin
  ledcAttach(DISPLAY_BL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RES);
  
  // Set to 50% brightness (512 out of 1023)
  ledcWrite(DISPLAY_BL, 512);
  
  DEBUG_INFO("Backlight initialized");
}

void backlight_set(uint8_t brightness) {
  // brightness: 0-100
  if (brightness > 100) brightness = 100;
  uint16_t pwm_value = (brightness * 1023) / 100;
  if (DISPLAY_BL_ACTIVE_HIGH) {
    ledcWrite(DISPLAY_BL, pwm_value);
  } else {
    ledcWrite(DISPLAY_BL, 1023 - pwm_value);
  }
  DEBUG_DEBUG("Backlight set to %d%%", brightness);
}

// ==================== DISPLAY DRAWING ====================

void display_fill_screen(uint16_t color) {
  spi_display.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  digitalWrite(DISPLAY_CS, LOW);
  
  // Set window to full screen
  spi_write_cmd(0x2A);  // CASET - Column Address Set
  spi_write_data(0x00);
  spi_write_data(0x00);
  spi_write_data(0x00);
  spi_write_data(0xEF);
  
  spi_write_cmd(0x2B);  // RASET - Row Address Set
  spi_write_data(0x00);
  spi_write_data(0x00);
  spi_write_data(0x01);
  spi_write_data(0x3F);
  
  spi_write_cmd(0x2C);  // RAMWR - RAM Write
  
  // Fill with color
  for (int i = 0; i < 240 * 320; i++) {
    spi_write_data_16(color);
  }
  
  digitalWrite(DISPLAY_CS, HIGH);
  spi_display.endTransaction();
}

// Simple text display (just draws a test pattern)
void display_test_pattern() {
  DEBUG_INFO("Drawing test pattern...");
  display_fill_screen(COLOR_BLACK);
  delay(500);
  display_fill_screen(COLOR_WHITE);
  delay(500);
  display_fill_screen(COLOR_PRIMARY);
  delay(500);
  display_fill_screen(COLOR_BLACK);
}

// ==================== TOUCH INITIALIZATION ====================

bool touch_i2c_read(uint8_t addr, uint16_t reg, uint8_t* data, uint32_t len) {
  i2c_touch.beginTransmission(addr);
  i2c_touch.write((uint8_t)(reg >> 8));
  i2c_touch.write((uint8_t)reg);
  if (i2c_touch.endTransmission(false) != 0) {
    return false;
  }
  
  uint32_t bytes_read = i2c_touch.requestFrom(addr, len);
  if (bytes_read != len) {
    return false;
  }
  
  for (uint32_t i = 0; i < len; i++) {
    data[i] = i2c_touch.read();
  }
  return true;
}

bool touch_i2c_write(uint8_t addr, uint16_t reg, const uint8_t* data, uint32_t len) {
  i2c_touch.beginTransmission(addr);
  i2c_touch.write((uint8_t)(reg >> 8));
  i2c_touch.write((uint8_t)reg);
  for (uint32_t i = 0; i < len; i++) {
    i2c_touch.write(data[i]);
  }
  if (i2c_touch.endTransmission(true) != 0) {
    return false;
  }
  return true;
}

void touch_reset() {
  DEBUG_INFO("Resetting touch controller...");
  digitalWrite(CST328_RST_PIN, HIGH);
  delay(50);
  digitalWrite(CST328_RST_PIN, LOW);
  delay(5);
  digitalWrite(CST328_RST_PIN, HIGH);
  delay(50);
  DEBUG_INFO("Touch reset complete");
}

bool touch_probe_cst328() {
  DEBUG_INFO("Probing for CST328 at I2C 0x%02X...", CST328_ADDR);
  
  uint8_t buf[24] = {0};
  
  // Try to read CST328 debug info
  if (!touch_i2c_write(CST328_ADDR, 0xD101, buf, 0)) {
    DEBUG_WARN("CST328 write failed");
    return false;
  }
  
  if (!touch_i2c_read(CST328_ADDR, 0xD1FC, buf, 4)) {
    DEBUG_WARN("CST328 read failed");
    return false;
  }
  
  if (!touch_i2c_read(CST328_ADDR, 0xD1FC, buf, 24)) {
    DEBUG_WARN("CST328 extended read failed");
    return false;
  }
  
  uint16_t checksum = (((uint16_t)buf[11] << 8) | buf[10]);
  DEBUG_INFO("CST328 checksum: 0x%04X", checksum);
  
  if (checksum == 0xCACA) {
    DEBUG_INFO("CST328 detected successfully!");
    // Return to normal mode
    touch_i2c_write(CST328_ADDR, 0xD109, buf, 0);
    return true;
  }
  
  return false;
}

bool touch_probe_cst3530() {
  DEBUG_INFO("Probing for CST3530 at I2C 0x%02X...", CST3530_ADDR);
  
  // Just try to send a transmission
  i2c_touch.beginTransmission(CST3530_ADDR);
  if (i2c_touch.endTransmission(true) == 0) {
    DEBUG_INFO("CST3530 detected successfully!");
    return true;
  }
  
  DEBUG_WARN("CST3530 not found");
  return false;
}

void touch_init() {
  DEBUG_INFO("Initializing touch on I2C1 (SDA=GPIO%d, SCL=GPIO%d)...", CST328_SDA_PIN, CST328_SCL_PIN);
  
  i2c_touch.begin(CST328_SDA_PIN, CST328_SCL_PIN, CST328_I2C_FREQ);
  
  pinMode(CST328_INT_PIN, INPUT);
  pinMode(CST328_RST_PIN, OUTPUT);
  
  touch_reset();
  delay(100);
  
  touch_controller = TOUCH_NONE;
  
  // Try CST328 first
  if (TOUCH_USE_CST328 && touch_probe_cst328()) {
    touch_controller = TOUCH_CST328;
    DEBUG_INFO("Touch controller: CST328");
    return;
  }
  
  // Fallback to CST3530
  if (TOUCH_USE_CST3530 && touch_probe_cst3530()) {
    touch_controller = TOUCH_CST3530;
    DEBUG_INFO("Touch controller: CST3530");
    return;
  }
  
  touch_controller = TOUCH_NONE;
  DEBUG_ERROR("No touch controller detected!");
}

// ==================== TOUCH READING ====================

bool touch_read_cst328(uint16_t& x, uint16_t& y) {
  uint8_t buf[5] = {0};
  
  if (!touch_i2c_read(CST328_ADDR, 0xD005, buf, 1)) {
    return false;
  }
  
  uint8_t touch_cnt = buf[0] & 0x0F;
  if (touch_cnt == 0) {
    return false;
  }
  
  if (!touch_i2c_read(CST328_ADDR, 0xD000, buf, 5)) {
    return false;
  }
  
  // Clear the status register
  uint8_t clear = 0;
  touch_i2c_write(CST328_ADDR, 0xD005, &clear, 1);
  
  // Extract coordinates
  x = (uint16_t)(((buf[1] << 4) + ((buf[3] & 0xF0) >> 4)));
  y = (uint16_t)(((buf[2] << 4) + (buf[3] & 0x0F)));
  
  return true;
}

bool touch_read_cst3530(uint16_t& x, uint16_t& y) {
  uint8_t buf[9] = {0};
  
  if (!touch_i2c_read(CST3530_ADDR, 0xD0070000, buf, 9)) {
    return false;
  }
  
  uint8_t touch_cnt = buf[3] & 0x0F;
  if (touch_cnt == 0 || (buf[8] & 0xF0) == 0) {
    return false;
  }
  
  // End read
  touch_i2c_write(CST3530_ADDR, 0xD00002AB, NULL, 0);
  
  // Extract coordinates
  x = (uint16_t)(((buf[7] & 0x0F) << 8) + buf[4]);
  y = (uint16_t)(((buf[7] & 0xF0) << 4) + buf[5]);
  
  return true;
}

bool touch_read(uint16_t& x, uint16_t& y) {
  if (touch_controller == TOUCH_CST328) {
    return touch_read_cst328(x, y);
  } else if (touch_controller == TOUCH_CST3530) {
    return touch_read_cst3530(x, y);
  }
  return false;
}

// ==================== SETUP ====================

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  Waveshare ESP32-S3 Hardware Test");
  Serial.println("  Display: ST7789 (240x320)");
  Serial.println("  Touch: CST328/CST3530");
  Serial.println("========================================");
  Serial.println();
  
  // Print board info
  printChipInfo();
  printMemory();
  Serial.println();
  
  // Print configuration
  DEBUG_INFO("Display SPI: CLK=%d, MOSI=%d, CS=%d, DC=%d, RST=%d",
    SPI_CLK, SPI_MOSI, DISPLAY_CS, DISPLAY_DC, DISPLAY_RST);
  DEBUG_INFO("Backlight PWM: GPIO%d (20kHz, 10-bit)", DISPLAY_BL);
  DEBUG_INFO("Touch I2C1: SDA=%d, SCL=%d, INT=%d, RST=%d",
    CST328_SDA_PIN, CST328_SCL_PIN, CST328_INT_PIN, CST328_RST_PIN);
  Serial.println();
  
  // Initialize systems
  DEBUG_INFO("Initializing display...");
  display_init();
  
  DEBUG_INFO("Initializing backlight...");
  backlight_init();
  backlight_set(100);
  
  DEBUG_INFO("Initializing touch...");
  touch_init();
  
  // Test display
  DEBUG_INFO("Running display test pattern...");
  display_test_pattern();
  
  // Show final test screen (black with brightness info)
  display_fill_screen(COLOR_BLACK);
  backlight_set(100);
  
  Serial.println();
  DEBUG_INFO("Hardware test complete. Touch the screen...");
  Serial.println();
}

// ==================== LOOP ====================

void loop() {
  static uint32_t last_touch = 0;
  static uint32_t last_status = 0;
  uint32_t now = millis();
  
  // Read touch
  uint16_t touch_x = 0, touch_y = 0;
  if (touch_read(touch_x, touch_y)) {
    if (now - last_touch > TOUCH_DEBOUNCE_MS) {
      DEBUG_INFO("TOUCH: X=%d Y=%d", touch_x, touch_y);
      last_touch = now;
    }
  }
  
  // Periodic status
  if (now - last_status > 5000) {
    DEBUG_DEBUG("Touch controller: %s",
      (touch_controller == TOUCH_CST328) ? "CST328" :
      (touch_controller == TOUCH_CST3530) ? "CST3530" : "NONE");
    printMemory();
    last_status = now;
  }
  
  delay(10);
}

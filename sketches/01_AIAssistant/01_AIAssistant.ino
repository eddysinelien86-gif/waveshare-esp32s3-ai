/*
  ELIO AI ASSISTANT - PHASE 1
  ============================
  
  Custom AI Assistant UI with:
  - LVGL 8.3.x for modern touchscreen interface
  - ST7789 display (240x320)
  - Touchscreen with 4 main buttons
  - Status bar with Wi-Fi/AI status
  - Clean modern dark interface
  
  Hardware:
  - ESP32-S3
  - Waveshare 2.8-inch ST7789 LCD
  - CST328/CST3530 touchscreen
  
  Phase 1 Goals:
  - Build stable UI framework
  - Implement touchscreen button handling
  - Display status indicators
  - No Wi-Fi or AI API code yet
  
  Arduino IDE:
  - Board: ESP32-S3 Dev Module
  - Build as single .ino sketch
  - No dependencies on PlatformIO
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <lvgl.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ==================== CONFIGURATION ====================

// Display Configuration
#define DISPLAY_WIDTH           240
#define DISPLAY_HEIGHT          320
#define SPI_CLK                 GPIO_NUM_40
#define SPI_MOSI                GPIO_NUM_45
#define SPI_MISO                -1
#define DISPLAY_CS              GPIO_NUM_42
#define DISPLAY_DC              GPIO_NUM_41
#define DISPLAY_RST             GPIO_NUM_39
#define DISPLAY_BL              GPIO_NUM_5
#define DISPLAY_BL_ACTIVE_HIGH  true
#define SPI_FREQUENCY           80000000

// Touch Configuration
#define TOUCH_USE_CST328        1
#define TOUCH_USE_CST3530       1
#define CST328_ADDR             0x1A
#define CST328_SDA_PIN          GPIO_NUM_1
#define CST328_SCL_PIN          GPIO_NUM_3
#define CST328_INT_PIN          GPIO_NUM_4
#define CST328_RST_PIN          GPIO_NUM_2
#define CST328_I2C_FREQ         400000
#define CST3530_ADDR            0x58

// Touch calibration
#define TOUCH_SWAP_XY           false
#define TOUCH_INVERT_X          false
#define TOUCH_INVERT_Y          false
#define TOUCH_MIN_X             0
#define TOUCH_MAX_X             240
#define TOUCH_MIN_Y             0
#define TOUCH_MAX_Y             320

// LVGL Configuration
#define LVGL_WIDTH              240
#define LVGL_HEIGHT             320
#define LVGL_BUF_LEN            (LVGL_WIDTH * LVGL_HEIGHT / 20)
#define LVGL_TICK_PERIOD_MS     2

// UI Colors (RGB565)
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

// Serial
#define SERIAL_BAUD             115200
#define TOUCH_DEBOUNCE_MS       50

// Wi-Fi + AI cloud settings
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* AI_ENDPOINT_URL = "https://your-ai-endpoint";
const char* AI_API_KEY = "";
const char* AI_ROOT_CA = "-----BEGIN CERTIFICATE-----\nYOUR_ROOT_CA_CERT\n-----END CERTIFICATE-----\n";

// ==================== GLOBALS ====================

SPIClass spi_display(FSPI);
TwoWire i2c_touch(1);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LVGL_BUF_LEN];

uint8_t touch_controller = 0;
#define TOUCH_NONE      0
#define TOUCH_CST328    1
#define TOUCH_CST3530   2

// Status labels (for updates)
lv_obj_t * lbl_wifi_status = NULL;
lv_obj_t * lbl_ai_status = NULL;
lv_obj_t * lbl_avatar_status = NULL;
lv_obj_t * ai_icon = NULL;
lv_obj_t * icon_lbl = NULL;

enum AvatarState {
  AVATAR_IDLE,
  AVATAR_LISTENING,
  AVATAR_THINKING,
  AVATAR_SPEAKING,
  AVATAR_ERROR
};

AvatarState avatar_state = AVATAR_IDLE;
uint32_t last_wifi_retry_ms = 0;
volatile bool ask_ai_requested = false;
bool wifi_connecting = false;
uint32_t wifi_connect_started_ms = 0;

void update_avatar_state(AvatarState state, const String& text);
bool connect_wifi(uint32_t timeout_ms = 15000);
void run_ask_ai_flow();
void service_ui_delay(uint32_t ms);
bool wifi_is_configured();

// ==================== DEBUG OUTPUT ====================

void print_chip_info() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  
  Serial.printf("ESP32 Chip: %s\n", CONFIG_IDF_TARGET);
  Serial.printf("Revision: %d\n", chip_info.revision);
  Serial.printf("CPU Cores: %d\n", chip_info.cores);
  Serial.printf("Flash Size: %u MB\n", spi_flash_get_chip_size() / (1024 * 1024));
  Serial.printf("Built-in Flash: %s\n", (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Yes" : "No");
}

void print_memory() {
  Serial.printf("Heap: Free=%u KB, Total=%u KB\n",
    ESP.getFreeHeap() / 1024,
    ESP.getHeapSize() / 1024);
  Serial.printf("PSRAM: Free=%u KB, Total=%u KB\n",
    ESP.getFreePsram() / 1024,
    ESP.getPsramSize() / 1024);
}

// ==================== DISPLAY SPI FUNCTIONS ====================

void spi_init() {
  Serial.println("Initializing SPI for display...");
  spi_display.begin(SPI_CLK, SPI_MISO, SPI_MOSI);
  pinMode(DISPLAY_CS, OUTPUT);
  pinMode(DISPLAY_DC, OUTPUT);
  pinMode(DISPLAY_RST, OUTPUT);
  digitalWrite(DISPLAY_CS, HIGH);
  digitalWrite(DISPLAY_DC, HIGH);
  Serial.println("SPI initialized");
}

void spi_write_cmd(uint8_t cmd) {
  digitalWrite(DISPLAY_DC, LOW);
  spi_display.write(cmd);
  digitalWrite(DISPLAY_DC, HIGH);
}

void spi_write_data(uint8_t data) {
  digitalWrite(DISPLAY_DC, HIGH);
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

void display_reset() {
  Serial.println("Resetting display...");
  digitalWrite(DISPLAY_CS, HIGH);
  delay(50);
  digitalWrite(DISPLAY_RST, LOW);
  delay(50);
  digitalWrite(DISPLAY_RST, HIGH);
  delay(50);
  Serial.println("Display reset complete");
}

void display_init() {
  Serial.println("Initializing ST7789 display (240x320)...");
  
  spi_init();
  display_reset();
  
  spi_display.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  digitalWrite(DISPLAY_CS, LOW);
  
  delay(120);
  
  spi_write_cmd(0x29);  // Display ON
  delay(120);
  
  spi_write_cmd(0x11);  // Sleep OUT
  delay(120);
  
  spi_write_cmd(0x36);  // MADCTL
  spi_write_data(0x00);
  
  spi_write_cmd(0x3A);  // COLMOD
  spi_write_data(0x05);
  
  spi_write_cmd(0xB0);  // RAMCTRL
  spi_write_data(0x00);
  spi_write_data(0xE8);
  
  spi_write_cmd(0xB2);  // FRCTR1
  spi_write_data(0x0C);
  spi_write_data(0x0C);
  spi_write_data(0x00);
  spi_write_data(0x33);
  spi_write_data(0x33);
  
  spi_write_cmd(0xB7);  // GCTRL
  spi_write_data(0x75);
  
  spi_write_cmd(0xBB);  // VCOMS
  spi_write_data(0x1A);
  
  spi_write_cmd(0xC0);  // LCMCTRL
  spi_write_data(0x2C);
  
  spi_write_cmd(0xC2);  // VDVVRHEN
  spi_write_data(0x01);
  spi_write_data(0xFF);
  
  spi_write_cmd(0xC3);  // VRHS
  spi_write_data(0x13);
  
  spi_write_cmd(0xC4);  // VDVS
  spi_write_data(0x20);
  
  spi_write_cmd(0xC6);  // FRCTRL2
  spi_write_data(0x0F);
  
  spi_write_cmd(0xD0);  // PWMFRSEL
  spi_write_data(0xA4);
  spi_write_data(0xA1);
  
  spi_write_cmd(0xD6);  // CSCON
  spi_write_data(0xA1);
  
  spi_write_cmd(0xE0);  // PVGAMCTRL
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
  
  spi_write_cmd(0xE1);  // NVGAMCTRL
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
  
  spi_write_cmd(0x21);  // INVON
  
  spi_write_cmd(0x29);  // DISPON
  
  spi_write_cmd(0x2C);  // RAMWR
  
  digitalWrite(DISPLAY_CS, HIGH);
  spi_display.endTransaction();
  
  Serial.println("ST7789 initialization complete");
}

void display_backlight_set(uint8_t brightness) {
  if (brightness > 100) brightness = 100;
  uint16_t pwm_value = (brightness * 1023) / 100;
  if (DISPLAY_BL_ACTIVE_HIGH) {
    ledcWrite(DISPLAY_BL, pwm_value);
  } else {
    ledcWrite(DISPLAY_BL, 1023 - pwm_value);
  }
}

void display_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  spi_display.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  digitalWrite(DISPLAY_CS, LOW);
  
  spi_write_cmd(0x2A);
  spi_write_data(x1 >> 8);
  spi_write_data(x1 & 0xFF);
  spi_write_data(x2 >> 8);
  spi_write_data(x2 & 0xFF);
  
  spi_write_cmd(0x2B);
  spi_write_data(y1 >> 8);
  spi_write_data(y1 & 0xFF);
  spi_write_data(y2 >> 8);
  spi_write_data(y2 & 0xFF);
  
  spi_write_cmd(0x2C);
  
  digitalWrite(DISPLAY_CS, HIGH);
  spi_display.endTransaction();
}

void display_write_pixels(uint16_t* pixels, uint32_t len) {
  spi_display.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  digitalWrite(DISPLAY_CS, LOW);
  digitalWrite(DISPLAY_DC, HIGH);
  spi_display.writeBytes((uint8_t*)pixels, len * 2);
  digitalWrite(DISPLAY_CS, HIGH);
  spi_display.endTransaction();
}

// ==================== BACKLIGHT PWM ====================

void backlight_init() {
  Serial.printf("Initializing backlight PWM on GPIO%d...\n", DISPLAY_BL);
  ledcAttach(DISPLAY_BL, 20000, 10);
  display_backlight_set(50);
  Serial.println("Backlight initialized");
}

// ==================== TOUCH I2C FUNCTIONS ====================

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

// ==================== TOUCH INITIALIZATION ====================

void touch_reset() {
  Serial.println("Resetting touch controller...");
  digitalWrite(CST328_RST_PIN, HIGH);
  delay(50);
  digitalWrite(CST328_RST_PIN, LOW);
  delay(5);
  digitalWrite(CST328_RST_PIN, HIGH);
  delay(50);
  Serial.println("Touch reset complete");
}

bool touch_probe_cst328() {
  Serial.printf("Probing for CST328 at I2C 0x%02X...\n", CST328_ADDR);
  
  uint8_t buf[24] = {0};
  
  if (!touch_i2c_write(CST328_ADDR, 0xD101, buf, 0)) {
    Serial.println("CST328 write failed");
    return false;
  }
  
  if (!touch_i2c_read(CST328_ADDR, 0xD1FC, buf, 4)) {
    Serial.println("CST328 read failed");
    return false;
  }
  
  if (!touch_i2c_read(CST328_ADDR, 0xD1FC, buf, 24)) {
    Serial.println("CST328 extended read failed");
    return false;
  }
  
  uint16_t checksum = (((uint16_t)buf[11] << 8) | buf[10]);
  Serial.printf("CST328 checksum: 0x%04X\n", checksum);
  
  if (checksum == 0xCACA) {
    Serial.println("CST328 detected successfully!");
    touch_i2c_write(CST328_ADDR, 0xD109, buf, 0);
    return true;
  }
  
  return false;
}

bool touch_probe_cst3530() {
  Serial.printf("Probing for CST3530 at I2C 0x%02X...\n", CST3530_ADDR);
  
  i2c_touch.beginTransmission(CST3530_ADDR);
  if (i2c_touch.endTransmission(true) == 0) {
    Serial.println("CST3530 detected successfully!");
    return true;
  }
  
  Serial.println("CST3530 not found");
  return false;
}

void touch_init() {
  Serial.printf("Initializing touch on I2C1 (SDA=GPIO%d, SCL=GPIO%d)...\n", CST328_SDA_PIN, CST328_SCL_PIN);
  
  i2c_touch.begin(CST328_SDA_PIN, CST328_SCL_PIN, CST328_I2C_FREQ);
  
  pinMode(CST328_INT_PIN, INPUT);
  pinMode(CST328_RST_PIN, OUTPUT);
  
  touch_reset();
  delay(100);
  
  touch_controller = TOUCH_NONE;
  
  if (TOUCH_USE_CST328 && touch_probe_cst328()) {
    touch_controller = TOUCH_CST328;
    Serial.println("Touch controller: CST328");
    return;
  }
  
  if (TOUCH_USE_CST3530 && touch_probe_cst3530()) {
    touch_controller = TOUCH_CST3530;
    Serial.println("Touch controller: CST3530");
    return;
  }
  
  touch_controller = TOUCH_NONE;
  Serial.println("ERROR: No touch controller detected!");
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
  
  uint8_t clear = 0;
  touch_i2c_write(CST328_ADDR, 0xD005, &clear, 1);
  
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
  
  touch_i2c_write(CST3530_ADDR, 0xD00002AB, NULL, 0);
  
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

// ==================== LVGL DISPLAY FLUSH ====================

void lv_disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p) {
  uint32_t width = (area->x2 - area->x1 + 1);
  uint32_t height = (area->y2 - area->y1 + 1);
  
  display_set_window(area->x1, area->y1, area->x2, area->y2);
  display_write_pixels((uint16_t*)color_p, width * height);
  
  lv_disp_flush_ready(disp);
}

// ==================== LVGL TICK HANDLER ====================

static uint32_t tick_count = 0;

void lv_tick_handler() {
  tick_count += LVGL_TICK_PERIOD_MS;
}

uint32_t lv_get_ticks() {
  return tick_count;
}

// ==================== LVGL TOUCH INPUT ====================

void lv_touch_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
  static uint16_t last_x = 0, last_y = 0;
  static bool last_pressed = false;
  
  uint16_t touch_x, touch_y;
  bool touched = touch_read(touch_x, touch_y);
  
  if (touched) {
    data->point.x = touch_x;
    data->point.y = touch_y;
    data->state = LV_INDEV_STATE_PRESSED;
    last_pressed = true;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
    last_pressed = false;
  }

  String capture_user_input_from_serial(uint32_t timeout_ms = 7000) {
    Serial.println("[LISTEN] Type your message in Serial Monitor and press Enter...");
    uint32_t start = millis();
    String input;
    input.reserve(256);

    while (millis() - start < timeout_ms) {
      while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
          input.trim();
          if (input.length() > 0) {
            return input;
          }
        } else {
          input += c;
        }
      }
      service_ui_delay(10);
    }

    input.trim();
    return input;
  }

  bool ask_ai_service(const String& user_prompt, String& response) {
    String endpoint = String(AI_ENDPOINT_URL);
    endpoint.trim();

    if (endpoint.length() == 0 || endpoint.indexOf("your-ai-endpoint") >= 0) {
      response = "Set AI_ENDPOINT_URL in the sketch to your AI service URL.";
      return false;
    }

    if (!endpoint.startsWith("https://")) {
      response = "AI endpoint must use HTTPS.";
      return false;
    }

    String root_ca = String(AI_ROOT_CA);
    if (root_ca.indexOf("YOUR_ROOT_CA_CERT") >= 0) {
      response = "Set AI_ROOT_CA certificate for TLS verification.";
      return false;
    }

    WiFiClientSecure secure_client;
    secure_client.setCACert(AI_ROOT_CA);

    HTTPClient http;
    if (!http.begin(secure_client, endpoint)) {
      response = "Failed to start HTTPS request.";
      return false;
    }
    http.setConnectTimeout(12000);
    http.setTimeout(20000);
    http.addHeader("Content-Type", "text/plain");

    String api_key = String(AI_API_KEY);
    api_key.trim();
    if (api_key.length() > 0) {
      http.addHeader("Authorization", "Bearer " + api_key);
    }

    int code = http.POST(user_prompt);
    response = "No response received.";
    bool success = false;

    if (code >= 200 && code < 300) {
      response = http.getString();
      response.trim();
      if (response.length() == 0) {
        response = "AI service returned an empty response.";
        success = false;
      } else {
        success = true;
      }
    } else if (code > 0) {
      response = "AI request failed with status: " + String(code);
    } else {
      response = "AI request failed. HTTP error: " + String(code);
    }

    http.end();
    return success;
  }

  void speak_text(const String& text) {
    Serial.println("[SPEAK] " + text);
  }

  bool wifi_is_configured() {
    String ssid = String(WIFI_SSID);
    ssid.trim();
    return ssid.length() > 0 && ssid != "YOUR_WIFI_SSID";
  }

  bool connect_wifi(uint32_t timeout_ms) {
    if (!wifi_is_configured()) {
      if (lbl_wifi_status) lv_label_set_text(lbl_wifi_status, "Wi-Fi: CONFIG NEEDED");
      return false;
    }

    WiFi.mode(WIFI_STA);
    wl_status_t wifi_status = WiFi.status();
    if (wifi_status == WL_CONNECTED) {
      wifi_connecting = false;
      if (lbl_wifi_status) lv_label_set_text(lbl_wifi_status, "Wi-Fi: ON");
      return true;
    }

    if (!wifi_connecting || (millis() - wifi_connect_started_ms > timeout_ms)) {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      wifi_connecting = true;
      wifi_connect_started_ms = millis();
    }

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) {
      service_ui_delay(50);
    }

    if (WiFi.status() == WL_CONNECTED) {
      wifi_connecting = false;
      if (lbl_wifi_status) lv_label_set_text(lbl_wifi_status, "Wi-Fi: ON");
      Serial.printf("Wi-Fi connected: %s\n", WiFi.localIP().toString().c_str());
      return true;
    }

    wifi_connecting = false;
    if (lbl_wifi_status) lv_label_set_text(lbl_wifi_status, "Wi-Fi: OFF");
    return false;
  }

  void update_avatar_state(AvatarState state, const String& text) {
    avatar_state = state;

    if (lbl_avatar_status) {
      lv_label_set_text(lbl_avatar_status, text.c_str());
    }

    if (icon_lbl) {
      switch (state) {
        case AVATAR_IDLE:
          lv_label_set_text(icon_lbl, LV_SYMBOL_MICROPHONE);
          break;
        case AVATAR_LISTENING:
          lv_label_set_text(icon_lbl, LV_SYMBOL_CALL);
          break;
        case AVATAR_THINKING:
          lv_label_set_text(icon_lbl, LV_SYMBOL_REFRESH);
          break;
        case AVATAR_SPEAKING:
          lv_label_set_text(icon_lbl, LV_SYMBOL_AUDIO);
          break;
        case AVATAR_ERROR:
          lv_label_set_text(icon_lbl, LV_SYMBOL_WARNING);
          break;
      }
    }

    if (ai_icon) {
      uint32_t color = COLOR_PRIMARY;
      if (state == AVATAR_LISTENING) color = COLOR_SUCCESS;
      if (state == AVATAR_THINKING) color = COLOR_WARNING;
      if (state == AVATAR_ERROR) color = COLOR_ERROR;
      lv_obj_set_style_bg_color(ai_icon, lv_color_hex(color), 0);
    }

    if (lbl_ai_status) {
      if (state == AVATAR_IDLE) lv_label_set_text(lbl_ai_status, "AI: READY");
      else if (state == AVATAR_ERROR) lv_label_set_text(lbl_ai_status, "AI: ERROR");
      else lv_label_set_text(lbl_ai_status, "AI: BUSY");
    }
  }

  void run_ask_ai_flow() {
    if (!wifi_is_configured()) {
      update_avatar_state(AVATAR_ERROR, "Wi-Fi config needed");
      return;
    }

    if (WiFi.status() != WL_CONNECTED && !connect_wifi(10000)) {
      update_avatar_state(AVATAR_ERROR, "No Wi-Fi");
      return;
    }

    update_avatar_state(AVATAR_LISTENING, "Listening...");
    String user_prompt = capture_user_input_from_serial();
    if (user_prompt.length() == 0) {
      update_avatar_state(AVATAR_ERROR, "No input detected");
      return;
    }

    Serial.println("[USER] " + user_prompt);
    update_avatar_state(AVATAR_THINKING, "Thinking...");

    String response;
    bool ai_ok = ask_ai_service(user_prompt, response);
    Serial.println("[AI] " + response);

    if (!ai_ok) {
      update_avatar_state(AVATAR_ERROR, response);
      return;
    }

    update_avatar_state(AVATAR_SPEAKING, "Speaking...");
    speak_text(response);
    service_ui_delay(1200);

    update_avatar_state(AVATAR_IDLE, "Tap ASK AI");
  }

  void service_ui_delay(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
      lv_timer_handler();
      delay(5);
    }
  }
}

// ==================== BUTTON CALLBACKS ====================

static void btn_ask_ai_clicked(lv_event_t * e) {
  Serial.println("Button clicked: ASK AI");
  ask_ai_requested = true;
}

static void btn_estimate_clicked(lv_event_t * e) {
  Serial.println("Button clicked: ESTIMATE");
  update_avatar_state(AVATAR_IDLE, "Tap ASK AI");
}

static void btn_business_clicked(lv_event_t * e) {
  Serial.println("Button clicked: BUSINESS");
  update_avatar_state(AVATAR_IDLE, "Tap ASK AI");
}

static void btn_settings_clicked(lv_event_t * e) {
  Serial.println("Button clicked: SETTINGS");
  update_avatar_state(AVATAR_IDLE, "Set Wi-Fi + AI URL");
}

// ==================== LVGL INITIALIZATION ====================

void lvgl_init() {
  Serial.println("Initializing LVGL 8.3.x...");
  
  lv_init();
  
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_BUF_LEN);
  
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.draw_buf = &draw_buf;
  disp_drv.flush_cb = lv_disp_flush;
  disp_drv.hor_res = LVGL_WIDTH;
  disp_drv.ver_res = LVGL_HEIGHT;
  disp_drv.sw_rotate = 0;
  disp_drv.rotated = LV_DISP_ROT_NONE;
  lv_disp_drv_register(&disp_drv);
  
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lv_touch_read;
  lv_indev_drv_register(&indev_drv);
  
  Serial.println("LVGL initialized");
}

// ==================== UI CREATION ====================

void create_main_ui() {
  Serial.println("Creating main UI...");
  
  lv_obj_t * scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_DARK_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_100, 0);
  
  // Header
  lv_obj_t * header = lv_obj_create(scr);
  lv_obj_set_size(header, LVGL_WIDTH, 80);
  lv_obj_set_pos(header, 0, 0);
  lv_obj_set_style_bg_color(header, lv_color_hex(COLOR_PRIMARY_DARK), 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_pad_all(header, 10, 0);
  
  // Title
  lv_obj_t * lbl_title = lv_label_create(header);
  lv_label_set_text(lbl_title, "ELIO AI");
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 5);
  
  // Subtitle
  lv_obj_t * lbl_subtitle = lv_label_create(header);
  lv_label_set_text(lbl_subtitle, "Your Smart Assistant");
  lv_obj_set_style_text_font(lbl_subtitle, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_subtitle, lv_color_hex(COLOR_TEXT_SECONDARY), 0);
  lv_obj_align(lbl_subtitle, LV_ALIGN_BOTTOM_MID, 0, -5);
  
  // Center area with icon
  lv_obj_t * center_area = lv_obj_create(scr);
  lv_obj_set_size(center_area, LVGL_WIDTH, 120);
  lv_obj_set_pos(center_area, 0, 85);
  lv_obj_set_style_bg_color(center_area, lv_color_hex(COLOR_DARK_BG), 0);
  lv_obj_set_style_border_width(center_area, 0, 0);
  lv_obj_set_style_pad_all(center_area, 0, 0);
  
  // AI Icon
  ai_icon = lv_obj_create(center_area);
  lv_obj_set_size(ai_icon, 100, 100);
  lv_obj_set_style_radius(ai_icon, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(ai_icon, lv_color_hex(COLOR_PRIMARY), 0);
  lv_obj_set_style_border_width(ai_icon, 0, 0);
  lv_obj_align(ai_icon, LV_ALIGN_TOP_MID, 0, 5);
  
  // Microphone symbol
  icon_lbl = lv_label_create(ai_icon);
  lv_label_set_text(icon_lbl, LV_SYMBOL_MICROPHONE);
  lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(icon_lbl, lv_color_hex(COLOR_WHITE), 0);
  lv_obj_align(icon_lbl, LV_ALIGN_CENTER, 0, 0);

  lbl_avatar_status = lv_label_create(center_area);
  lv_label_set_text(lbl_avatar_status, "Tap ASK AI");
  lv_obj_set_style_text_font(lbl_avatar_status, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_avatar_status, lv_color_hex(COLOR_TEXT_SECONDARY), 0);
  lv_obj_align(lbl_avatar_status, LV_ALIGN_BOTTOM_MID, 0, -5);
  
  // Button area
  lv_obj_t * btn_area = lv_obj_create(scr);
  lv_obj_set_size(btn_area, LVGL_WIDTH, 135);
  lv_obj_set_pos(btn_area, 0, 210);
  lv_obj_set_style_bg_color(btn_area, lv_color_hex(COLOR_DARK_BG), 0);
  lv_obj_set_style_border_width(btn_area, 0, 0);
  lv_obj_set_style_pad_all(btn_area, 8, 0);
  lv_obj_set_flex_flow(btn_area, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(btn_area, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  
  uint16_t btn_width = (LVGL_WIDTH - 24) / 2;
  uint16_t btn_height = 55;
  
  // Button 1: ASK AI
  lv_obj_t * btn_ask = lv_btn_create(btn_area);
  lv_obj_set_size(btn_ask, btn_width, btn_height);
  lv_obj_set_style_bg_color(btn_ask, lv_color_hex(COLOR_PRIMARY), 0);
  lv_obj_set_style_bg_color(btn_ask, lv_color_hex(0x0673), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(btn_ask, 0, 0);
  lv_obj_set_style_radius(btn_ask, 8, 0);
  
  lv_obj_t * lbl_ask = lv_label_create(btn_ask);
  lv_label_set_text(lbl_ask, "ASK AI");
  lv_obj_set_style_text_font(lbl_ask, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lbl_ask, lv_color_hex(COLOR_WHITE), 0);
  lv_obj_center(lbl_ask);
  lv_obj_add_event_cb(btn_ask, btn_ask_ai_clicked, LV_EVENT_CLICKED, NULL);
  
  // Button 2: ESTIMATE
  lv_obj_t * btn_estimate = lv_btn_create(btn_area);
  lv_obj_set_size(btn_estimate, btn_width, btn_height);
  lv_obj_set_style_bg_color(btn_estimate, lv_color_hex(COLOR_PRIMARY), 0);
  lv_obj_set_style_bg_color(btn_estimate, lv_color_hex(0x0673), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(btn_estimate, 0, 0);
  lv_obj_set_style_radius(btn_estimate, 8, 0);
  
  lv_obj_t * lbl_estimate = lv_label_create(btn_estimate);
  lv_label_set_text(lbl_estimate, "ESTIMATE");
  lv_obj_set_style_text_font(lbl_estimate, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lbl_estimate, lv_color_hex(COLOR_WHITE), 0);
  lv_obj_center(lbl_estimate);
  lv_obj_add_event_cb(btn_estimate, btn_estimate_clicked, LV_EVENT_CLICKED, NULL);
  
  // Button 3: BUSINESS
  lv_obj_t * btn_business = lv_btn_create(btn_area);
  lv_obj_set_size(btn_business, btn_width, btn_height);
  lv_obj_set_style_bg_color(btn_business, lv_color_hex(COLOR_PRIMARY), 0);
  lv_obj_set_style_bg_color(btn_business, lv_color_hex(0x0673), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(btn_business, 0, 0);
  lv_obj_set_style_radius(btn_business, 8, 0);
  
  lv_obj_t * lbl_business = lv_label_create(btn_business);
  lv_label_set_text(lbl_business, "BUSINESS");
  lv_obj_set_style_text_font(lbl_business, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lbl_business, lv_color_hex(COLOR_WHITE), 0);
  lv_obj_center(lbl_business);
  lv_obj_add_event_cb(btn_business, btn_business_clicked, LV_EVENT_CLICKED, NULL);
  
  // Button 4: SETTINGS
  lv_obj_t * btn_settings = lv_btn_create(btn_area);
  lv_obj_set_size(btn_settings, btn_width, btn_height);
  lv_obj_set_style_bg_color(btn_settings, lv_color_hex(COLOR_PRIMARY), 0);
  lv_obj_set_style_bg_color(btn_settings, lv_color_hex(0x0673), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(btn_settings, 0, 0);
  lv_obj_set_style_radius(btn_settings, 8, 0);
  
  lv_obj_t * lbl_settings = lv_label_create(btn_settings);
  lv_label_set_text(lbl_settings, "SETTINGS");
  lv_obj_set_style_text_font(lbl_settings, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lbl_settings, lv_color_hex(COLOR_WHITE), 0);
  lv_obj_center(lbl_settings);
  lv_obj_add_event_cb(btn_settings, btn_settings_clicked, LV_EVENT_CLICKED, NULL);
  
  // Status bar
  lv_obj_t * status_bar = lv_obj_create(scr);
  lv_obj_set_size(status_bar, LVGL_WIDTH, 35);
  lv_obj_set_pos(status_bar, 0, LVGL_HEIGHT - 35);
  lv_obj_set_style_bg_color(status_bar, lv_color_hex(COLOR_PRIMARY_DARK), 0);
  lv_obj_set_style_border_width(status_bar, 0, 0);
  lv_obj_set_style_pad_hor(status_bar, 5, 0);
  lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  
  // Wi-Fi status
  lbl_wifi_status = lv_label_create(status_bar);
  lv_label_set_text(lbl_wifi_status, "Wi-Fi: OFF");
  lv_obj_set_style_text_font(lbl_wifi_status, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_wifi_status, lv_color_hex(COLOR_TEXT_SECONDARY), 0);
  
  // AI status
  lbl_ai_status = lv_label_create(status_bar);
  lv_label_set_text(lbl_ai_status, "AI: READY");
  lv_obj_set_style_text_font(lbl_ai_status, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_ai_status, lv_color_hex(COLOR_TEXT_SECONDARY), 0);
  
  // Ready status
  lv_obj_t * lbl_ready = lv_label_create(status_bar);
  lv_label_set_text(lbl_ready, "Ready");
  lv_obj_set_style_text_font(lbl_ready, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_ready, lv_color_hex(COLOR_SUCCESS), 0);
  
  Serial.println("Main UI created");
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  ELIO AI ASSISTANT - PHASE 1");
  Serial.println("  Display: ST7789 (240x320)");
  Serial.println("  Interface: LVGL 8.3.x");
  Serial.println("========================================");
  Serial.println();
  
  print_chip_info();
  print_memory();
  Serial.println();
  
  // Initialize display
  Serial.println("Initializing hardware...");
  display_init();
  backlight_init();
  display_backlight_set(100);
  delay(500);
  
  // Initialize touch
  touch_init();
  delay(200);
  
  // Initialize LVGL
  lvgl_init();
  
  // Create UI
  create_main_ui();
  update_avatar_state(AVATAR_IDLE, "Tap ASK AI");
  if (!wifi_is_configured() && lbl_wifi_status) {
    lv_label_set_text(lbl_wifi_status, "Wi-Fi: CONFIG NEEDED");
  }
  
  Serial.println();
  Serial.println("ELIO AI Assistant Phase 1 initialized successfully!");
  Serial.println("Waiting for user interaction...");
  Serial.println();
}

// ==================== LOOP ====================

void loop() {
  static uint32_t last_lvgl_update = 0;
  uint32_t now = millis();
  
  // Update LVGL
  if (now - last_lvgl_update >= LVGL_TICK_PERIOD_MS) {
    lv_timer_handler();
    last_lvgl_update = now;
  }
  
  // Periodic memory check
  static uint32_t last_memory_check = 0;
  if (now - last_memory_check > 10000) {
    print_memory();
    last_memory_check = now;
  }

  // Periodic Wi-Fi reconnect
  if (!ask_ai_requested &&
      wifi_is_configured() &&
      WiFi.status() != WL_CONNECTED &&
      (last_wifi_retry_ms == 0 || now - last_wifi_retry_ms > 30000)) {
    last_wifi_retry_ms = now;
    connect_wifi(3000);
  }

  if (ask_ai_requested) {
    ask_ai_requested = false;
    run_ask_ai_flow();
  }
  
  delay(5);
}

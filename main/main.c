#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sh1106.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "MAIN";

// Pin definitions - adjust these according to your wiring
#define I2C_MASTER_SCL_IO 22      // GPIO for I2C SCL
#define I2C_MASTER_SDA_IO 21      // GPIO for I2C SDA
#define I2C_MASTER_FREQ_HZ 400000 // I2C frequency

// Array of body texts to rotate through
static const char *body_texts[] = {"Message 1",   "Message 2",   "Message 3",
                                   "Hello ESP32", "SH1106 OLED", "Dynamic Text",
                                   "Rotating...", "ESP-IDF Demo"};

#define BODY_TEXT_COUNT (sizeof(body_texts) / sizeof(body_texts[0]))

void app_main(void) {
  ESP_LOGI(TAG, "Starting SH1106 OLED Display Demo");

  // Initialize display handle
  sh1106_handle_t display;

  // Initialize display
  esp_err_t ret = sh1106_init(&display, I2C_NUM_0, I2C_MASTER_SDA_IO,
                              I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SH1106 display");
    return;
  }

  // Clear entire display
  sh1106_clear_display(&display);

  // Write fixed header text (Pages 0-1)
  sh1106_write_text(&display, SECTION_HEADER, "  HEADER  ", 0, 0);
  sh1106_write_text(&display, SECTION_HEADER, "1.3\" OLED", 0, 1);

  // Write fixed footer text (Pages 6-7)
  sh1106_write_text(&display, SECTION_FOOTER, "  FOOTER  ", 0, 0);
  sh1106_write_text(&display, SECTION_FOOTER, "ESP32-IDF", 0, 1);

  // Update display to show header and footer
  sh1106_update_display(&display);

  ESP_LOGI(TAG, "Header and Footer set");

  // Main loop - rotate body text every 2 seconds
  uint8_t text_index = 0;

  while (1) {
    // Clear body section
    sh1106_clear_section(&display, SECTION_BODY);

    // Write current body text (centered in body section)
    // Body section has 4 pages, we'll use pages 0 and 2 (relative to body)
    sh1106_write_text(&display, SECTION_BODY, body_texts[text_index], 8, 1);

    // You can add more lines in the body if needed
    // sh1106_write_text(&display, SECTION_BODY, "Line 2", 8, 2);

    // Update display
    sh1106_update_display(&display);

    ESP_LOGI(TAG, "Displaying: %s", body_texts[text_index]);

    // Move to next text
    text_index = (text_index + 1) % BODY_TEXT_COUNT;

    // Wait 2 seconds before changing
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
#include "sh1106.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sh1106_fonts.h"
#include <string.h>


static const char *TAG = "SH1106";

static esp_err_t sh1106_write_command(sh1106_handle_t *handle, uint8_t cmd) {
  uint8_t data[2] = {0x00, cmd}; // 0x00 = command mode
  return i2c_master_transmit(handle->dev_handle, data, 2,
                             SH1106_I2C_TIMEOUT_MS);
}

static esp_err_t sh1106_write_data(sh1106_handle_t *handle, uint8_t *data,
                                   size_t len) {
  // Allocate buffer for 0x40 prefix + data
  uint8_t *tx_buffer = malloc(len + 1);
  if (tx_buffer == NULL) {
    return ESP_ERR_NO_MEM;
  }

  tx_buffer[0] = 0x40; // 0x40 = data mode
  memcpy(tx_buffer + 1, data, len);

  esp_err_t ret = i2c_master_transmit(handle->dev_handle, tx_buffer, len + 1,
                                      SH1106_I2C_TIMEOUT_MS);

  free(tx_buffer);
  return ret;
}

esp_err_t sh1106_init(sh1106_handle_t *handle, gpio_num_t sda_pin,
                      gpio_num_t scl_pin, uint32_t i2c_freq) {
  esp_err_t ret;

  // Configure I2C master bus
  i2c_master_bus_config_t bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_0,
      .scl_io_num = scl_pin,
      .sda_io_num = sda_pin,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ret = i2c_new_master_bus(&bus_config, &handle->bus_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
    return ret;
  }

  // Configure I2C device (SH1106)
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = SH1106_I2C_ADDRESS,
      .scl_speed_hz = i2c_freq,
  };

  ret = i2c_master_bus_add_device(handle->bus_handle, &dev_config,
                                  &handle->dev_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
    i2c_del_master_bus(handle->bus_handle);
    return ret;
  }

  handle->current_font = sh1106_get_font(FONT_8X8_DEFAULT); // Set default font

  // Initialize display
  vTaskDelay(pdMS_TO_TICKS(100)); // Wait for display to power up

  sh1106_write_command(handle, SH1106_CMD_DISPLAY_OFF);
  sh1106_write_command(handle, SH1106_CMD_SET_CLOCK_DIV);
  sh1106_write_command(handle, 0x80);
  sh1106_write_command(handle, SH1106_CMD_SET_MULTIPLEX);
  sh1106_write_command(handle, 0x3F);
  sh1106_write_command(handle, SH1106_CMD_SET_DISPLAY_OFFSET);
  sh1106_write_command(handle, 0x00);
  sh1106_write_command(handle, 0x40); // Set start line
  sh1106_write_command(handle, SH1106_CMD_SET_CHARGE_PUMP);
  sh1106_write_command(handle, 0x14); // Enable charge pump
  sh1106_write_command(handle, SH1106_CMD_SET_SEGMENT_REMAP);
  sh1106_write_command(handle, SH1106_CMD_SET_SCAN_DIRECTION);
  sh1106_write_command(handle, SH1106_CMD_SET_COM_PINS);
  sh1106_write_command(handle, 0x12);
  sh1106_write_command(handle, SH1106_CMD_SET_CONTRAST);
  sh1106_write_command(handle, 0xCF);
  sh1106_write_command(handle, SH1106_CMD_SET_PRECHARGE);
  sh1106_write_command(handle, 0xF1);
  sh1106_write_command(handle, SH1106_CMD_SET_VCOM_DESELECT);
  sh1106_write_command(handle, 0x40);
  sh1106_write_command(handle, 0xA4); // Display RAM content
  sh1106_write_command(handle, 0xA6); // Normal display (not inverted)
  sh1106_write_command(handle, SH1106_CMD_DISPLAY_ON);

  // Clear buffer
  memset(handle->buffer, 0, sizeof(handle->buffer));

  ESP_LOGI(TAG, "SH1106 initialized successfully");
  return ESP_OK;
}

esp_err_t sh1106_deinit(sh1106_handle_t *handle) {
  esp_err_t ret;

  // Turn off display
  sh1106_write_command(handle, SH1106_CMD_DISPLAY_OFF);

  // Remove device from bus
  ret = i2c_master_bus_rm_device(handle->dev_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to remove I2C device: %s", esp_err_to_name(ret));
    return ret;
  }

  // Delete I2C master bus
  ret = i2c_del_master_bus(handle->bus_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to delete I2C bus: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "SH1106 deinitialized successfully");
  return ESP_OK;
}

esp_err_t sh1106_clear_display(sh1106_handle_t *handle) {
  memset(handle->buffer, 0, sizeof(handle->buffer));
  return sh1106_update_display(handle);
}

esp_err_t sh1106_clear_section(sh1106_handle_t *handle,
                               sh1106_section_t section) {
  uint8_t start_page, num_pages;

  switch (section) {
  case SECTION_HEADER:
    start_page = 0;
    num_pages = 3; // Pages 0-2
    break;
  case SECTION_BODY:
    start_page = 3;
    num_pages = 3; // Pages 3-5
    break;
  case SECTION_FOOTER:
    start_page = 6;
    num_pages = 2; // Pages 6-7
    break;
  default:
    return ESP_ERR_INVALID_ARG;
  }

  for (uint8_t page = start_page; page < start_page + num_pages; page++) {
    memset(handle->buffer[page], 0, SH1106_WIDTH);
  }

  return ESP_OK;
}

esp_err_t sh1106_write_text_offset(sh1106_handle_t *handle,
                                   sh1106_section_t section, const char *text,
                                   uint8_t x, uint8_t y, uint8_t v_offset) {
  uint8_t start_page;

  switch (section) {
  case SECTION_HEADER:
    start_page = 0; // Pages 0-2
    break;
  case SECTION_BODY:
    start_page = 3; // Pages 3-5
    break;
  case SECTION_FOOTER:
    start_page = 6; // Pages 6-7
    break;
  default:
    return ESP_ERR_INVALID_ARG;
  }

  // Limit vertical offset to prevent overflow
  if (v_offset > 7) {
    v_offset = 7;
  }

  const sh1106_font_t *font = handle->current_font;
  uint8_t page = start_page + y;
  uint8_t col = x;

  for (size_t i = 0; text[i] != '\0' && col < SH1106_WIDTH; i++) {
    uint8_t c = text[i];
    if (c >= font->first_char && c <= font->last_char) {
      // Calculate font data index
      uint16_t char_index = (c - font->first_char) * font->width;
      const uint8_t *char_data = font->data + char_index;

      // Copy font character to buffer with vertical offset
      for (uint8_t j = 0; j < font->width && col < SH1106_WIDTH; j++) {
        uint8_t font_data = char_data[j];

        if (v_offset > 0) {
          // Split across two pages if offset is used
          uint8_t upper_bits = font_data << v_offset;
          uint8_t lower_bits = font_data >> (8 - v_offset);

          handle->buffer[page][col] |= upper_bits;
          if (page + 1 < SH1106_PAGES) {
            handle->buffer[page + 1][col] |= lower_bits;
          }
        } else {
          handle->buffer[page][col] = font_data;
        }
        col++;
      }
    }
  }

  return ESP_OK;
}

esp_err_t sh1106_write_text(sh1106_handle_t *handle, sh1106_section_t section,
                            const char *text, uint8_t x, uint8_t y) {
  return sh1106_write_text_offset(handle, section, text, x, y, 0);
}

esp_err_t sh1106_update_display(sh1106_handle_t *handle) {
  for (uint8_t page = 0; page < SH1106_PAGES; page++) {
    // Set page address
    sh1106_write_command(handle, SH1106_CMD_SET_PAGE_ADDR | page);

    // Set column address (SH1106 has 132 columns, display starts at column 2)
    sh1106_write_command(handle, SH1106_CMD_SET_LOW_COLUMN | 0x02);
    sh1106_write_command(handle, SH1106_CMD_SET_HIGH_COLUMN | 0x00);

    // Write page data
    sh1106_write_data(handle, handle->buffer[page], SH1106_WIDTH);
  }

  return ESP_OK;
}

esp_err_t sh1106_set_contrast(sh1106_handle_t *handle, uint8_t contrast) {
  sh1106_write_command(handle, SH1106_CMD_SET_CONTRAST);
  sh1106_write_command(handle, contrast);
  return ESP_OK;
}

esp_err_t sh1106_set_font(sh1106_handle_t *handle,
                          sh1106_font_type_t font_type) {
  const sh1106_font_t *font = sh1106_get_font(font_type);
  if (font == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  handle->current_font = font;
  return ESP_OK;
}

esp_err_t sh1106_write_text_font(sh1106_handle_t *handle,
                                 sh1106_section_t section, const char *text,
                                 uint8_t x, uint8_t y,
                                 sh1106_font_type_t font_type) {
  // Temporarily change font
  const sh1106_font_t *original_font = handle->current_font;
  esp_err_t ret = sh1106_set_font(handle, font_type);
  if (ret != ESP_OK) {
    return ret;
  }

  // Write text with new font
  ret = sh1106_write_text(handle, section, text, x, y);

  // Restore original font
  handle->current_font = original_font;

  return ret;
}

esp_err_t sh1106_write_text_centered(sh1106_handle_t *handle,
                                     sh1106_section_t section, const char *text,
                                     uint8_t y) {
  if (text == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  // Calculate text width
  const sh1106_font_t *font = handle->current_font;
  size_t text_len = strlen(text);
  uint16_t text_width = text_len * font->width;

  // Calculate starting X position for centering
  uint8_t x = 0;
  if (text_width < SH1106_WIDTH) {
    x = (SH1106_WIDTH - text_width) / 2;
  }

  return sh1106_write_text(handle, section, text, x, y);
}

esp_err_t sh1106_write_text_centered_font(sh1106_handle_t *handle,
                                          sh1106_section_t section,
                                          const char *text, uint8_t y,
                                          sh1106_font_type_t font_type) {
  // Temporarily change font
  const sh1106_font_t *original_font = handle->current_font;
  esp_err_t ret = sh1106_set_font(handle, font_type);
  if (ret != ESP_OK) {
    return ret;
  }

  // Write centered text with new font
  ret = sh1106_write_text_centered(handle, section, text, y);

  // Restore original font
  handle->current_font = original_font;

  return ret;
}
#ifndef SH1106_H
#define SH1106_H

#include "driver/i2c.h"
#include "sh1106_fonts.h"
#include <stdbool.h>
#include <stdint.h>

// SH1106 Configuration
#define SH1106_WIDTH 128
#define SH1106_HEIGHT 64
#define SH1106_PAGES 8

// I2C Configuration
#define SH1106_I2C_ADDRESS 0x3C
#define SH1106_I2C_TIMEOUT_MS 1000

// Command definitions
#define SH1106_CMD_DISPLAY_OFF 0xAE
#define SH1106_CMD_DISPLAY_ON 0xAF
#define SH1106_CMD_SET_CONTRAST 0x81
#define SH1106_CMD_SET_SEGMENT_REMAP 0xA1
#define SH1106_CMD_SET_SCAN_DIRECTION 0xC8
#define SH1106_CMD_SET_MULTIPLEX 0xA8
#define SH1106_CMD_SET_DISPLAY_OFFSET 0xD3
#define SH1106_CMD_SET_CLOCK_DIV 0xD5
#define SH1106_CMD_SET_PRECHARGE 0xD9
#define SH1106_CMD_SET_COM_PINS 0xDA
#define SH1106_CMD_SET_VCOM_DESELECT 0xDB
#define SH1106_CMD_SET_CHARGE_PUMP 0x8D
#define SH1106_CMD_SET_LOW_COLUMN 0x00
#define SH1106_CMD_SET_HIGH_COLUMN 0x10
#define SH1106_CMD_SET_PAGE_ADDR 0xB0

// Display sections
typedef enum {
  SECTION_HEADER =
      0, // Pages 0-2 (24 pixels height) - allows spacing between 2 lines
  SECTION_BODY = 3,  // Pages 3-5 (24 pixels height) - 3 pages for body
  SECTION_FOOTER = 6 // Pages 6-7 (16 pixels height)
} sh1106_section_t;

// SH1106 Handle
typedef struct {
  i2c_port_t i2c_port;
  uint8_t i2c_address;
  uint8_t buffer[SH1106_PAGES][SH1106_WIDTH];
  const sh1106_font_t *current_font; // Current font selection
} sh1106_handle_t;

/**
 * @brief Initialize SH1106 display
 *
 * @param handle Pointer to SH1106 handle
 * @param i2c_port I2C port number
 * @param sda_pin SDA GPIO pin
 * @param scl_pin SCL GPIO pin
 * @param i2c_freq I2C frequency in Hz
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_init(sh1106_handle_t *handle, i2c_port_t i2c_port,
                      gpio_num_t sda_pin, gpio_num_t scl_pin,
                      uint32_t i2c_freq);

/**
 * @brief Clear entire display
 *
 * @param handle Pointer to SH1106 handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_clear_display(sh1106_handle_t *handle);

/**
 * @brief Clear specific section of display
 *
 * @param handle Pointer to SH1106 handle
 * @param section Section to clear
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_clear_section(sh1106_handle_t *handle,
                               sh1106_section_t section);

/**
 * @brief Write text to specific section with vertical offset for spacing
 *
 * @param handle Pointer to SH1106 handle
 * @param section Section to write to
 * @param text Text string to display
 * @param x X position (column)
 * @param y Y position within section (0 for first line)
 * @param v_offset Vertical pixel offset (0-7) to add spacing between lines
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_write_text_offset(sh1106_handle_t *handle,
                                   sh1106_section_t section, const char *text,
                                   uint8_t x, uint8_t y, uint8_t v_offset);

/**
 * @brief Write text to specific section
 *
 * @param handle Pointer to SH1106 handle
 * @param section Section to write to
 * @param text Text string to display
 * @param x X position (column)
 * @param y Y position within section (0 for first line)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_write_text(sh1106_handle_t *handle, sh1106_section_t section,
                            const char *text, uint8_t x, uint8_t y);

/**
 * @brief Update display with buffer content
 *
 * @param handle Pointer to SH1106 handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_update_display(sh1106_handle_t *handle);

/**
 * @brief Set contrast level
 *
 * @param handle Pointer to SH1106 handle
 * @param contrast Contrast value (0-255)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_set_contrast(sh1106_handle_t *handle, uint8_t contrast);

/**
 * @brief Set font for text rendering
 *
 * @param handle Pointer to SH1106 handle
 * @param font_type Font type to use
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_set_font(sh1106_handle_t *handle,
                          sh1106_font_type_t font_type);

/**
 * @brief Write text with specific font
 *
 * @param handle Pointer to SH1106 handle
 * @param section Section to write to
 * @param text Text string to display
 * @param x X position (column)
 * @param y Y position within section
 * @param font_type Font to use for this text
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_write_text_font(sh1106_handle_t *handle,
                                 sh1106_section_t section, const char *text,
                                 uint8_t x, uint8_t y,
                                 sh1106_font_type_t font_type);

/**
 * @brief Write centered text in section
 *
 * @param handle Pointer to SH1106 handle
 * @param section Section to write to
 * @param text Text string to display (will be centered horizontally)
 * @param y Y position within section
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_write_text_centered(sh1106_handle_t *handle,
                                     sh1106_section_t section, const char *text,
                                     uint8_t y);

/**
 * @brief Write centered text with specific font
 *
 * @param handle Pointer to SH1106 handle
 * @param section Section to write to
 * @param text Text string to display (will be centered horizontally)
 * @param y Y position within section
 * @param font_type Font to use for this text
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sh1106_write_text_centered_font(sh1106_handle_t *handle,
                                          sh1106_section_t section,
                                          const char *text, uint8_t y,
                                          sh1106_font_type_t font_type);

#endif // SH1106_H
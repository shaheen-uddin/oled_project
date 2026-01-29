#ifndef SH1106_FONTS_H
#define SH1106_FONTS_H

#include <stdint.h>

// Font types
typedef enum {
  FONT_8X8_DEFAULT = 0, // Default 8x8 font
  FONT_8X8_BOLD,        // Bold 8x8 font
  FONT_6X8_THIN,        // Thin 6x8 font (narrower characters)
  FONT_5X7_SMALL,       // Small 5x7 font
  FONT_16X16_LARGE      // Large 16x16 font (double size)
} sh1106_font_type_t;

// Font structure
typedef struct {
  const uint8_t *data; // Pointer to font data
  uint8_t width;       // Character width in pixels
  uint8_t height;      // Character height in pixels
  uint8_t first_char;  // First character in font
  uint8_t last_char;   // Last character in font
} sh1106_font_t;

// Font declarations
extern const sh1106_font_t font_8x8_default;
extern const sh1106_font_t font_8x8_bold;
extern const sh1106_font_t font_6x8_thin;
extern const sh1106_font_t font_5x7_small;

// Get font by type
const sh1106_font_t *sh1106_get_font(sh1106_font_type_t type);

#endif // SH1106_FONTS_H
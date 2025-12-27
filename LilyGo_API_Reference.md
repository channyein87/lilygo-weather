# LilyGo-EPD47 Library - Function Signatures Reference

## Text Rendering Functions

### 1. `writeln()` - Basic Text Output
```cpp
void writeln(const GFXfont *font, const char *string, int32_t *cursor_x,
             int32_t *cursor_y, uint8_t *framebuffer);
```
**Parameters:**
- `const GFXfont *font` - Font to use (e.g., `&FiraSans`)
- `const char *string` - Text string to display
- `int32_t *cursor_x` - Pointer to x position (will be updated after drawing)
- `int32_t *cursor_y` - Pointer to y position (will be updated after drawing)
- `uint8_t *framebuffer` - Framebuffer to draw to (NULL for direct drawing, but deprecated)

**Usage:**
```cpp
epd_poweron();
epd_clear();
int cursor_x = 10;
int cursor_y = 10;
writeln(&FiraSans, "Hello World!", &cursor_x, &cursor_y, NULL);
epd_poweroff_all();
```

---

### 2. `write_string()` - Multi-line Text Output
```cpp
void write_string(const GFXfont *font, const char *string, int32_t *cursor_x,
                  int32_t *cursor_y, uint8_t *framebuffer);
```
**Parameters:**
- `const GFXfont *font` - Font to use (e.g., `&FiraSans`)
- `const char *string` - Text string to display (supports newlines `\n`)
- `int32_t *cursor_x` - Pointer to x position (will be updated after drawing)
- `int32_t *cursor_y` - Pointer to y position (will be updated after drawing)
- `uint8_t *framebuffer` - Framebuffer to draw to (NULL for direct drawing, but deprecated)

**Usage:**
```cpp
int cursor_x = 10;
int cursor_y = 10;
write_string(&FiraSans, "Line 1\nLine 2", &cursor_x, &cursor_y, NULL);
```

---

### 3. `write_mode()` - Advanced Text with DrawMode
```cpp
void write_mode(const GFXfont *font, const char *string, int32_t *cursor_x,
                int32_t *cursor_y, uint8_t *framebuffer, DrawMode_t mode,
                const FontProperties *properties);
```
**Parameters:**
- `const GFXfont *font` - Font to use
- `const char *string` - Text string to display
- `int32_t *cursor_x` - Pointer to x position (will be updated after drawing)
- `int32_t *cursor_y` - Pointer to y position (will be updated after drawing)
- `uint8_t *framebuffer` - Framebuffer to draw to
- `DrawMode_t mode` - Drawing mode:
  - `BLACK_ON_WHITE` - Draw black text on white background
  - `WHITE_ON_WHITE` - "Draw with white ink" (erase effect)
  - `WHITE_ON_BLACK` - Draw white text on black background
- `const FontProperties *properties` - Font properties (colors, flags):
  - `fg_color` - Foreground color (4-bit)
  - `bg_color` - Background color (4-bit)
  - `fallback_glyph` - Unicode codepoint for missing glyphs
  - `flags` - Additional flags (e.g., `DRAW_BACKGROUND`)

**Usage:**
```cpp
FontProperties props = {
    .fg_color = 0,     // Black foreground
    .bg_color = 15,    // White background
    .flags = DRAW_BACKGROUND
};
int cursor_x = 10;
int cursor_y = 10;
write_mode(&FiraSans, "Text", &cursor_x, &cursor_y, NULL, BLACK_ON_WHITE, &props);
```

---

### 4. `get_text_bounds()` - Calculate Text Bounds
```cpp
void get_text_bounds(const GFXfont *font, const char *string, int32_t *x, int32_t *y,
                     int32_t *x1, int32_t *y1, int32_t *w, int32_t *h,
                     const FontProperties *props);
```
**Parameters:**
- `const GFXfont *font` - Font to use
- `const char *string` - Text string
- `int32_t *x` - Starting x position (input/output)
- `int32_t *y` - Starting y position (input/output)
- `int32_t *x1` - Bottom-right x coordinate (output)
- `int32_t *y1` - Bottom-right y coordinate (output)
- `int32_t *w` - Text width in pixels (output)
- `int32_t *h` - Text height in pixels (output)
- `const FontProperties *props` - Font properties (can be NULL for defaults)

**Usage:**
```cpp
int32_t x = 10, y = 10;
int32_t x1, y1, w, h;
get_text_bounds(&FiraSans, "Hello", &x, &y, &x1, &y1, &w, &h, NULL);
// Now w and h contain the text dimensions
```

---

### 5. `get_glyph()` - Get Individual Glyph
```cpp
void get_glyph(const GFXfont *font, uint32_t code_point, GFXglyph **glyph);
```
**Parameters:**
- `const GFXfont *font` - Font to use
- `uint32_t code_point` - Unicode code point (e.g., 'A' = 65)
- `GFXglyph **glyph` - Pointer to glyph pointer (output)

---

## Image Display Functions

### 1. `epd_draw_image()` - Draw Grayscale Image
```cpp
void IRAM_ATTR epd_draw_image(Rect_t area, uint8_t *data, DrawMode_t mode);
```
**Parameters:**
- `Rect_t area` - Rectangle defining position and size:
  ```cpp
  typedef struct {
      int32_t x;      // X position (0-960)
      int32_t y;      // Y position (0-540)
      int32_t width;  // Image width in pixels
      int32_t height; // Image height in pixels
  } Rect_t;
  ```
- `uint8_t *data` - Image data buffer (4-bit grayscale, 2 pixels per byte)
  - **Important:** Pixel data is packed (two pixels per byte)
  - If width is odd, add 1 padding nibble per line
  - Buffer size = `(width * height + 1) / 2` bytes
  - Range: 0-15 per pixel (0=white, 15=black)
- `DrawMode_t mode` - Drawing mode:
  - `BLACK_ON_WHITE` - Draw grayscale normally
  - `WHITE_ON_WHITE` - Erase (draw white pixels)
  - `WHITE_ON_BLACK` - Invert colors

**Usage:**
```cpp
// Create a 100x100 grayscale image
uint8_t image_data[5000];  // (100*100)/2 bytes
// ... fill image_data with grayscale values ...

epd_poweron();
epd_clear();

Rect_t area = {
    .x = 50,
    .y = 50,
    .width = 100,
    .height = 100
};
epd_draw_image(area, image_data, BLACK_ON_WHITE);
epd_poweroff_all();
```

---

### 2. `epd_draw_grayscale_image()` - Draw Grayscale (No Mode)
```cpp
void IRAM_ATTR epd_draw_grayscale_image(Rect_t area, uint8_t *data);
```
**Parameters:**
- `Rect_t area` - Rectangle defining position and size
- `uint8_t *data` - Image data buffer (4-bit grayscale, 2 pixels per byte)

**Note:** Simpler version without draw mode; uses default BLACK_ON_WHITE behavior.

---

## Display Control Functions

### 1. `epd_init()` - Initialize Display
```cpp
void epd_init();
```
**Must be called in `setup()`** to initialize the display driver.

---

### 2. `epd_poweron()` - Enable Display
```cpp
void epd_poweron();
```
Enables the display power supply before drawing.

---

### 3. `epd_poweroff_all()` - Disable Display and Save Power
```cpp
void epd_poweroff_all();
```
Disables the display power supply. **The image persists on screen.** Use this after drawing to save battery power.

---

### 4. `epd_clear()` - Clear Display
```cpp
void epd_clear();
```
Clears the entire display by flashing it white.

---

### 5. `epd_clear_area()` - Clear Region
```cpp
void epd_clear_area(Rect_t area);
```
Clears a specific rectangular area on the display.

---

## Font Types

### Available Fonts
The library includes the **FiraSans** font, declared in `firasans.h`:
```cpp
extern const GFXfont FiraSans;
```

### GFXfont Structure
```cpp
typedef struct {
    uint8_t *bitmap;              // Glyph bitmaps, concatenated
    GFXglyph *glyph;              // Glyph array
    UnicodeInterval *intervals;   // Valid unicode intervals
    uint32_t interval_count;      // Number of unicode intervals
    bool compressed;              // Glyph bitmaps are zlib-compressed
    uint8_t advance_y;            // Newline distance (y axis)
    int32_t ascender;             // Maximal height above baseline
    int32_t descender;            // Maximal height below baseline
} GFXfont;
```

---

## Display Constants

```cpp
#define EPD_WIDTH  960  // Display width in pixels
#define EPD_HEIGHT 540  // Display height in pixels
```

---

## Key Data Types

### DrawMode_t
```cpp
typedef enum {
    BLACK_ON_WHITE = 1 << 0,  // Draw black/grayscale on white
    WHITE_ON_WHITE = 1 << 1,  // "White ink" erase effect
    WHITE_ON_BLACK = 1 << 2,  // White on black display
} DrawMode_t;
```

### FontProperties
```cpp
typedef struct {
    uint8_t fg_color: 4;       // Foreground color (0-15)
    uint8_t bg_color: 4;       // Background color (0-15)
    uint32_t fallback_glyph;   // Fallback for missing glyphs
    uint32_t flags;            // Flags (e.g., DRAW_BACKGROUND)
} FontProperties;
```

### DrawFlags
```cpp
enum DrawFlags {
    DRAW_BACKGROUND = 1 << 0,  // Draw background around text
};
```

---

## Complete Example

```cpp
#include "epd_driver.h"
#include "firasans.h"

void setup() {
    epd_init();
    epd_poweron();
    epd_clear();
    
    // Draw text
    int cursor_x = 10;
    int cursor_y = 10;
    writeln(&FiraSans, "Welcome!", &cursor_x, &cursor_y, NULL);
    
    // Draw more text with custom properties
    FontProperties props = {
        .fg_color = 0,
        .bg_color = 15,
        .flags = DRAW_BACKGROUND
    };
    cursor_x = 10;
    cursor_y = 100;
    write_mode(&FiraSans, "With Background", &cursor_x, &cursor_y, NULL, 
               BLACK_ON_WHITE, &props);
    
    epd_poweroff_all();
}

void loop() {
    // Update display every 30 seconds
    delay(30000);
    epd_poweron();
    epd_clear();
    // ... update display ...
    epd_poweroff_all();
}
```

---

## Important Notes

1. **Display is Slow**: Full refresh takes 1-3 seconds. Don't refresh more than 1-2× per minute.
2. **1-bit B&W**: The display is black and white only. Grayscale (0-15 values) will be dithered or converted to B&W.
3. **Framebuffer**: Most functions accept a `framebuffer` parameter. Passing `NULL` uses direct drawing (older behavior).
4. **Power Management**: Always call `epd_poweroff_all()` after drawing to save battery. The image persists on the e-paper display.
5. **Cursor is Updated**: `writeln()` and `write_string()` modify `cursor_x` and `cursor_y` so you can chain text output.


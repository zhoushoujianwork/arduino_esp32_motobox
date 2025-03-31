#define USER_SETUP_ID 147

#ifdef USER_SETUP_INFO
#undef USER_SETUP_INFO
#define USER_SETUP_INFO "ESP32-S3-Touch-LCD-1.47"
#endif

#define ST7789_DRIVER

#define CGRAM_OFFSET
// #define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue
#define TFT_RGB_ORDER TFT_BGR // Colour order Blue-Green-Red

#define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

#define TFT_WIDTH 172
#define TFT_HEIGHT 320

#define TFT_MISO -1
#define TFT_MOSI 13
#define TFT_SCLK 12
#define TFT_CS 11 // Chip select control pin
#define TFT_DC 10 // Data Command control pin
#define TFT_RST 14
#define TFT_BL 9
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD  // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2 // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4 // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6 // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7 // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8 // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT
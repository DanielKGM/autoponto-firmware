#define USER_SETUP_INFO "User_Setup"
#define GC9A01_DRIVER
#define TFT_SDA_READ      // This option is for ESP32 ONLY, tested with ST7789 and GC9A01 display only
#define TFT_HEIGHT 240 // GC9A01 240 x 240
#define TFT_MOSI 13 // In some display driver board, it might be written as "SDA" and so on.	
#define TFT_SCLK 14
#define TFT_CS   2  // Chip select control pin
#define TFT_DC   12  // Data Command control pin
#define TFT_RST  15  // Reset pin (could connect to Arduino RESET pin)
#define TOUCH_CS 21     // Chip select pin (T_CS) of touch screen
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define SMOOTH_FONT
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
#define USE_HSPI_PORT
#define SUPPORT_TRANSACTIONS

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Prototype Pins
#define PIR               33
#define BUZZER            3
#define DISPLAY_ENABLE    1
#define BUZZER_INTERVAL_MS 10000   // 10 segundos
#define BUZZER_ON_TIME_MS  2000    // apita por 2 segundos

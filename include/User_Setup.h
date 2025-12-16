// =======================================================
//  USER_SETUP DA BIBLIOTECA TFT_ESPI (Bodmer)
//  I.E., CONFIGURAÇÃO DO DISPLAY
// =======================================================
#define USER_SETUP_INFO "User_Setup"


// =======================================================
//  DRIVER DO DISPLAY
// =======================================================
// Driver para display redondo GC9A01 (240x240)
#define GC9A01_DRIVER

// Permite leitura via SDA para o GC9A01
#define TFT_SDA_READ


// =======================================================
//  RESOLUÇÃO DO DISPLAY
// =======================================================
#define TFT_HEIGHT 240


// =======================================================
//  PINAGEM SPI DO DISPLAY
// =======================================================
// Em algumas placas o pino MOSI aparece como "SDA"
#define TFT_MOSI 13
#define TFT_SCLK 14

// Chip Select do display
#define TFT_CS   2

// Data / Command
#define TFT_DC   12

// Reset do display (pode ser ligado ao RESET do ESP32)
#define TFT_RST  15


// =======================================================
//  TOUCH SCREEN
// =======================================================
// Chip Select do touch (T_CS) - Não suportado pelo GC9A01
#define TOUCH_CS 21


// =======================================================
//  FONTES
// =======================================================
// Fonte padrão Adafruit (8px)
#define LOAD_GLCD

// Fonte pequena (16px)
#define LOAD_FONT2

// Fonte média (26px)
#define LOAD_FONT4

// Fonte grande (48px)
#define LOAD_FONT6

// Fonte estilo 7 segmentos (48px)
#define LOAD_FONT7

// Fonte muito grande (75px)
#define LOAD_FONT8

// FreeFonts da Adafruit (FF1 a FF48)
#define LOAD_GFXFF

// Suporte a fontes suaves (.vlw)
#define SMOOTH_FONT


// =======================================================
//  CONFIGURAÇÕES DE SPI (frequência)
// =======================================================
// Frequência SPI principal (display)
#define SPI_FREQUENCY        27000000

// Frequência SPI para leitura do display
#define SPI_READ_FREQUENCY   20000000

// Frequência SPI do touch
#define SPI_TOUCH_FREQUENCY  2500000


// =======================================================
//  PORTA SPI E TRANSAÇÕES
// =======================================================
// Usa a porta HSPI do ESP32
#define USE_HSPI_PORT

// Habilita transações SPI (mais seguro com multitarefas)
#define SUPPORT_TRANSACTIONS

// =======================================================
//  CONFIGURAÇÃO DA CÂMERA
// =======================================================


// =======================================================
//  CONTROLE E CLOCK
// =======================================================
// Power Down da câmera
#define PWDN_GPIO_NUM     32

// Reset da câmera (-1 = não utilizado)
#define RESET_GPIO_NUM    -1

// Clock externo da câmera (XCLK)
#define XCLK_GPIO_NUM      0


// =======================================================
//  BARRAMENTO SCCB (I2C DA CÂMERA)
// =======================================================
// Dados SCCB
#define SIOD_GPIO_NUM     26

// Clock SCCB
#define SIOC_GPIO_NUM     27


// =======================================================
//  BARRAMENTO DE DADOS (DVP)
// =======================================================
// Dados de imagem
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5


// =======================================================
//  SINCRONISMO DE VÍDEO
// =======================================================
// Sincronismo vertical
#define VSYNC_GPIO_NUM    25

// Referência horizontal
#define HREF_GPIO_NUM     23

// Clock de pixel
#define PCLK_GPIO_NUM     22

// Sensor de presença (PIR)
#define PIR_PIN 33
#define SLEEP_TIMEOUT_MS 20000 // Tempo sem detecção PIR para entrar em deep sleep (20 segundos)

// Configurações do display TFT
#define DISPLAY_ENABLE_PIN 1     // Pino para ativar/desativar transistor associado ao display
#define DISPLAY_WIDTH 240        // Largura do display em pixels
#define DISPLAY_HEIGHT 240       // Altura do display em pixels
#define MESSAGE_INTERVAL_MS 2000 // Tempo mínimo de exibição de mensagens
#define DISPLAY_MSG_MAX_LEN 64   // Tamanho máximo da mensagem exibida no display

// Feedback positivo
#define POSITIVE_FB_PIN 3 // Liga o buzzer ativo e o LED indicador
#define POSITIVE_FB_DURATION_MS 500

// Rede
// Algumas definições de rede foram movidas para o arquivo Secrets.h
#define DATA_SEND_INTERVAL_MS 3000 // Intervalo entre envios de frames em milissegundos
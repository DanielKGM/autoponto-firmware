#pragma once

//  Sensor de presença (PIR)
#define PIR_PIN 33
#define SLEEP_TIMEOUT_MS 20000 // Tempo sem detecção PIR para entrar em deep sleep (20 segundos)

//  Configurações do display TFT
#define DISPLAY_ENABLE_PIN 1   // Pino para ativar/desativar transistor associado ao display
#define DISPLAY_WIDTH 240      // Largura do display em pixels
#define DISPLAY_HEIGHT 240     // Altura do display em pixels
#define DISPLAY_MSG_MAX_LEN 64 // Tamanho máximo da mensagem exibida no display

//  Feedback positivo
#define POSITIVE_FB_PIN 3 // Liga o buzzer ativo e o LED indicador
#define POSITIVE_FB_DURATION_MS 1000

//      WiFi
#define WIFI_TIMEOUT_MS 10000
#define WIFI_SSID ""
#define WIFI_PASS ""
#define HOST_NAME ""

//      Servidor REST
#define REST_URL ""
#define REST_PASS ""
#define REST_TIMEOUT_MS 8000
#define REST_POST_INTERVAL_MS 3000 // Intervalo entre envios de frames em milissegundos

//      Servidor MQTT
#define MQTT_TIMEOUT_MS 10000
#define MQTT_LOG_INTERVAL_MS 5000
#define MQTT_URL ""
#define MQTT_PORT 0000
#define MQTT_USER ""
#define MQTT_PASS ""
#define MQTT_LOG_DEFAULT true

//      Servidor REST + MQTT
#define RESPONSE_WAIT_TIMEOUT_MS 5000 // Tempo de espera, por MQTT, de uma resposta após POST REST
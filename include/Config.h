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
#define POSITIVE_FB_DURATION_MS 500

//  Rede
#define POST_INTERVAL_MS 3000            // Intervalo entre envios de frames em milissegundos
#define MESSAGE_PUBLISH_INTERVAL_MS 3000 // Invervale entre envio de mensagens MQTT, em milissegundos

//      WiFi
#define CONN_WAIT_INTERVAL_MS 1000 // Intervalo para esperar conexão
#define WIFI_SSID "GAEL"
#define WIFI_PASS "mimita1981"
#define HOST_NAME "esp32cam-autotoponto"

//      Servidor REST
#define POST_URL "http://192.168.15.6:5000/upload"
#define REST_PASS ""

//      Servidor MQTT
#define MQTT_URL ""
#define MQTT_PORT 1927
#define MQTT_USER ""
#define MQTT_PASS ""
#define MQTT_LOG_DEFAULT true
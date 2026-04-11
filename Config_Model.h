#pragma once

// ===== Valores Não Configuráveis por Bluetooth =====

//  Sensor de presença (PIR)
#define PIR_PIN 33
#define SLEEP_TIMEOUT_MS 300000 // Tempo sem detecção PIR para entrar em deep sleep (5 minutos)
#define IDLE_TIMEOUT_MS 60000   // Tempo sem atividade para entrar em modo de espera (1 minuto)

//  Configurações do display TFT
#define DISPLAY_ENABLE_PIN 1   // Pino para ativar/desativar transistor associado ao display
#define DISPLAY_WIDTH 240      // Largura do display em pixels
#define DISPLAY_HEIGHT 240     // Altura do display em pixels
#define DISPLAY_MSG_MAX_LEN 64 // Tamanho máximo da mensagem exibida no display

//  Feedback positivo
#define POSITIVE_FB_PIN 3 // Liga o buzzer ativo e o LED indicador
#define POSITIVE_FB_DURATION_MS 1000

// Bluetooth
#define CONFIG_PIN 4
#define CONFIG_TIMEOUT_MS 180000
#define BLUETOOTH_TOKEN ""

//      WiFi
#define WIFI_TIMEOUT_MS 10000

//      Servidor REST
#define REST_TIMEOUT_MS 8000
#define REST_POST_INTERVAL_MS 3000 // Intervalo entre envios de frames em milissegundos

//      Servidor REST + MQTT
#define RESPONSE_WAIT_TIMEOUT_MS 5000 // Tempo de espera, por MQTT, de uma resposta após POST REST

// ===== Valores-Padrão Configuráveis por Bluetooth =====

//      WiFi
#define WIFI_SSID ""
#define WIFI_PASS ""

//      Servidor REST
#define REST_URL ""
#define REST_PASS ""
#define REST_POST_PATH ""
#define REST_FETCH_PATH ""

//      Servidor MQTT
#define MQTT_URL ""
#define MQTT_PORT 0
#define MQTT_USER ""
#define MQTT_PASS ""
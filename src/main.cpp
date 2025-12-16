#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
#include <TFT_eSPI.h>          // Hardware-specific library
#include <SPI.h>
#include "User_Setup.h"
#include "Camera_Setup.h"
#include "System_Setup.h"

// 1. TEMPO DE EXIBIÇÃO E VARIÁVEIS DE CONTROLE
volatile bool should_sleep_flag = false;
volatile unsigned long next_sleep_time = 0; 
// Não precisamos do pirMux e da ISR complexa se monitorarmos o PIR na Task
// Vamos usar uma flag simples para checar se a Task deve reiniciar o timer
volatile bool pir_event_flag = false; 

TaskHandle_t Task1;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
camera_config_t config;
uint16_t *scr;

// 2. INTERRUPT SERVICE ROUTINE (ISR) SIMPLIFICADA
// A ISR apenas informa à Task1code que houve um evento no pino PIR.
void IRAM_ATTR handlePIRInterrupt() {
  pir_event_flag = true;
}

void Task1code( void * pvParameters ) {
  // Inicialização do Display (Deve ser a primeira coisa)
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Alterado para branco no fundo preto para melhor visibilidade
  tft.setTextDatum(MC_DATUM); // Centraliza texto
  scr = (uint16_t*)spr.createSprite(240, 240);
  spr.setTextDatum(MC_DATUM); 
  spr.setSwapBytes(true);

  // Configuração da Câmera (mantida)
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_240X240;
  config.pixel_format = PIXFORMAT_RGB565;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // 2. LÓGICA DE RESET DE HARDWARE ANTES DA INICIALIZAÇÃO
  if (config.pin_pwdn != -1) {
    pinMode(config.pin_pwdn, OUTPUT);
    digitalWrite(config.pin_pwdn, LOW); 
  }
  if (config.pin_reset != -1) {
    pinMode(config.pin_reset, OUTPUT);
    digitalWrite(config.pin_reset, LOW);
    delay(10);
    digitalWrite(config.pin_reset, HIGH); 
    delay(10);
  }

  // Inicialização da Câmera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    tft.drawString("Camera FAILED", 120, 120, 2); 
    vTaskDelete(NULL);
    return;
  }

  tft.drawString("Loading...", 120, 120, 2);

  // 3. LOOP DE CAPTURA COM LÓGICA DE TEMPO E EXIBIÇÃO
  while (!should_sleep_flag) {
    
    // 3A. VERIFICAÇÃO E RENOVAÇÃO DO TIMER
    
    // Leitura do pino PIR (feita com segurança na Task)
    bool pir_is_high = digitalRead(PIR);
    
    // Se o PIR estiver em HIGH, REINICIA O TIMER
    if (pir_is_high) {
        next_sleep_time = millis() + DISPLAY_TIMEOUT_MS;
    }

    // CÁLCULO DE TEMPO RESTANTE
    unsigned long time_left = 0;
    if (next_sleep_time < millis()) {
      should_sleep_flag = true; 
      continue;
    }
    
    // 3B. CAPTURA E PROCESSAMENTO DA IMAGEM
    camera_fb_t  * fb = esp_camera_fb_get();

    if (!fb) {
      vTaskDelay(pdMS_TO_TICKS(50)); 
      continue;
    }

    memcpy(scr, fb->buf, 57600 * 2); 
    
    spr.pushSprite(0, 0); 
    
    esp_camera_fb_return(fb); 
    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
  
  // 4. LIMPEZA (CLEANUP)
  esp_camera_deinit();
  vTaskDelete(NULL); 
}

void TaskBuzzerCode(void * pvParameters) {

  unsigned long lastBuzz = millis();

  while (!should_sleep_flag) {

    unsigned long now = millis();

    // Verifica se passaram 10 segundos
    if (now - lastBuzz >= BUZZER_INTERVAL_MS) {
      lastBuzz = now;

      // Liga o buzzer
      digitalWrite(POSITIVE_FEEDBACK, HIGH);
      vTaskDelay(pdMS_TO_TICKS(BUZZER_ON_TIME_MS));
      digitalWrite(POSITIVE_FEEDBACK, LOW);
    }

    // Pequeno delay para não monopolizar a CPU
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  // Garante que o buzzer fique desligado ao sair
  digitalWrite(POSITIVE_FEEDBACK, LOW);

  vTaskDelete(NULL);
}
  
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

  pinMode(DISPLAY_ENABLE, OUTPUT);
  pinMode(PIR, INPUT);
  pinMode(POSITIVE_FEEDBACK, OUTPUT);
  digitalWrite(DISPLAY_ENABLE, HIGH);
  
  // 5. INICIALIZAÇÃO E INTERRUPÇÃO
  
  // Define o tempo inicial de sleep (10 segundos)
  next_sleep_time = millis() + DISPLAY_TIMEOUT_MS; 

  // Usamos a interrupção CHANGE para que a Task1code seja ativada rapidamente
  // sempre que o PIR mudar de estado (HIGH/LOW).
  attachInterrupt(digitalPinToInterrupt(PIR), handlePIRInterrupt, CHANGE);

  xTaskCreatePinnedToCore(
    Task1code,   
    "Task1",     
    100000,      
    NULL,        
    1,           
    &Task1,      
    0);     

  xTaskCreatePinnedToCore(
    TaskBuzzerCode,
    "TaskBuzzer",
    2048,
    NULL,
    1,
    NULL,
    1);     

  // Configuração para o wakeup
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);
  rtc_gpio_pullup_dis(GPIO_NUM_33);
  rtc_gpio_pulldown_en(GPIO_NUM_33);

  // 6. LOOP QUE ESPERA A TASK TERMINAR
  while (!should_sleep_flag) {
      // Pequeno atraso para dar tempo para a Task1code e o scheduler.
      vTaskDelay(pdMS_TO_TICKS(100)); 
  }
  
  // 7. FINALIZAÇÃO ANTES DO DEEP SLEEP
  detachInterrupt(digitalPinToInterrupt(PIR)); 
  digitalWrite(DISPLAY_ENABLE, LOW); 
  
  vTaskDelay(pdMS_TO_TICKS(500)); 
  
  esp_deep_sleep_start();
} 
 
void loop() {
 
}
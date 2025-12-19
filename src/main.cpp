// DRIVER ESP32 CAM
#include "esp_camera.h"
#include "Arduino.h"

// RISCO DE BROWNOUT
#include "soc/soc.h"

// RTC para DEEP SLEEP
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"

#include <EEPROM.h>
#include <TFT_eSPI.h>
#include <SPI.h>

#include "Camera_Setup.h"
#include "System_Setup.h"

volatile bool should_sleep_flag = false;
volatile TickType_t last_PIR_tick = 0;
volatile bool PIR_triggered = false;
volatile int tasks_alive = 0;
portMUX_TYPE tasks_alive_mux = portMUX_INITIALIZER_UNLOCKED;
const TickType_t ticks_to_sleep = pdMS_TO_TICKS(SLEEP_TIMEOUT_MS);

TaskHandle_t TaskDisplay;

camera_config_t camera_config;

// Biblioteca TFT_eSPI de Bodmer
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
uint16_t *spr_pixel_buffer; // Buffer de pixels do sprite, destino da imagem da câmera

// Interrupt Service Routine (ISR) para detectar borda de descida no sinal do PIR
void IRAM_ATTR handlePIRInterrupt()
{
    PIR_triggered = true;
}

// Configuração do display TFT
void setupTFT()
{
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
}

// Configuração do sprite
void setupSprite()
{
    spr_pixel_buffer = (uint16_t *)spr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    spr.fillScreen(TFT_BLACK);
    spr.setRotation(0);
    spr.setSwapBytes(true);
    spr.setCursor(0, 0);
    spr.setTextColor(TFT_WHITE, TFT_BLACK); // Cor do texto: branco, fundo preto
    spr.setTextDatum(MC_DATUM);             // Centraliza o texto
    spr.setTextFont(2);                     // Define a fonte do texto
    spr.setTextSize(2);                     // Define o tamanho do texto
    spr.setTextWrap(true, false);           // Habilita quebra de linha automática horizontal
}

// Função para exibir texto no sprite
void showText(const String &text, bool clearBackground = false, bool pushToDisplay = true)
{
    if (clearBackground)
    {
        spr.fillScreen(TFT_BLACK);
    }

    spr.drawString(text, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 2);

    if (pushToDisplay)
    {
        spr.pushSprite(0, 0);
    }
}

// Configuração da câmera
void setupCamera()
{
    camera_config.ledc_channel = LEDC_CHANNEL_0;
    camera_config.ledc_timer = LEDC_TIMER_0;
    camera_config.pin_d0 = Y2_GPIO_NUM;
    camera_config.pin_d1 = Y3_GPIO_NUM;
    camera_config.pin_d2 = Y4_GPIO_NUM;
    camera_config.pin_d3 = Y5_GPIO_NUM;
    camera_config.pin_d4 = Y6_GPIO_NUM;
    camera_config.pin_d5 = Y7_GPIO_NUM;
    camera_config.pin_d6 = Y8_GPIO_NUM;
    camera_config.pin_d7 = Y9_GPIO_NUM;
    camera_config.pin_xclk = XCLK_GPIO_NUM;
    camera_config.xclk_freq_hz = XCLK_FREQ_HZ;
    camera_config.pin_pclk = PCLK_GPIO_NUM;
    camera_config.pin_vsync = VSYNC_GPIO_NUM;
    camera_config.pin_href = HREF_GPIO_NUM;
    camera_config.pin_sccb_sda = SIOD_GPIO_NUM;
    camera_config.pin_sccb_scl = SIOC_GPIO_NUM;
    camera_config.pin_pwdn = PWDN_GPIO_NUM;
    camera_config.pin_reset = RESET_GPIO_NUM;
    camera_config.pixel_format = PIXFORMAT_RGB565; // Formato de pixel RGB565 (compatível com TFT)
    camera_config.grab_mode = CAMERA_GRAB_LATEST;
    camera_config.frame_size = FRAMESIZE_240X240; // 240x240
    camera_config.jpeg_quality = JPEG_QUALITY;
    camera_config.fb_location = CAMERA_FB_IN_PSRAM;
    camera_config.fb_count = BUFFER_NUMBER;
}

bool startCamera()
{
    short int retry_count = 0;

    while (esp_camera_init(&camera_config) != ESP_OK)
    {
        retry_count++;

        showText("Falha ao iniciar câmera! Tentando novamente... (" + String(retry_count) + "/5)", true);

        if (retry_count > 5)
        {
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Talvez conflito?
    }

    return true;
}

__attribute__((noreturn)) void sleep()
{
    esp_camera_deinit();
    detachInterrupt(digitalPinToInterrupt(PIR_PIN));
    digitalWrite(DISPLAY_ENABLE_PIN, LOW);
    digitalWrite(POSITIVE_FB_PIN, LOW);
    esp_deep_sleep_start();
}

void TaskDisplayCode(void *pvParameters)
{
    tasks_alive = tasks_alive + 1;

    while (should_sleep_flag == false)
    {
        camera_fb_t *fb = esp_camera_fb_get();

        if (!fb)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        memcpy(spr_pixel_buffer, fb->buf, DISPLAY_HEIGHT * DISPLAY_WIDTH * 2);
        esp_camera_fb_return(fb);

        spr.pushSprite(0, 0);

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    portENTER_CRITICAL(&tasks_alive_mux);
    tasks_alive = tasks_alive - 1;
    portEXIT_CRITICAL(&tasks_alive_mux);

    vTaskDelete(nullptr);
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // Configuração dos pinos
    pinMode(DISPLAY_ENABLE_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(POSITIVE_FB_PIN, OUTPUT);
    digitalWrite(DISPLAY_ENABLE_PIN, HIGH);

    // Configuração da interrupção do PIR
    attachInterrupt(digitalPinToInterrupt(PIR_PIN), handlePIRInterrupt, RISING);

    // Configuração para o wakeup
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIR_PIN, 1);
    rtc_gpio_pullup_dis((gpio_num_t)PIR_PIN);
    rtc_gpio_pulldown_en((gpio_num_t)PIR_PIN);

    setupTFT();
    setupSprite();

    showText("Carregando...", true);

    setupCamera();

    if (!startCamera())
    {
        return;
    }

    xTaskCreatePinnedToCore(
        TaskDisplayCode,
        "TaskDisplay",
        12288,
        nullptr,
        1,
        &TaskDisplay,
        APP_CPU_NUM);
}

// FREERTOS cria loopTask automaticamente, no CORE 1
void loop()
{
    if (PIR_triggered)
    {
        last_PIR_tick = xTaskGetTickCount();
        PIR_triggered = false;
    }

    if ((xTaskGetTickCount() - last_PIR_tick) > ticks_to_sleep)
    {
        should_sleep_flag = true;
    }

    if (should_sleep_flag && tasks_alive == 0)
    {
        sleep();
    }

    vTaskDelay(1);
}
#include "esp_camera.h"
#include "Arduino.h"

#include "esp_wifi.h"
#include <EEPROM.h> // SALVAR DADOS PRÉ-DEEP SLEEP

// RISCO DE BROWNOUT
#include "soc/soc.h"

// RTC para DEEP SLEEP
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"

#include <TFT_eSPI.h>
#include <SPI.h>

#include "Camera_Setup.h"
#include "System_Setup.h"
#include "Secrets.h"

volatile bool should_sleep_flag = false;
volatile bool wifi_connected_flag = false;

volatile TickType_t last_PIR_tick = 0;
volatile bool PIR_triggered = false;
volatile int tasks_alive = 0;

portMUX_TYPE tasks_alive_mux = portMUX_INITIALIZER_UNLOCKED;

const TickType_t ticks_to_sleep = pdMS_TO_TICKS(SLEEP_TIMEOUT_MS);
const TickType_t ticks_to_send = pdMS_TO_TICKS(DATA_SEND_INTERVAL_MS);

TaskHandle_t TaskDisplay;
TaskHandle_t TaskNetwork;

camera_config_t camera_config;

// Biblioteca TFT_eSPI de Bodmer
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

uint16_t *sprite_buf;

using DisplayMessage = struct
{
    char text[DISPLAY_MSG_MAX_LEN];
    TickType_t duration; // 0 = persistente
};

QueueHandle_t messageQueue =
    xQueueCreate(5, sizeof(DisplayMessage));

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
    tft.setTextDatum(MC_DATUM);
    tft.fillScreen(TFT_BLACK);
}

// Configuração do sprite
void setupSprite()
{
    sprite_buf = (uint16_t *)spr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    spr.fillScreen(TFT_BLACK);
    spr.setRotation(0);
    spr.setSwapBytes(true);
    spr.setTextColor(TFT_WHITE, TFT_BLACK); // Cor do texto: branco, fundo preto
    spr.setTextDatum(MC_DATUM);             // Centraliza o texto
    spr.setTextFont(2);                     // Define a fonte do texto
    spr.setTextSize(1);                     // Define o tamanho do texto
}

// Função para exibir texto no sprite. Sempre com fundo preto, no centro da tela
void showText(const char *text, bool pushToDisplay = true)
{
    spr.fillScreen(TFT_BLACK);

    spr.drawString(text, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);

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

// Inicia Câmera
bool startCamera()
{
    unsigned short int retry_count = 0;
    char msg[DISPLAY_MSG_MAX_LEN];

    while (esp_camera_init(&camera_config) != ESP_OK)
    {
        retry_count++;

        snprintf(
            msg,
            DISPLAY_MSG_MAX_LEN,
            "Falha ao iniciar camera! Tentando novamente... (%d/5)",
            retry_count);

        showText(msg, true);

        if (retry_count > 5)
        {
            showText("Falha ao iniciar camera! Reinicie o dispositivo.", true);
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    return true;
}

// Envia uma mensagem para lista da TaskDisplay
bool sendDisplayMessage(const char *text, unsigned long duration_ms)
{
    if (should_sleep_flag || !messageQueue)
    {
        return false;
    }

    DisplayMessage msg{};
    strncpy(msg.text, text, DISPLAY_MSG_MAX_LEN - 1);
    msg.duration = pdMS_TO_TICKS(duration_ms); // 0 = persistente

    return xQueueSendToBack(messageQueue, &msg, 0) == pdPASS;
}

// Entra em Deepsleep
__attribute__((noreturn)) void sleep()
{
    detachInterrupt(digitalPinToInterrupt(PIR_PIN));
    digitalWrite(DISPLAY_ENABLE_PIN, LOW);
    digitalWrite(POSITIVE_FB_PIN, LOW);
    esp_camera_deinit();
    esp_deep_sleep_start();
}

// Muda a variável compartilhada de tasks vivas
void changeAliveTaskCount(int delta)
{
    portENTER_CRITICAL(&tasks_alive_mux);
    tasks_alive = tasks_alive + delta;
    portEXIT_CRITICAL(&tasks_alive_mux);
}

// Envia os pixels do buffer da câmera para o sprite do display
bool showCamFrame(bool push_sprite = false)
{
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
        return false;
    }

    memcpy(sprite_buf, fb->buf, DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);

    esp_camera_fb_return(fb);

    if (push_sprite)
    {
        spr.pushSprite(0, 0);
    }

    return true;
}

// Converte o último frame da câmera e envia para o servidor
bool sendFrame()
{
    // TODO: ENVIAR FRAME PARA O SERVIDOR
    sendDisplayMessage("Frame enviado", 1000);
    return true;
}

void TaskNetworkCode(void *pvParameters)
{
    changeAliveTaskCount(1);

    const TickType_t tickDelay = pdMS_TO_TICKS(100);
    const unsigned short send_its = ticks_to_send / tickDelay;
    unsigned short it_cnt = 0;

    while (!should_sleep_flag)
    {

        if (++it_cnt >= send_its)
        {
            it_cnt = 0;
            sendFrame();
        }

        vTaskDelay(tickDelay);
    }

    changeAliveTaskCount(-1);
    vTaskDelete(nullptr);
}

void TaskDisplayCode(void *pvParameters)
{
    changeAliveTaskCount(1);

    DisplayMessage msg{};
    bool overlayActive = false;
    TickType_t overlayUntil = 0;

    const TickType_t tickDelay = pdMS_TO_TICKS(50);

    while (!should_sleep_flag)
    {
        if (!overlayActive && xQueueReceive(messageQueue, &msg, 0) == pdPASS)
        {
            overlayActive = true;
            showText(msg.text, true);

            if (msg.duration > 0)
            {
                overlayUntil = xTaskGetTickCount() + msg.duration;
            }
            else
            {
                overlayUntil = 0;
            }
        }

        if (overlayActive && msg.duration > 0 && xTaskGetTickCount() >= overlayUntil)
        {
            overlayActive = false;
        }

        if (!overlayActive)
        {
            showCamFrame(true);
        }

        vTaskDelay(tickDelay);
    }

    changeAliveTaskCount(-1);
    vTaskDelete(nullptr);
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    last_PIR_tick = xTaskGetTickCount();

    // Configuração dos pinos
    pinMode(DISPLAY_ENABLE_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(POSITIVE_FB_PIN, OUTPUT);
    digitalWrite(DISPLAY_ENABLE_PIN, HIGH);

    // Configuração da interrupção do PIR
    attachInterrupt(digitalPinToInterrupt(PIR_PIN), handlePIRInterrupt, CHANGE);

    // Configuração para o wakeup
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIR_PIN, 1);
    rtc_gpio_pullup_dis((gpio_num_t)PIR_PIN);
    rtc_gpio_pulldown_en((gpio_num_t)PIR_PIN);

    setupTFT();
    setupSprite();

    showText("Carregando...");

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
        2,
        &TaskDisplay,
        APP_CPU_NUM);

    xTaskCreatePinnedToCore(
        TaskNetworkCode,
        "TaskNetwork",
        4096,
        nullptr,
        1,
        &TaskNetwork,
        PRO_CPU_NUM);
}

// FREERTOS cria loopTask automaticamente, no CORE 1
void loop()
{
    if (PIR_triggered)
    {
        PIR_triggered = false;
        last_PIR_tick = xTaskGetTickCount();
    }

    if ((xTaskGetTickCount() - last_PIR_tick) > ticks_to_sleep)
    {
        should_sleep_flag = true;
    }

    // todas as tasks devem ser programadas para eventualmente "morrer" sob a condição "should sleep"
    if (should_sleep_flag && tasks_alive == 0)
    {
        sleep();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}
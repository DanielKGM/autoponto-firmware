#include "Camera.h"
#include "Config.h"
#include "Display.h"

static camera_config_t camera_config;

void initCamera()
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
#include "Camera.h"
#include "Config.h"
#include "Display.h"

namespace camera
{
    camera_config_t config;

    void configCamera()
    {
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
        config.xclk_freq_hz = XCLK_FREQ_HZ;
        config.pin_pclk = PCLK_GPIO_NUM;
        config.pin_vsync = VSYNC_GPIO_NUM;
        config.pin_href = HREF_GPIO_NUM;
        config.pin_sccb_sda = SIOD_GPIO_NUM;
        config.pin_sccb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn = PWDN_GPIO_NUM;
        config.pin_reset = RESET_GPIO_NUM;
        config.pixel_format = PIXFORMAT_JPEG;
        config.grab_mode = CAMERA_GRAB_LATEST;
        config.frame_size = FRAMESIZE_240X240;
        config.jpeg_quality = JPEG_QUALITY;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.fb_count = BUFFER_NUMBER;
    }

    bool startCamera()
    {
        unsigned short int retry_count = 0;
        char msg[DISPLAY_MSG_MAX_LEN];

        while (true)
        {
            esp_err_t err = esp_camera_init(&config);

            if (err == ESP_OK)
                return true;

            esp_camera_deinit();
            retry_count++;

            snprintf(msg, DISPLAY_MSG_MAX_LEN,
                     "Falha ao iniciar camera (%d/5)", retry_count);
            display::sendDisplayMessage(msg);

            if (retry_count >= 5)
            {
                display::sendDisplayMessage(
                    "Falha ao iniciar camera! Reinicie o dispositivo.",
                    0,
                    &display::ICON_SAD);
                return false;
            }

            vTaskDelay(pdMS_TO_TICKS(2000));
        }

        return true;
    }
}
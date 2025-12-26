#include "Camera.h"
#include "Config.h"
#include "Display.h"

namespace camera
{
    constexpr int PWDN_GPIO_NUM = 32;
    constexpr int RESET_GPIO_NUM = -1;
    constexpr int XCLK_GPIO_NUM = 0;
    constexpr int XCLK_FREQ_HZ = 20000000;
    constexpr int SIOD_GPIO_NUM = 26;
    constexpr int SIOC_GPIO_NUM = 27;
    constexpr int Y9_GPIO_NUM = 35;
    constexpr int Y8_GPIO_NUM = 34;
    constexpr int Y7_GPIO_NUM = 39;
    constexpr int Y6_GPIO_NUM = 36;
    constexpr int Y5_GPIO_NUM = 21;
    constexpr int Y4_GPIO_NUM = 19;
    constexpr int Y3_GPIO_NUM = 18;
    constexpr int Y2_GPIO_NUM = 5;
    constexpr int VSYNC_GPIO_NUM = 25;
    constexpr int HREF_GPIO_NUM = 23;
    constexpr int PCLK_GPIO_NUM = 22;
    constexpr int BUFFER_NUMBER = 2;
    constexpr int JPEG_QUALITY = 10;

    camera_config_t config;

    void initCamera()
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
        using namespace display;

        unsigned short int retry_count = 0;
        char msg[DISPLAY_MSG_MAX_LEN];

        while (esp_camera_init(&config) != ESP_OK)
        {
            retry_count++;

            snprintf(
                msg,
                DISPLAY_MSG_MAX_LEN,
                "Falha ao iniciar camera! Tentando novamente... (%d/5)",
                retry_count);

            sendDisplayMessage(msg);

            if (retry_count > 5)
            {
                sendDisplayMessage("Falha ao iniciar camera! Reinicie o dispositivo.", 0, &ICON_SAD);
                return false;
            }

            vTaskDelay(pdMS_TO_TICKS(2000));
        }

        return true;
    }
}
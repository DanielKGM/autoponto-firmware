#include "Camera.h"
#include "Display.h"
#include "Config_Camera.h"
#include "Globals.h"
#include "Power.h"

namespace camera
{

    namespace
    {
        camera_config_t config;

        QueueHandle_t previewQueue = xQueueCreate(1, sizeof(FrameBuffer));
        QueueHandle_t snapshotQueue = xQueueCreate(1, sizeof(FrameBuffer));
        volatile bool snapshotRequested = false;
        portMUX_TYPE snapshotMux = portMUX_INITIALIZER_UNLOCKED;

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

        bool cloneFrame(camera_fb_t const *fb, FrameBuffer &out)
        {
            out.data = nullptr;
            out.len = 0;

            uint8_t *copy = (uint8_t *)ps_malloc(fb->len);
            if (!copy)
            {
                return false;
            }

            memcpy(copy, fb->buf, fb->len);
            out.data = copy;
            out.len = fb->len;
            return true;
        }

        void replaceQueueFrame(QueueHandle_t queue, FrameBuffer const &frame)
        {
            FrameBuffer old;
            if (xQueueReceive(queue, &old, 0) == pdTRUE)
            {
                releaseFrame(old);
            }

            xQueueOverwrite(queue, &frame);
        }

        bool consumeSnapshotRequest()
        {
            bool requested = false;

            portENTER_CRITICAL(&snapshotMux);
            requested = snapshotRequested;
            snapshotRequested = false;
            portEXIT_CRITICAL(&snapshotMux);

            return requested;
        }
    }

    bool startCamera()
    {
        configCamera();

        unsigned short int retry_count = 0;
        char msg[64];

        while (true)
        {
            esp_err_t err = esp_camera_init(&config);

            if (err == ESP_OK)
                return true;

            esp_camera_deinit();
            retry_count++;

            snprintf(msg, 64,
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
    }

    void releaseFrame(FrameBuffer &frame)
    {
        if (frame.data)
        {
            free(frame.data);
            frame.data = nullptr;
        }

        frame.len = 0;
    }

    bool requestSnapshot()
    {
        portENTER_CRITICAL(&snapshotMux);
        bool canRequest = !snapshotRequested;
        if (canRequest)
        {
            snapshotRequested = true;
        }
        portEXIT_CRITICAL(&snapshotMux);

        return canRequest;
    }

    bool takePreviewFrame(FrameBuffer &out, TickType_t wait)
    {
        return xQueueReceive(previewQueue, &out, wait) == pdTRUE;
    }

    bool takeSnapshotFrame(FrameBuffer &out, TickType_t wait)
    {
        return xQueueReceive(snapshotQueue, &out, wait) == pdTRUE;
    }

    void TaskCameraCode(void *pvParameters)
    {
        changeTaskCount(1);

        const TickType_t activeDelay = pdMS_TO_TICKS(30);
        const TickType_t idleDelay = pdMS_TO_TICKS(120);

        while (true)
        {
            TickType_t delay = power::checkIdle() ? idleDelay : activeDelay;

            if (checkSleepEvent(delay))
            {
                break;
            }

            if (power::checkIdle())
            {
                continue;
            }

            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb)
            {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            FrameBuffer preview;
            if (cloneFrame(fb, preview))
            {
                replaceQueueFrame(previewQueue, preview);
            }

            if (consumeSnapshotRequest())
            {
                FrameBuffer snap;
                if (cloneFrame(fb, snap))
                {
                    replaceQueueFrame(snapshotQueue, snap);
                }
            }

            esp_camera_fb_return(fb);
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }

}
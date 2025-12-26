#pragma once
#include "esp_camera.h"

namespace display
{
    struct FrameBuffer
    {
        uint8_t *data;
        size_t len;
    };

    struct Icon
    {
        const uint16_t *data;
        uint16_t width;
        uint16_t height;
    };

    extern QueueHandle_t frameQueue;
    extern const Icon ICON_WIFI;
    extern const Icon ICON_SAD;
    extern const Icon ICON_HAPPY;
    extern const Icon ICON_SERVER;

    bool sendDisplayMessage(const char *text, unsigned long durationMs = 0, const Icon *icon = nullptr);
    void TaskDisplayCode(void *pvParameters);
}

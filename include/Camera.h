#pragma once

#include "esp_camera.h"
#define PWDN_GPIO_NUM 32
#include "Arduino.h"

namespace camera
{
    struct FrameBuffer
    {
        uint8_t *data = nullptr;
        size_t len = 0;
    };

    bool startCamera();

    void TaskCameraCode(void *pvParameters);
    void releaseFrame(FrameBuffer &frame);

    // for http request
    bool requestSnapshot();
    bool takeSnapshotFrame(FrameBuffer &out, TickType_t wait = 0);
    // for display feedback
    bool takePreviewFrame(FrameBuffer &out, TickType_t wait = 0);

}

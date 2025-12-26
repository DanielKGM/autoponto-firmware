#pragma once

#include "esp_camera.h"
#include "freertos/task.h"

namespace camera
{
    extern camera_config_t config;
    void initCamera();
    bool startCamera();
}

#pragma once
#include "driver/rtc_io.h"
#include "esp_camera.h"
#include "esp32-hal-cpu.h"
#include <WiFi.h>

namespace power
{
    extern volatile bool sensorTriggered;
    extern volatile bool buzzerTriggered;

    void positiveFB();
    void configSleep();
    void configPins();
    void sleep();
    void enterIdle();
    void exitIdle();
}
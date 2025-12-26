#pragma once
#include "driver/rtc_io.h"
#include "esp32-hal-cpu.h"
#include <WiFi.h>

namespace power
{
    extern volatile bool sensorTriggered;
    extern volatile bool buzzerTriggered;

    void changeTriggeredFlag(bool value);
    void positiveFB();
    void configSleep();
    void configPins();
    void sleep();
    void enterIdle();
    void exitIdle();
}
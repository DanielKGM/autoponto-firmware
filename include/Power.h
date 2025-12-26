#pragma once

#include "driver/rtc_io.h"

namespace power
{
    extern volatile TickType_t lastSensorTick;
    extern volatile bool sensorTriggered;
    extern volatile bool buzzerTriggered;

    void positiveFB();
    void setBuzzerTriggered(bool value);
    void initSleep();
    void initPins();
    void sleep();
}
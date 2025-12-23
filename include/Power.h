#pragma once
#include "driver/rtc_io.h"
#include "esp_camera.h"

extern volatile TickType_t lastSensorTick;
extern volatile bool sensorTriggered;

void initSleep();
void initPins();
__attribute__((noreturn)) void sleep();
void IRAM_ATTR handlePIRInterrupt();
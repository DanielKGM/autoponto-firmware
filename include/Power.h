#pragma once
#include "driver/rtc_io.h"
#include "esp_camera.h"
#include "Arduino.h"

extern volatile TickType_t lastSensorTick;
extern volatile bool sensorTriggered;
extern volatile bool buzzerTriggered;
extern portMUX_TYPE buzzerMux;

void initSleep();
void initPins();
void setBuzzerTriggered(bool value);
__attribute__((noreturn)) void sleep();
void IRAM_ATTR handlePIRInterrupt();
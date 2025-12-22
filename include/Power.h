#pragma once
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "esp_camera.h"

extern volatile TickType_t last_PIR_tick;
extern volatile bool PIR_triggered;

void initSleep();
void initPins();
__attribute__((noreturn)) void sleep();
void IRAM_ATTR handlePIRInterrupt();
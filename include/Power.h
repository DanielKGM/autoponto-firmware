#pragma once
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "esp_camera.h"

void initSleep();
void initPins();
__attribute__((noreturn)) void sleep();
void IRAM_ATTR handlePIRInterrupt();
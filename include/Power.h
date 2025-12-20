#pragma once
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "esp_camera.h"

void setupSleep();
void setupPins();
__attribute__((noreturn)) void sleep();
void IRAM_ATTR handlePIRInterrupt();
#pragma once
#include "Arduino.h"
#include <WiFi.h>

extern bool should_send_flag;

bool sendFrame();
bool initWifi();
void TaskNetworkCode(void *pvParameters);

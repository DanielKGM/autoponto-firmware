#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>

extern bool should_send_flag;

bool sendFrame();
bool initWifi();
void TaskNetworkCode(void *pvParameters);

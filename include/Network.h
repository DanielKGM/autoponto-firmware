#pragma once
#include "Arduino.h"
#include <WiFi.h>

extern WiFiClient wifiClient;

bool sendFrame();
bool initWifi();
bool connWifi();
void TaskNetworkCode(void *pvParameters);

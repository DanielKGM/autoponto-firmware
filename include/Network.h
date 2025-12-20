#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

extern WiFiClientSecure client;

bool sendFrame();
void initWifi();
void TaskNetworkCode(void *pvParameters);

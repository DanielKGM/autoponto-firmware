#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

extern WiFiClientSecure client;
extern bool should_send_flag;

bool sendFrame();
bool initWifi();
void TaskNetworkCode(void *pvParameters);

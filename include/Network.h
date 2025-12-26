#pragma once
#include "Arduino.h"
#include <WiFi.h>

extern WiFiClient wifiClient;

namespace network
{
    void TaskNetworkCode(void *pvParameters);
}

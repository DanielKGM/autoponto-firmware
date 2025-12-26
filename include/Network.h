#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>

namespace network
{
    extern WiFiClient wifiClient;
    void TaskNetworkCode(void *pvParameters);
}

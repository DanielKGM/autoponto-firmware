#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>

namespace network
{
    int8_t getRSSI();
    bool isConnected();
    extern WiFiClient wifiClient;
    void TaskNetworkCode(void *pvParameters);
}

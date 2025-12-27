#pragma once
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>

namespace network
{
    bool isConnected();
    extern WiFiClient wifiClient;
    void TaskNetworkCode(void *pvParameters);
}

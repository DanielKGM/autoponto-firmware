#pragma once
#include <ArduinoJson.h>
#include <PubSubClient.h>

namespace mqtt
{
    bool publishStatus(const char *payload);
    void TaskMqttCode(void *pvParameters);
}
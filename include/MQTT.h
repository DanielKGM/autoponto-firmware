#pragma once
#include <ArduinoJson.h>
#include <PubSubClient.h>

namespace mqtt
{
    void publishStatus(const char *payload);
    void TaskMqttCode(void *pvParameters);
}
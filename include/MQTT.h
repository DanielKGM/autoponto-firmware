#pragma once

namespace mqtt
{
    bool publishStatus(const char *payload);
    void TaskMqttCode(void *pvParameters);
}
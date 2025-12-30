#pragma once
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define MQTT_TOPIC_SIZE 17 // deviceId + '/' + ('log' | 'cmd') + '/0'
#define MQTT_PAYLOAD_MAX 256

namespace mqtt
{
    extern char topicCmd[MQTT_TOPIC_SIZE];
    extern char topicLogs[MQTT_TOPIC_SIZE];
    extern char topicStatus[MQTT_TOPIC_SIZE];

    bool isConnected();
    void publish(char *topic, const char *payload, bool retain = false);
    void TaskMqttCode(void *pvParameters);
}
#pragma once
#include <PubSubClient.h>
#define MQTT_TOPIC_SIZE 17 // deviceId + '/' + ('log' | 'cmd') + '/0'
#define MQTT_PAYLOAD_MAX 256

extern PubSubClient mqtt;
extern volatile bool loggerFlag;
extern char topicCmd[];
extern char topicLogs[];
extern char topicStatus[];
extern QueueHandle_t mqttQueue;

void loadLogFlag();
void setLogFlag(bool value);

void buildTopics();
bool connMqtt();
bool initMqtt();
void publishLog(const char *tag, const char *msg);
void mqttCallback(char *topic, byte *payload, unsigned int length);
void TaskMqttCode(void *pvParameters);
void publishSystemStats();
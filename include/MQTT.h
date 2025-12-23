#pragma once
#include <ArduinoJson.h>
#include <PubSubClient.h>

extern PubSubClient mqtt;
extern volatile bool logFlag;
extern char topicStatus[];
extern char topicCmd[];
extern char topicLogs[];

void buildTopics();
bool connMqtt();
bool initMqtt();

void loadLogFlag();
void setLogFlag(bool value);

void mqttCallback(char *topic, byte *payload, unsigned int length);
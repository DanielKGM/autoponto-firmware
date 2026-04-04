#pragma once
#include <ArduinoJson.h>

#define SMALL_STR 32
#define MEDIUM_STR 64
#define LARGE_STR 96

struct RuntimeConfig
{
    char wifiSsid[SMALL_STR];
    char wifiPass[MEDIUM_STR];

    char restUrl[LARGE_STR];
    char restPass[LARGE_STR];
    char restPostPath[SMALL_STR];
    char restFetchPath[SMALL_STR];

    char mqttUrl[LARGE_STR];
    uint16_t mqttPort;
    char mqttUser[SMALL_STR];
    char mqttPass[MEDIUM_STR];
};

extern RuntimeConfig runtimeConfig;

namespace configs
{
    bool loadConfigs();
    bool saveConfigs();

    void loadDefaults();
    bool ensureLoaded();

    bool importJson(const JsonVariantConst &json);
    bool exportJson(JsonDocument &doc);
}
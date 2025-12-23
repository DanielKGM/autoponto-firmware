#pragma once
#include "Arduino.h"
#include <Preferences.h>

// BOOTING -> NET_OFF -> NET_ON -> MQTT_OFF -> READY -> SLEEPING

enum class SystemState
{
    BOOTING,
    NET_ON,
    NET_OFF,
    MQTT_OFF,
    READY,
    USER_PRESENT,
    WAITING_SERVER,
    SLEEPING
};

extern char deviceId[13];

extern volatile SystemState systemState;
extern portMUX_TYPE systemStateMux;

extern volatile int tasksAlive;
extern portMUX_TYPE tasksAliveMux;

void changeTaskCount(int delta);
void setSystemState(SystemState newState);

extern TaskHandle_t TaskDisplay;
extern TaskHandle_t TaskNetwork;
extern TaskHandle_t TaskMqtt;
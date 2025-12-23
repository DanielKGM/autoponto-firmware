#pragma once
#include "Arduino.h"
#include "Config.h"
#include <Preferences.h>

struct DisplayMessage
{
    char text[DISPLAY_MSG_MAX_LEN];
    TickType_t duration;
};

// BOOTING -> NET_OFF -> NET_ON -> MQTT_OFF -> READY -> SLEEPING

enum class SystemState
{
    BOOTING,
    NET_ON,
    NET_OFF,
    MQTT_OFF,
    READY,
    SLEEPING
};

extern char deviceId[13];

extern volatile SystemState systemState;
extern portMUX_TYPE systemStateMux;

extern volatile int tasksAlive;
extern portMUX_TYPE tasksAliveMux;

void changeTaskCount(int delta);
void setSystemState(SystemState newState);
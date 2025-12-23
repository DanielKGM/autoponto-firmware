#pragma once
#include "Arduino.h"
#include "Config.h"
#include <Preferences.h>

struct DisplayMessage
{
    char text[DISPLAY_MSG_MAX_LEN];
    TickType_t duration;
};

enum class SystemState
{
    BOOTING,
    READY,
    DISCONNECTED,
    SLEEPING
};

extern char deviceId[13];

extern volatile SystemState currentState;
extern portMUX_TYPE systemStateMux;

extern volatile int tasksAlive;
extern portMUX_TYPE tasksAliveMux;

void changeTaskCount(int delta);
void setSystemState(SystemState newState);
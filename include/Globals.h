#pragma once
#include "Arduino.h"
#include "Config.h"

enum class SystemState
{
    BOOTING,
    READY,
    DISCONNECTED,
    SLEEPING
};

extern volatile SystemState current_state;
extern portMUX_TYPE system_state_mux;

extern volatile int tasks_alive;
extern portMUX_TYPE tasks_alive_mux;

void changeTaskCount(int delta);
void setSystemState(SystemState new_state);
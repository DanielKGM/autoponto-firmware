#pragma once
#include "Arduino.h"
#include "Config.h"

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

extern volatile SystemState current_state;
extern portMUX_TYPE system_state_mux;

extern QueueHandle_t messageQueue;

extern volatile int tasks_alive;
extern portMUX_TYPE tasks_alive_mux;

extern volatile bool PIR_triggered;

void changeTaskCount(int delta);
void setSystemState(SystemState new_state);
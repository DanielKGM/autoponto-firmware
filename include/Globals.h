#pragma once
#include "Arduino.h"

#define DISPLAY_MSG_MAX_LEN 50
struct DisplayMessage
{
    char text[DISPLAY_MSG_MAX_LEN];
    TickType_t duration;
};

extern volatile bool should_sleep_flag;
extern volatile bool PIR_triggered;
extern volatile int tasks_alive;
extern portMUX_TYPE tasks_alive_mux;
extern QueueHandle_t messageQueue;

void changeAliveTaskCount(int delta);
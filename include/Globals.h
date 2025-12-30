#pragma once
#include "Arduino.h"
#include <Preferences.h>
#define EVT_SLEEP (1 << 0)

#define CHAIR_STR_LENGTH 256

enum class SystemState
{
    BOOTING,
    NET_OFF,
    NET_ON,
    MQTT_OFF,
    IDLE,
    WORKING,
    WAITING_SERVER,
    FETCHING,
    SLEEPING
};

struct AttendanceContext
{
    char chair[CHAIR_STR_LENGTH];
    unsigned long msForNext;
    unsigned long msRemaining;
};

extern AttendanceContext context;
extern EventGroupHandle_t systemEvents;
extern char deviceId[13];
extern TaskHandle_t TaskDisplay;
extern TaskHandle_t TaskNetwork;
extern TaskHandle_t TaskMqtt;

void changeTaskCount(short int delta);
short int checkTaskCount();
void setState(SystemState newState);
bool checkState(SystemState state);
void triggerSleepEvent();
bool checkSleepEvent(TickType_t waitInterval);
const char *stateStr(SystemState state);
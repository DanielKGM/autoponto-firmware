#pragma once
#include "Arduino.h"
#include <Preferences.h>
#define EVT_SLEEP (1 << 0)

// BOOTING -> NET_OFF -> NET_ON -> MQTT_OFF -> IDLE -> WORKING -> WAITING_SERVER || IDLE
// WAITING_SERVER -> WORKING
// IDLE -> SLEEPING

enum class SystemState
{
    BOOTING,
    NET_OFF,
    NET_ON,
    MQTT_OFF,
    IDLE,
    WORKING,
    WAITING_SERVER,
    SLEEPING
};

extern EventGroupHandle_t systemEvents;
extern char deviceId[13];
extern TaskHandle_t TaskDisplay;
extern TaskHandle_t TaskNetwork;
extern TaskHandle_t TaskMqtt;

void changeTaskCount(short int delta);
short int checkTaskCount();
void setSystemState(SystemState newState);
bool checkSystemState(SystemState state);
void triggerSleepEvent();
bool checkSleepEvent(TickType_t waitInterval);
const char *stateStr(SystemState state);
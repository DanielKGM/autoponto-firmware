#pragma once
#include "Arduino.h"
#include <Preferences.h>
#define EVT_SLEEP (1 << 0)

// BOOTING -> NET_OFF -> NET_ON -> MQTT_OFF -> WORKING -> WAITING_SERVER || IDLE || NET_OFF || MQTT_OFF
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
void setState(SystemState newState);
bool checkState(SystemState state);
void triggerSleepEvent();
bool checkSleepEvent(TickType_t waitInterval);
const char *stateStr(SystemState state);
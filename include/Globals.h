#pragma once
#include "Arduino.h"
#include <Preferences.h>
#define EVT_SLEEP (1 << 0)

#define LESSON_NAME_LENGTH 256

enum class SystemState
{
    BOOTING,
    CONFIGURING,
    NET_OFF,
    MQTT_OFF,
    IDLE,
    WORKING,
    WAITING_SERVER,
    FETCHING,
    SLEEPING
};

struct AttendanceContext
{
    char lesson_name[LESSON_NAME_LENGTH];
    TickType_t ticksForNext;
    TickType_t ticksRemaining;
    TickType_t fetchTick;
};

extern AttendanceContext context;
extern EventGroupHandle_t systemEvents;
extern char deviceId[13];
extern TaskHandle_t TaskDisplay;
extern TaskHandle_t TaskNetwork;
extern TaskHandle_t TaskMqtt;

void clearContext();
void changeTaskCount(short int delta);
short int checkTaskCount();
void setState(SystemState newState);
bool checkState(SystemState state);
bool checkSleepEvent(TickType_t waitInterval);
const char *stateStr(SystemState state);
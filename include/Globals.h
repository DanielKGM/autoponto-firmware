#pragma once
#include "Arduino.h"
#include <Preferences.h>
#define EVT_SLEEP (1 << 0)

#define FB_MSG_LENGTH 256

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

enum ContextFields
{
    MSG = 1 << 0,
    MS_NEXT = 1 << 1,
    MS_REM = 1 << 2,
    SYNCED = 1 << 3,
    UPDATE = 1 << 4,
    ALL = 0xFF
};

struct AttendanceContext
{
    char msg[FB_MSG_LENGTH];
    unsigned long msForNext;
    unsigned long msRemaining;
    bool synced;
    bool shouldUpdate;
};

extern AttendanceContext context;
extern EventGroupHandle_t systemEvents;
extern char deviceId[13];
extern TaskHandle_t TaskDisplay;
extern TaskHandle_t TaskNetwork;
extern TaskHandle_t TaskMqtt;

void saveContext(uint8_t fields = ALL);
void loadContext(uint8_t fiels = ALL);
void changeTaskCount(short int delta);
short int checkTaskCount();
void setState(SystemState newState);
bool checkState(SystemState state);
void triggerSleepEvent();
bool checkSleepEvent(TickType_t waitInterval);
const char *stateStr(SystemState state);
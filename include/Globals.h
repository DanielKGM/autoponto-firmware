#pragma once
#include "Arduino.h"
#define EVT_SLEEP (1 << 0)

#define LESSON_NAME_LENGTH 256
#define TASK_METRIC_COUNT 5

enum class SystemState
{
    BOOTING,        // OFFLINE
    NET_OFF,        // OFFLINE
    MQTT_OFF,       // OFFLINE
    WAITING_SERVER, // not logged
    WORKING,
    FETCHING,
    SLEEPING
};

enum class TaskMetric : uint8_t
{
    LOOP_TASK,
    MQTT_TASK,
    NETWORK_TASK,
    CAMERA_TASK,
    DISPLAY_TASK
};

struct AttendanceContext
{
    char lesson_name[LESSON_NAME_LENGTH];
    uint64_t msForNext;
    uint64_t msRemaining;
    TickType_t fetchTick;
};

extern AttendanceContext context;
extern EventGroupHandle_t systemEvents;
extern char deviceId[13];
extern TaskHandle_t TaskDisplay;
extern TaskHandle_t TaskNetwork;
extern TaskHandle_t TaskMqtt;
extern TaskHandle_t TaskCamera;
extern volatile SystemState systemState;

void clearContext();
void changeTaskCount(short int delta);
short int checkTaskCount();
void setState(SystemState newState);
bool checkState(SystemState state);
bool checkSleepEvent(TickType_t waitInterval);
uint64_t getRemainingMs(TickType_t now, uint64_t totalMs, TickType_t startTick);
void recordTaskRuntime(TaskMetric task, uint32_t durationUs);
void snapshotAndResetTaskAverages(uint32_t averagesUs[TASK_METRIC_COUNT], uint32_t counts[TASK_METRIC_COUNT]);
void recordRestPostDuration(uint32_t durationMs);
uint32_t snapshotAndResetRestPostMax();

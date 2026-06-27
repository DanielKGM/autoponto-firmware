#include "Globals.h"
#include "MQTT.h"
#include "Power.h"

volatile SystemState systemState;
portMUX_TYPE tasksAliveMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE systemStateMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE metricsMux = portMUX_INITIALIZER_UNLOCKED;
volatile short int tasksAlive = 0;

struct TaskRuntimeStats
{
    uint64_t totalUs;
    uint32_t samples;
};

TaskRuntimeStats taskRuntimeStats[TASK_METRIC_COUNT] = {};
uint32_t restPostMaxMs = 0;

// extern
char deviceId[13];
EventGroupHandle_t systemEvents = xEventGroupCreate();
TaskHandle_t TaskDisplay;
TaskHandle_t TaskNetwork;
TaskHandle_t TaskMqtt;
TaskHandle_t TaskCamera;
AttendanceContext context = AttendanceContext{};

void clearContext()
{
    context.fetchTick = 0;
    context.lesson_name[0] = '\0';
    context.msForNext = 0;
    context.msRemaining = 0;
    setState(SystemState::FETCHING);
}

void changeTaskCount(short int delta)
{
    portENTER_CRITICAL(&tasksAliveMux);
    tasksAlive = tasksAlive + delta;
    portEXIT_CRITICAL(&tasksAliveMux);
}

short int checkTaskCount()
{
    return tasksAlive;
}

void setState(SystemState newState)
{
    portENTER_CRITICAL(&systemStateMux);
    systemState = newState;
    portEXIT_CRITICAL(&systemStateMux);

    switch (newState)
    {
    case SystemState::WORKING:
        mqtt::publishStatus("working");
        break;

    case SystemState::FETCHING:
        mqtt::publishStatus("fetching");
        break;

    case SystemState::SLEEPING:
        mqtt::publishStatus("sleep");
        break;

    default:
        return;
    }
}

bool checkState(SystemState state)
{
    return (systemState == state);
}

bool checkSleepEvent(TickType_t waitInterval)
{
    return xEventGroupWaitBits(
               systemEvents,
               EVT_SLEEP,
               pdFALSE,
               pdFALSE,
               waitInterval) &
           EVT_SLEEP;
}

uint64_t getRemainingMs(TickType_t now, uint64_t totalMs, TickType_t startTick)
{
    uint64_t elapsedMs = (static_cast<uint64_t>(now - startTick) * 1000ULL) / configTICK_RATE_HZ;
    return elapsedMs >= totalMs ? 0 : totalMs - elapsedMs;
}

void recordTaskRuntime(TaskMetric task, uint32_t durationUs)
{
    uint8_t index = static_cast<uint8_t>(task);
    if (index >= TASK_METRIC_COUNT)
    {
        return;
    }

    portENTER_CRITICAL(&metricsMux);
    taskRuntimeStats[index].totalUs += durationUs;
    taskRuntimeStats[index].samples++;
    portEXIT_CRITICAL(&metricsMux);
}

void snapshotAndResetTaskAverages(uint32_t averagesUs[TASK_METRIC_COUNT], uint32_t counts[TASK_METRIC_COUNT])
{
    portENTER_CRITICAL(&metricsMux);
    for (uint8_t i = 0; i < TASK_METRIC_COUNT; i++)
    {
        counts[i] = taskRuntimeStats[i].samples;
        averagesUs[i] = taskRuntimeStats[i].samples > 0
                            ? taskRuntimeStats[i].totalUs / taskRuntimeStats[i].samples
                            : 0;
        taskRuntimeStats[i] = {};
    }
    portEXIT_CRITICAL(&metricsMux);
}

void recordRestPostDuration(uint32_t durationMs)
{
    portENTER_CRITICAL(&metricsMux);
    if (durationMs > restPostMaxMs)
    {
        restPostMaxMs = durationMs;
    }
    portEXIT_CRITICAL(&metricsMux);
}

uint32_t snapshotAndResetRestPostMax()
{
    portENTER_CRITICAL(&metricsMux);
    uint32_t value = restPostMaxMs;
    restPostMaxMs = 0;
    portEXIT_CRITICAL(&metricsMux);
    return value;
}

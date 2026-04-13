#include "Globals.h"
#include "MQTT.h"

volatile SystemState systemState;
portMUX_TYPE tasksAliveMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE systemStateMux = portMUX_INITIALIZER_UNLOCKED;
volatile short int tasksAlive = 0;

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
    context.ticksForNext = 0;
    context.ticksRemaining = 0;
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

    // only publish meaningful states
    if (newState < SystemState::WORKING)
    {
        return;
    }

    mqtt::publish(mqtt::topicStatus, stateStr(newState), true);
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

const char *stateStr(SystemState state)
{
    switch (state)
    {
    case SystemState::WORKING:
        return "WORKING";
    case SystemState::SLEEPING:
        return "SLEEPING";
    case SystemState::WAITING_SERVER:
        return "WAITING SERVER";
    case SystemState::FETCHING:
        return "FETCHING";
    default:
        return "UNKNOWN";
    }
}
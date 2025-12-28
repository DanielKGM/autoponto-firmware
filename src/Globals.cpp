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

    mqtt::publishStatus(stateStr(newState));
}

bool checkState(SystemState state)
{
    return (systemState == state);
}

void triggerSleepEvent()
{
    setState(SystemState::SLEEPING);
    xEventGroupSetBits(systemEvents, EVT_SLEEP);
    vTaskDelay(pdMS_TO_TICKS(100));
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
    case SystemState::BOOTING:
        return "BOOTING";
    case SystemState::NET_OFF:
        return "NET_OFF";
    case SystemState::NET_ON:
        return "NET_ON";
    case SystemState::MQTT_OFF:
        return "MQTT_OFF";
    case SystemState::IDLE:
        return "IDLE";
    case SystemState::WORKING:
        return "WORKING";
    case SystemState::WAITING_SERVER:
        return "WAITING_SERVER";
    case SystemState::SLEEPING:
        return "SLEEPING";
    default:
        return "UNKNOWN";
    }
}
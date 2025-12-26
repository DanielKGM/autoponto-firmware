#include "Globals.h"
#include "MQTT.h"

volatile SystemState systemState;
portMUX_TYPE tasksAliveMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE systemStateMux = portMUX_INITIALIZER_UNLOCKED;
volatile short int tasksAlive = 0;

// extern
volatile bool idleFlag = false;

char deviceId[13];
EventGroupHandle_t systemEvents;
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
    short int result;
    portENTER_CRITICAL(&tasksAliveMux);
    result = tasksAlive;
    portEXIT_CRITICAL(&tasksAliveMux);
    return result;
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
    bool result = false;
    portENTER_CRITICAL(&systemStateMux);
    result = (systemState == state);
    portEXIT_CRITICAL(&systemStateMux);
    return result;
}

void triggerSleepEvent()
{
    setState(SystemState::SLEEPING);
    xEventGroupSetBits(systemEvents, EVT_SLEEP);
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
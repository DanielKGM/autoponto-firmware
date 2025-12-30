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
AttendanceContext context = AttendanceContext{};

void saveContext(uint8_t flags)
{
    Preferences prefs;
    prefs.begin("context", false);

    if (flags & MSG)
        prefs.putString("msg", context.msg);

    if (flags & MS_NEXT)
        prefs.putULong("msForNext", context.msForNext);

    if (flags & MS_REM)
        prefs.putULong("msRemaining", context.msRemaining);

    if (flags & SYNCED)
        prefs.putBool("synced", context.synced);

    if (flags & UPDATE)
        prefs.putBool("shouldUpdate", context.shouldUpdate);

    prefs.end();
}

bool loadContext(uint8_t flags)
{
    Preferences prefs;

    if (!prefs.begin("context", true))
        return false;

    bool foundAny = false;

    if ((flags & MSG) && prefs.isKey("msg"))
    {
        prefs.getString("msg", context.msg, sizeof(context.msg));
        foundAny = true;
    }

    if ((flags & MS_NEXT) && prefs.isKey("msForNext"))
    {
        context.msForNext = prefs.getULong("msForNext");
        foundAny = true;
    }

    if ((flags & MS_REM) && prefs.isKey("msRemaining"))
    {
        context.msRemaining = prefs.getULong("msRemaining");
        foundAny = true;
    }

    if ((flags & SYNCED) && prefs.isKey("synced"))
    {
        context.synced = prefs.getBool("synced");
        foundAny = true;
    }

    if ((flags & UPDATE) && prefs.isKey("shouldUpdate"))
    {
        context.shouldUpdate = prefs.getBool("shouldUpdate");
        foundAny = true;
    }

    prefs.end();

    return foundAny;
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
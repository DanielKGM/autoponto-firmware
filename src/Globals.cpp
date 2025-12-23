#include "Globals.h"

volatile SystemState systemState;
char deviceId[13];

volatile int tasksAlive = 0;
portMUX_TYPE tasksAliveMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE systemStateMux = portMUX_INITIALIZER_UNLOCKED;

void changeTaskCount(int delta)
{
    portTRY_ENTER_CRITICAL(&tasksAliveMux, pdMS_TO_TICKS(1000));
    tasksAlive = tasksAlive + delta;
    portEXIT_CRITICAL(&tasksAliveMux);
}

void setSystemState(SystemState newState)
{
    portTRY_ENTER_CRITICAL(&systemStateMux, pdMS_TO_TICKS(1000));
    systemState = newState;
    portEXIT_CRITICAL(&systemStateMux);
}
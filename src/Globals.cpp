#include "Globals.h"

volatile SystemState systemState;
char deviceId[13];

volatile int tasksAlive = 0;
portMUX_TYPE tasksAliveMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE systemStateMux = portMUX_INITIALIZER_UNLOCKED;

void changeTaskCount(int delta)
{
    portENTER_CRITICAL(&tasksAliveMux);
    tasksAlive = tasksAlive + delta;
    portEXIT_CRITICAL(&tasksAliveMux);
}

void setSystemState(SystemState newState)
{
    portENTER_CRITICAL(&systemStateMux);
    systemState = newState;
    portEXIT_CRITICAL(&systemStateMux);
}

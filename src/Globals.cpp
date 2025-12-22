#include "Globals.h"

volatile SystemState current_state;

volatile int tasks_alive = 0;
portMUX_TYPE tasks_alive_mux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE system_state_mux = portMUX_INITIALIZER_UNLOCKED;

void changeTaskCount(int delta)
{
    portENTER_CRITICAL(&tasks_alive_mux);
    tasks_alive = tasks_alive + delta;
    portEXIT_CRITICAL(&tasks_alive_mux);
}

void setSystemState(SystemState new_state)
{
    portENTER_CRITICAL(&system_state_mux);
    current_state = new_state;
    portEXIT_CRITICAL(&system_state_mux);
}
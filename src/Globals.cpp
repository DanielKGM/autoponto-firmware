#include "Globals.h"

volatile bool should_sleep_flag = false;
volatile bool PIR_triggered = false;
volatile int tasks_alive = 0;
portMUX_TYPE tasks_alive_mux = portMUX_INITIALIZER_UNLOCKED;
QueueHandle_t messageQueue = NULL;

void changeAliveTaskCount(int delta)
{
    portENTER_CRITICAL(&tasks_alive_mux);
    tasks_alive += delta;
    portEXIT_CRITICAL(&tasks_alive_mux);
}
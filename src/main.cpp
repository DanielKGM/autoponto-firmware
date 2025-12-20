#include <Arduino.h>
#include "Config.h"
#include "Globals.h"
#include "Power.h"
#include "Camera.h"
#include "Display.h"
#include "Network.h"
#include "soc/rtc_cntl_reg.h"

TickType_t last_PIR_tick = 0;
const TickType_t ticks_to_sleep = pdMS_TO_TICKS(SLEEP_TIMEOUT_MS);

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    messageQueue = xQueueCreate(5, sizeof(DisplayMessage));
    last_PIR_tick = xTaskGetTickCount();

    setupPins();
    setupSleep();
    setupTFT();
    setupSprite();

    sendDisplayMessage("Iniciando...");

    if (startCamera())
    {
        xTaskCreatePinnedToCore(TaskDisplayCode, "TaskDisplay", 10240, NULL, 2, NULL, 1);
        xTaskCreatePinnedToCore(TaskNetworkCode, "TaskNetwork", 4096, NULL, 1, NULL, 0);
    }
}

void loop()
{
    if (PIR_triggered)
    {
        PIR_triggered = false;
        last_PIR_tick = xTaskGetTickCount();
    }

    if ((xTaskGetTickCount() - last_PIR_tick) > ticks_to_sleep)
    {
        should_sleep_flag = true;
    }

    if (should_sleep_flag && tasks_alive == 0)
    {
        sleep();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}
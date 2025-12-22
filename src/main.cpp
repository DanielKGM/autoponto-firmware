#include <Arduino.h>
#include "Globals.h"
#include "Power.h"
#include "Camera.h"
#include "Display.h"
#include "Network.h"
#include "soc/rtc_cntl_reg.h"

const TickType_t ticks_to_sleep = pdMS_TO_TICKS(SLEEP_TIMEOUT_MS);
TaskHandle_t TaskDisplay;
TaskHandle_t TaskNetwork;

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    setSystemState(SystemState::BOOTING);

    initPins();
    initSleep();
    initTFT();
    initSprite();
    initCamera();
    initWifi();

    if (startCamera())
    {
        setSystemState(SystemState::READY);

        xTaskCreatePinnedToCore(
            TaskDisplayCode,
            "TaskDisplay",
            12288,
            nullptr,
            1,
            &TaskDisplay,
            APP_CPU_NUM);

        xTaskCreatePinnedToCore(
            TaskNetworkCode,
            "TaskNetwork",
            8192,
            nullptr,
            1,
            &TaskNetwork,
            PRO_CPU_NUM);
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
        // setSystemState(SystemState::SLEEPING);
    }

    if (current_state == SystemState::SLEEPING && tasks_alive == 0)
    {
        // sleep();
    }

    if (!WiFi.isConnected())
    {
        setSystemState(SystemState::DISCONNECTED);
        if (connWifi())
        {
            setSystemState(SystemState::READY);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(500));
}
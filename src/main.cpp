#include <Arduino.h>
#include "Globals.h"
#include "Power.h"
#include "Camera.h"
#include "Display.h"
#include "Network.h"
#include "MQTT.h"
#include "soc/rtc_cntl_reg.h"

const TickType_t ticks_to_sleep = pdMS_TO_TICKS(SLEEP_TIMEOUT_MS);
TaskHandle_t TaskDisplay;
TaskHandle_t TaskNetwork;

void initID()
{
    Preferences prefs;
    prefs.begin("system", false);

    if (prefs.getString("deviceId", deviceId, sizeof(deviceId)) == 0)
    {
        uint64_t mac = ESP.getEfuseMac();

        snprintf(deviceId, sizeof(deviceId), "%04X%08X",
                 (uint16_t)(mac >> 32),
                 (uint32_t)mac);

        prefs.putString("deviceId", deviceId);
    }

    prefs.end();
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    setSystemState(SystemState::BOOTING);

    initID();
    initPins();
    initSleep();
    initTFT();
    initSprite();
    initCamera();
    initWifi();
    initMqtt();

    if (!startCamera())
    {
        return;
    }

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

void loop()
{
    if (sensorTriggered)
    {
        sensorTriggered = false;
        lastSensorTick = xTaskGetTickCount();
    }

    if ((xTaskGetTickCount() - lastSensorTick) > ticks_to_sleep)
    {
        // setSystemState(SystemState::SLEEPING);
    }

    if (currentState == SystemState::SLEEPING && tasksAlive == 0)
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
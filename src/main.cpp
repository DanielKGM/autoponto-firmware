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
TaskHandle_t TaskMqtt;

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

void positiveFB()
{
    digitalWrite(POSITIVE_FB_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(POSITIVE_FB_DURATION_MS));
    digitalWrite(POSITIVE_FB_PIN, LOW);
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

    xTaskCreatePinnedToCore(
        TaskDisplayCode,
        "TaskDisplay",
        12288,
        nullptr,
        1,
        &TaskDisplay,
        APP_CPU_NUM);

    if (!startCamera())
    {
        return;
    }

    xTaskCreatePinnedToCore(
        TaskNetworkCode,
        "TaskNetwork",
        8192,
        nullptr,
        1,
        &TaskNetwork,
        PRO_CPU_NUM);

    xTaskCreatePinnedToCore(
        TaskMqttCode,
        "TaskMqtt",
        4096,
        nullptr,
        1,
        &TaskMqtt,
        APP_CPU_NUM);
}

void loop()
{
    if (sensorTriggered)
    {
        sensorTriggered = false;
        lastSensorTick = xTaskGetTickCount();
    }

    if (buzzerTriggered)
    {
        setBuzzerTriggered(false);
        positiveFB();
    }

    if ((xTaskGetTickCount() - lastSensorTick) > ticks_to_sleep)
    {
        // setSystemState(SystemState::SLEEPING);
    }

    if (systemState == SystemState::SLEEPING && tasksAlive == 0)
    {
        // sleep();
    }

    vTaskDelay(pdMS_TO_TICKS(500));
}
#include "Config.h"
#include "Globals.h"
#include "Power.h"
#include "Camera.h"
#include "Display.h"
#include "Network.h"
#include "MQTT.h"

const TickType_t ticksToSleep = pdMS_TO_TICKS(SLEEP_TIMEOUT_MS);
const TickType_t loopDelay = pdMS_TO_TICKS(500);

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
    power::initPins();
    power::initSleep();
    camera::initCamera();

    xTaskCreatePinnedToCore(
        display::TaskDisplayCode,
        "TaskDisplay",
        12288,
        nullptr,
        1,
        &TaskDisplay,
        APP_CPU_NUM);

    if (!camera::startCamera())
    {
        return;
    }

    xTaskCreatePinnedToCore(
        network::TaskNetworkCode,
        "TaskNetwork",
        8192,
        nullptr,
        1,
        &TaskNetwork,
        PRO_CPU_NUM);

    xTaskCreatePinnedToCore(
        mqtt::TaskMqttCode,
        "TaskMqtt",
        4096,
        nullptr,
        1,
        &TaskMqtt,
        APP_CPU_NUM);
}

void loop()
{
    using namespace power;

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

    if ((xTaskGetTickCount() - lastSensorTick) > ticksToSleep)
    {
        triggerSleepEvent();
    }

    if (checkSleepEvent(loopDelay) && checkTaskCount() == 0)
    {
        sleep();
    }
}
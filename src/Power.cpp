#include "Power.h"
#include "Globals.h"
#include "Config.h"
#include "Camera.h"
#include "MQTT.h"
#include "Network.h"

namespace power
{
    volatile bool buzzerTriggered = false;
    volatile bool sensorTriggered = false;

    namespace
    {
        volatile bool idleFlag = false;

        portMUX_TYPE sensorMux = portMUX_INITIALIZER_UNLOCKED;

        void IRAM_ATTR handlePIRInterrupt()
        {
            portENTER_CRITICAL_ISR(&sensorMux);
            sensorTriggered = true;
            portEXIT_CRITICAL_ISR(&sensorMux);
        }
    }
    bool checkIdle()
    {
        return idleFlag;
    }

    void changeTriggeredFlag(bool value)
    {
        portENTER_CRITICAL(&sensorMux);
        sensorTriggered = value;
        portEXIT_CRITICAL(&sensorMux);
    }

    void positiveFB()
    {
        digitalWrite(POSITIVE_FB_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(POSITIVE_FB_DURATION_MS));
        digitalWrite(POSITIVE_FB_PIN, LOW);
    }

    void enterIdle()
    {
        if (idleFlag)
        {
            return;
        }

        digitalWrite(DISPLAY_ENABLE_PIN, LOW);
        idleFlag = true;
        mqtt::publishStatus("idle");
    }

    void exitIdle()
    {
        if (!idleFlag)
        {
            return;
        }

        idleFlag = false;
        digitalWrite(DISPLAY_ENABLE_PIN, HIGH);
        esp_wifi_set_ps(WIFI_PS_NONE);

        if (checkState(SystemState::FETCHING))
        {
            mqtt::publishStatus("fetching");
        }
        else if (checkState(SystemState::WORKING))
        {
            mqtt::publishStatus("working");
        }
    }

    bool isBlockingIdle()
    {
        switch (systemState)
        {
        case SystemState::BOOTING:
        case SystemState::SLEEPING:
            return true;

        default:
            return sensorTriggered;
        }
    }

    void configPins()
    {
        pinMode(DISPLAY_ENABLE_PIN, OUTPUT);
        pinMode(PIR_PIN, INPUT);
        pinMode(PWDN_GPIO_NUM, OUTPUT);
        digitalWrite(POSITIVE_FB_PIN, LOW);
        pinMode(POSITIVE_FB_PIN, OUTPUT);
        digitalWrite(DISPLAY_ENABLE_PIN, HIGH);
        digitalWrite(POSITIVE_FB_PIN, LOW);

        attachInterrupt(digitalPinToInterrupt(PIR_PIN), handlePIRInterrupt, RISING);
    }

    void configSleep()
    {
        esp_sleep_enable_ext0_wakeup((gpio_num_t)PIR_PIN, 1);
        rtc_gpio_pullup_dis((gpio_num_t)PIR_PIN);
        rtc_gpio_pulldown_en((gpio_num_t)PIR_PIN);
    }

    void triggerSleepEvent()
    {
        setState(SystemState::SLEEPING);
        vTaskDelay(pdMS_TO_TICKS(500));
        xEventGroupSetBits(systemEvents, EVT_SLEEP);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    __attribute__((noreturn)) void sleep()
    {
        detachInterrupt(digitalPinToInterrupt(PIR_PIN));
        digitalWrite(DISPLAY_ENABLE_PIN, LOW);
        digitalWrite(POSITIVE_FB_PIN, LOW);
        esp_camera_deinit();
        esp_deep_sleep_start();
    }
}

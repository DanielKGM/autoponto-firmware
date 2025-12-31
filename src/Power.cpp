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
        setCpuFrequencyMhz(160);
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        idleFlag = true;
        setState(SystemState::IDLE);
    }

    void exitIdle()
    {
        if (!idleFlag)
        {
            return;
        }

        idleFlag = false;
        setCpuFrequencyMhz(240);
        digitalWrite(DISPLAY_ENABLE_PIN, HIGH);
        esp_wifi_set_ps(WIFI_PS_NONE);

        if (!network::isConnected())
        {
            setState(SystemState::NET_OFF);
            return;
        }

        if (!mqtt::isConnected())
        {
            setState(SystemState::MQTT_OFF);
            return;
        }

        setState(SystemState::FETCHING);
    }

    void configPins()
    {
        pinMode(DISPLAY_ENABLE_PIN, OUTPUT);
        pinMode(PIR_PIN, INPUT);
        pinMode(PWDN_GPIO_NUM, OUTPUT);
        pinMode(POSITIVE_FB_PIN, OUTPUT);
        digitalWrite(DISPLAY_ENABLE_PIN, HIGH);
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
        xEventGroupSetBits(systemEvents, EVT_SLEEP);
        vTaskDelay(pdMS_TO_TICKS(500));
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
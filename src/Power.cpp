#include "Power.h"
#include "Globals.h"
#include "Config.h"
#include "Camera.h"

namespace power
{
    volatile bool sensorTriggered = false;
    volatile bool buzzerTriggered = false;

    namespace
    {
        void IRAM_ATTR handlePIRInterrupt()
        {
            sensorTriggered = true;
        }
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
        pinMode(PWDN_GPIO_NUM, OUTPUT);
        digitalWrite(PWDN_GPIO_NUM, HIGH);
        setCpuFrequencyMhz(80);
        WiFi.setSleep(true);

        setState(SystemState::IDLE);
        idleFlag = true;
    }

    void exitIdle()
    {
        if (!idleFlag)
        {
            return;
        }

        setCpuFrequencyMhz(240);
        digitalWrite(PWDN_GPIO_NUM, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(DISPLAY_ENABLE_PIN, HIGH);

        setState(SystemState::WORKING);
        idleFlag = false;
    }

    void configPins()
    {
        pinMode(DISPLAY_ENABLE_PIN, OUTPUT);
        pinMode(PIR_PIN, INPUT);
        pinMode(POSITIVE_FB_PIN, OUTPUT);
        digitalWrite(DISPLAY_ENABLE_PIN, HIGH);
        attachInterrupt(digitalPinToInterrupt(PIR_PIN), handlePIRInterrupt, CHANGE);
    }

    void configSleep()
    {
        esp_sleep_enable_ext0_wakeup((gpio_num_t)PIR_PIN, 1);
        rtc_gpio_pullup_dis((gpio_num_t)PIR_PIN);
        rtc_gpio_pulldown_en((gpio_num_t)PIR_PIN);
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
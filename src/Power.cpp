#include "Power.h"
#include "Globals.h"

volatile TickType_t last_PIR_tick = xTaskGetTickCount();
volatile bool PIR_triggered = false;

void IRAM_ATTR handlePIRInterrupt()
{
    PIR_triggered = true;
}

void initPins()
{
    pinMode(DISPLAY_ENABLE_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(POSITIVE_FB_PIN, OUTPUT);
    digitalWrite(DISPLAY_ENABLE_PIN, HIGH);
    attachInterrupt(digitalPinToInterrupt(PIR_PIN), handlePIRInterrupt, CHANGE);
}

void initSleep()
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
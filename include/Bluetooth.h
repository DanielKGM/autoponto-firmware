#pragma once
#include <ArduinoJson.h>
#include <NimBLEDevice.h>

namespace bluetooth
{
    bool shouldEnterConfigMode();
    void runConfigMode();
}
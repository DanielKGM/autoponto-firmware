#pragma once
#include <ArduinoJson.h>
#include <BluetoothSerial.h>

namespace bluetooth
{
    bool shouldEnterConfigMode();
    void runConfigMode();
}
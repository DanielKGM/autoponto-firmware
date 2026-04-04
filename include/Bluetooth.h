#pragma once
#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include "driver/gpio.h"

namespace bluetooth
{
    bool shouldEnterConfigMode();
    void runConfigMode();
}
#include "Display.h"
#include "Network.h"
#include "Config.h"
#include "Globals.h"

bool sendFrame()
{
    // TODO: ENVIAR FRAME PARA O SERVIDOR
    sendDisplayMessage("Frame enviado", 1000);
    return true;
}

void TaskNetworkCode(void *pvParameters)
{
    changeAliveTaskCount(1);

    const TickType_t tickDelay = pdMS_TO_TICKS(100);
    const TickType_t ticks_to_send = pdMS_TO_TICKS(DATA_SEND_INTERVAL_MS);
    const unsigned short send_its = ticks_to_send / tickDelay;
    unsigned short it_cnt = 0;

    while (!should_sleep_flag)
    {

        if (++it_cnt >= send_its)
        {
            it_cnt = 0;
            sendFrame();
        }

        vTaskDelay(tickDelay);
    }

    changeAliveTaskCount(-1);
    vTaskDelete(nullptr);
}
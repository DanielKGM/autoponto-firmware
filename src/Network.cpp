#include "Display.h"
#include "Network.h"
#include "Globals.h"

WiFiClient wifiClient;

bool sendFrame()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    sendDisplayMessage("ENVIADO", 2000);

    return true;
}

bool connWifi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    char msg[DISPLAY_MSG_MAX_LEN];

    short int dots = 0;

    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(WIFI_TIMEOUT_MS);

    while (WiFi.status() != WL_CONNECTED)
    {
        if (xTaskGetTickCount() - start > timeout)
        {
            sendDisplayMessage("WiFi falhou!", 2000);
            return false;
        }

        dots = (dots + 1) % 4;

        snprintf(msg, sizeof(msg),
                 "Conectando ao WiFi%.*s",
                 dots, "...");

        sendDisplayMessage(msg);

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    return true;
}

bool initWifi()
{
    WiFi.mode(WIFI_STA);

    return connWifi();
}

void TaskNetworkCode(void *pvParameters)
{
    changeTaskCount(1);

    if (initWifi())
    {
        setSystemState(SystemState::NET_ON);
    }

    const TickType_t tickDelay = pdMS_TO_TICKS(100);
    const TickType_t ticks_to_send = pdMS_TO_TICKS(POST_INTERVAL_MS);
    const unsigned short send_its = ticks_to_send / tickDelay;
    unsigned short it_cnt = 0;

    while (systemState != SystemState::SLEEPING)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            setSystemState(SystemState::NET_OFF);

            if (!connWifi())
            {
                vTaskDelay(tickDelay);
                continue;
            }

            setSystemState(SystemState::NET_ON);
        }

        if (it_cnt++ > send_its)
        {
            it_cnt = 0;

            if (systemState == SystemState::READY)
            {
                sendFrame();
            }
        }

        vTaskDelay(tickDelay);
    }

    changeTaskCount(-1);
    vTaskDelete(nullptr);
}
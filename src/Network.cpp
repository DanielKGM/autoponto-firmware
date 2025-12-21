#include "Display.h"
#include "Network.h"
#include "Globals.h"
#include "Secrets.h"

bool should_send_flag = false;

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
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    char msg[DISPLAY_MSG_MAX_LEN];
    strcpy(msg, "Conectando ao WiFi");
    sendDisplayMessage(msg);

    while (WiFi.status() != WL_CONNECTED)
    {
        if (strlen(msg) < DISPLAY_MSG_MAX_LEN - 1)
        {
            strcat(msg, ".");
        }

        sendDisplayMessage(msg);
        vTaskDelay(pdMS_TO_TICKS(CONN_WAIT_INTERVAL_MS));
    }

    return true;
}

bool initWifi()
{
    WiFi.mode(WIFI_STA);

    IPAddress local_IP(LOCAL_IP);
    IPAddress gateway(GATEWAY);
    IPAddress subnet(SUBNET);

    if (!WiFi.config(local_IP, gateway, subnet))
    {
        sendDisplayMessage("Configurando WiFi...");
        while (!WiFi.config(local_IP, gateway, subnet))
        {
            vTaskDelay(pdMS_TO_TICKS(CONN_WAIT_INTERVAL_MS));
        }
    }

    WiFi.setHostname(HOST_NAME);

    return connWifi();
}

void TaskNetworkCode(void *pvParameters)
{
    changeAliveTaskCount(1);

    const TickType_t tickDelay = pdMS_TO_TICKS(100);
    const TickType_t ticks_to_send = pdMS_TO_TICKS(DATA_SEND_INTERVAL_MS);
    const unsigned short send_its = ticks_to_send / tickDelay;
    unsigned short it_cnt = 0;

    should_send_flag = initWifi();

    while (!should_sleep_flag)
    {
        if (++it_cnt >= send_its)
        {
            it_cnt = 0;

            if (WiFi.status() != WL_CONNECTED)
            {
                should_send_flag = connWifi();
                continue;
            }

            if (should_send_flag)
            {
                sendFrame();
            }
        }

        vTaskDelay(tickDelay);
    }

    changeAliveTaskCount(-1);
    vTaskDelete(nullptr);
}
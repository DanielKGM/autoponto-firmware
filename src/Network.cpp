#include "Display.h"
#include "Network.h"
#include "Globals.h"
#include "Secrets.h"

WiFiClientSecure client;
bool should_send_flag = false;

bool sendFrame()
{
    // TODO: ENVIAR FRAME PARA O SERVIDOR
    sendDisplayMessage("Frame enviado", 1000);
    return true;
}

bool connWifi(const char *txt)
{
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    char msg[DISPLAY_MSG_MAX_LEN];
    strncpy(msg, txt, DISPLAY_MSG_MAX_LEN - 1);
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

void WiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    should_send_flag = connWifi("Conexão perdida! Reconectando");
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

    WiFi.onEvent(WiFiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.setHostname(HOST_NAME);

    return connWifi("Conectando ao WiFi");
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
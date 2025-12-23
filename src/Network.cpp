#include "Display.h"
#include "Config.h"
#include "Network.h"
#include "Globals.h"

WiFiClient wifiClient;

bool sendFrame()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    FrameBuffer frame;

    if (xQueueReceive(frameQueue, &frame, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        return false;
    }

    HTTPClient http;
    http.setTimeout(REST_TIMEOUT_MS);
    http.begin(REST_URL);

    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-Device-Id", deviceId);
    http.addHeader("X-Auth", REST_PASS);

    int resp = http.POST(frame.data, frame.len);
    http.end();
    free(frame.data);

    if (resp != HTTP_CODE_OK)
    {
        sendDisplayMessage("Erro ao enviar captura!", 1000);
    }

    return resp != HTTP_CODE_OK;
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

    TickType_t lastReqTick = 0;

    const TickType_t reqInterval = pdMS_TO_TICKS(REST_POST_INTERVAL_MS);
    const TickType_t waitInterval = pdMS_TO_TICKS(RESPONSE_WAIT_TIMEOUT_MS);

    while (systemState != SystemState::SLEEPING)
    {
        TickType_t now = xTaskGetTickCount();

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

        if (systemState == SystemState::READY && (now - lastReqTick) > reqInterval)
        {
            if (TaskDisplay)
            {
                xTaskNotifyGive(TaskDisplay);
            }

            lastReqTick = now;

            if (sendFrame())
            {
                setSystemState(SystemState::WAITING_SERVER);
            }
        }

        if (systemState == SystemState::WAITING_SERVER && (now - lastReqTick) > waitInterval)
        {
            setSystemState(SystemState::READY);
        }

        vTaskDelay(tickDelay);
    }

    changeTaskCount(-1);
    vTaskDelete(nullptr);
}
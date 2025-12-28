#include "Network.h"
#include "Display.h"
#include "Config.h"
#include "Globals.h"

namespace network
{
    WiFiClient wifiClient;

    namespace
    {
        bool sendFrame()
        {
            using namespace display;

            if (!isConnected())
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
                sendDisplayMessage("Erro ao enviar captura!", 3000, &ICON_SAD);
            }

            return resp == HTTP_CODE_OK;
        }

        bool connWifi()
        {
            using namespace display;

            WiFi.begin(WIFI_SSID, WIFI_PASS);

            char msg[DISPLAY_MSG_MAX_LEN];

            short int dots = 0;

            const TickType_t start = xTaskGetTickCount();
            const TickType_t timeout = pdMS_TO_TICKS(WIFI_TIMEOUT_MS);

            while (!isConnected())
            {
                if (xTaskGetTickCount() - start > timeout)
                {
                    sendDisplayMessage("WiFi falhou!", 2000, &ICON_SAD);
                    return false;
                }

                dots = (dots + 1) % 4;

                snprintf(msg, sizeof(msg),
                         "Conectando ao WiFi%.*s",
                         dots, "...");

                sendDisplayMessage(msg, 0, &ICON_WIFI);

                vTaskDelay(pdMS_TO_TICKS(500));
            }

            return true;
        }

        bool configWifi()
        {
            WiFi.mode(WIFI_STA);

            return connWifi();
        }
    } //

    int8_t getRSSI()
    {
        return isConnected() ? WiFi.RSSI() : 0;
    }

    bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    void TaskNetworkCode(void *pvParameters)
    {
        changeTaskCount(1);

        setState(SystemState::NET_OFF);

        if (configWifi())
        {
            setState(SystemState::NET_ON);
        }

        const TickType_t delay = pdMS_TO_TICKS(100);
        const TickType_t reqInterval = pdMS_TO_TICKS(REST_POST_INTERVAL_MS);
        const TickType_t waitInterval = pdMS_TO_TICKS(RESPONSE_WAIT_TIMEOUT_MS);

        TickType_t lastReqTick = 0;

        while (true)
        {
            if (checkSleepEvent(delay))
            {
                break;
            }

            TickType_t now = xTaskGetTickCount();

            if (!isConnected())
            {
                setState(SystemState::NET_OFF);

                if (connWifi())
                {
                    setState(SystemState::NET_ON);
                }

                continue;
            }

            if (checkState(SystemState::WAITING_SERVER))
            {
                if (now - lastReqTick > waitInterval)
                {
                    setState(SystemState::WORKING);
                }

                continue;
            }

            if (checkState(SystemState::WORKING) &&
                now - lastReqTick > reqInterval)
            {
                lastReqTick = now;

                if (TaskDisplay)
                {
                    xTaskNotifyGive(TaskDisplay);
                }

                if (sendFrame())
                {
                    setState(SystemState::WAITING_SERVER);
                }
            }
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }

}
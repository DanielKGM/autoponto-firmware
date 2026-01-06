#include "Network.h"
#include "Display.h"
#include "Config.h"
#include "Globals.h"
#include "Power.h"
#include "MQTT.h"

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

            if (xQueueReceive(frameQueue, &frame, 1000) != pdTRUE)
            {
                return false;
            }

            HTTPClient http;
            http.setTimeout(REST_TIMEOUT_MS);
            http.begin(REST_POST_URL);

            http.addHeader("Content-Type", "image/jpeg");
            http.addHeader("X-Device-Id", deviceId);
            http.addHeader("Connection", "close");
            http.addHeader("X-Auth", REST_PASS);

            int resp = http.POST(frame.data, frame.len);

            if (resp != HTTP_CODE_OK)
            {
                vTaskDelay(pdMS_TO_TICKS(200));
                resp = http.POST(frame.data, frame.len);
            }

            http.end();
            free(frame.data);

            if (resp != HTTP_CODE_OK)
            {
                sendDisplayMessage("Servidor indisponivel para envios!", 3000, &ICON_SAD);
            }

            setState(SystemState::WAITING_SERVER);
            return resp == HTTP_CODE_OK;
        }

        bool getContext()
        {
            using namespace display;

            if (!isConnected())
            {
                return false;
            }

            if (context.chair[0] != '\0' &&
                (context.msForNext > 0 || context.msRemaining > 0))
            {
                return true;
            }

            HTTPClient http;
            http.setTimeout(REST_TIMEOUT_MS);
            http.begin(String(REST_GET_URL) + String(deviceId));
            http.addHeader("X-Device-Id", deviceId);
            http.addHeader("X-Auth", REST_PASS);

            sendDisplayMessage("Coletando informações da turma...", 0, &ICON_SERVER);

            int resp = http.GET();

            if (resp != HTTP_CODE_OK)
            {
                vTaskDelay(pdMS_TO_TICKS(200));
                resp = http.GET();
            }

            if (resp != HTTP_CODE_OK)
            {
                sendDisplayMessage("Servidor indisponivel para sincronizacao!", 3000, &ICON_SAD);
                http.end();
                return false;
            }

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, http.getStream());

            if (error)
            {
                sendDisplayMessage("Erro em analisar as informações!", 3000, &ICON_SAD);
                http.end();
                return false;
            }

            strlcpy(context.chair, doc["chair"] | "", sizeof(context.chair));
            context.msForNext = pdMS_TO_TICKS(doc["msForNext"]) | 0;
            context.msRemaining = pdMS_TO_TICKS(doc["msRemaining"]) | 0;
            context.fetchTime = xTaskGetTickCount();
            http.end();

            mqtt::publish(mqtt::topicLogs, "{\"synced\":true}", true);

            return true;
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
        const TickType_t waitInterval = pdMS_TO_TICKS(RESPONSE_WAIT_TIMEOUT_MS);
        const TickType_t reqInterval = pdMS_TO_TICKS(REST_POST_INTERVAL_MS);

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

            const TickType_t lastRequestInterval = now - lastReqTick;
            bool waitTimeOut = lastRequestInterval > waitInterval;
            bool shouldSendFrame = lastRequestInterval > reqInterval && context.msRemaining;

            if (checkState(SystemState::FETCHING) && getContext())
            {
                setState(power::checkIdle() ? SystemState::IDLE : SystemState::WORKING);
            }
            else if (checkState(SystemState::WAITING_SERVER) && waitTimeOut)
            {
                lastReqTick = now;
                setState(power::checkIdle() ? SystemState::IDLE : SystemState::WORKING);
            }
            else if (checkState(SystemState::WORKING) && shouldSendFrame)
            {
                lastReqTick = now;
                xTaskNotifyGive(TaskDisplay);
                sendFrame();
            }
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }

}
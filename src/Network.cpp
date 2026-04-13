#include "Network.h"
#include "Display.h"
#include "Globals.h"
#include "Power.h"
#include "MQTT.h"
#include "RuntimeConfig.h"
#include "Config.h"
#include "Camera.h"

namespace network
{
    WiFiClient wifiClient;

    namespace
    {
        bool sendFrame()
        {
            if (!isConnected())
            {
                return false;
            }

            if (!camera::requestSnapshot())
            {
                return false;
            }

            camera::FrameBuffer frame;
            if (!camera::takeSnapshotFrame(frame, pdMS_TO_TICKS(500)))
            {
                display::sendDisplayMessage("Falha ao capturar frame!", 2000, &display::ICON_SAD);
                return false;
            }

            HTTPClient http;
            http.setTimeout(REST_TIMEOUT_MS);
            http.begin(String(runtimeConfig.restUrl) + String(runtimeConfig.restPostPath));

            http.addHeader("Content-Type", "image/jpeg");
            http.addHeader("X-Device-Id", deviceId);
            http.addHeader("Connection", "close");
            http.addHeader("X-Auth", runtimeConfig.restPass);

            int resp = http.POST(frame.data, frame.len);

            if (resp != HTTP_CODE_OK)
            {
                vTaskDelay(pdMS_TO_TICKS(200));
                resp = http.POST(frame.data, frame.len);
            }

            http.end();
            camera::releaseFrame(frame);

            bool ok = resp == HTTP_CODE_OK;

            if (!ok)
            {
                display::sendDisplayMessage("Servidor indisponivel para envios!", 2000, &display::ICON_SAD);
                return false;
            }

            setState(SystemState::WAITING_SERVER);
            return true;
        }

        bool getContext()
        {
            using namespace display;

            if (!isConnected())
            {
                return false;
            }

            if (context.lesson_name[0] != '\0' &&
                (context.ticksForNext > 0 || context.ticksRemaining > 0))
            {
                return true;
            }

            HTTPClient http;
            http.setTimeout(REST_TIMEOUT_MS);
            http.begin(String(runtimeConfig.restUrl) + String(runtimeConfig.restFetchPath));
            http.addHeader("X-Device-Id", deviceId);
            http.addHeader("X-Auth", runtimeConfig.restPass);

            sendDisplayMessage("Coletando informações da turma...", 0, &ICON_SERVER);

            int resp = http.GET();

            if (resp != HTTP_CODE_OK)
            {
                vTaskDelay(pdMS_TO_TICKS(200));
                resp = http.GET();
            }

            if (resp != HTTP_CODE_OK)
            {
                sendDisplayMessage("Servidor indisponivel para sincronizacao!", 2000, &ICON_SAD);
                http.end();
                return false;
            }

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, http.getStream());

            if (error)
            {
                sendDisplayMessage("Erro em analisar as informações!", 2000, &ICON_SAD);
                http.end();
                return false;
            }

            strlcpy(context.lesson_name, doc["lesson_name"] | "", sizeof(context.lesson_name));
            context.ticksForNext = pdMS_TO_TICKS(doc["msForNext"]) | 0;
            context.ticksRemaining = pdMS_TO_TICKS(doc["msRemaining"]) | 0;
            context.fetchTick = xTaskGetTickCount();
            http.end();

            if (!(context.ticksForNext || context.ticksRemaining))
            {
                sendDisplayMessage("Sem agendamento disponivel para o dispositivo!", 2000, &ICON_SAD);
                return false;
            }

            mqtt::publish(mqtt::topicLogs, "{\"synced\":true}", true);

            return true;
        }

        bool connWifi()
        {
            using namespace display;

            WiFi.begin(runtimeConfig.wifiSsid, runtimeConfig.wifiPass);

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
            WiFi.setHostname(deviceId);

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
            setState(SystemState::MQTT_OFF);
        }

        const TickType_t delay = pdMS_TO_TICKS(100);
        const TickType_t waitInterval = pdMS_TO_TICKS(RESPONSE_WAIT_TIMEOUT_MS);
        const TickType_t reqInterval = pdMS_TO_TICKS(REST_POST_INTERVAL_MS);

        const TickType_t fetchRetryInterval = pdMS_TO_TICKS(5000);
        TickType_t lastFetchAttempt = 0;

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
                    setState(SystemState::MQTT_OFF);
                }

                continue;
            }

            const TickType_t lastRequestInterval = now - lastReqTick;
            bool waitTimeOut = lastRequestInterval > waitInterval;
            bool shouldSendFrame = lastRequestInterval > reqInterval && context.ticksRemaining;

            if (checkState(SystemState::FETCHING) &&
                (now - lastFetchAttempt) > fetchRetryInterval)
            {
                lastFetchAttempt = now;

                if (getContext())
                {
                    setState(SystemState::WORKING);
                }
            }
            else if (checkState(SystemState::WAITING_SERVER) && waitTimeOut)
            {
                lastReqTick = now;
                setState(SystemState::WORKING);
            }
            else if (!power::checkIdle() && checkState(SystemState::WORKING) && shouldSendFrame)
            {
                lastReqTick = now;
                sendFrame();
            }
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }

}
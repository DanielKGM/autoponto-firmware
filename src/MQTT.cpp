#include "MQTT.h"
#include "Config.h"
#include "Globals.h"
#include "Network.h"
#include "Display.h"
#include "Power.h"

#define MQTT_TOPIC_SIZE 17 // deviceId + '/' + ('log' | 'cmd') + '/0'
#define MQTT_PAYLOAD_MAX 256

namespace mqtt
{
    namespace
    {
        char topicCmd[MQTT_TOPIC_SIZE];
        char topicLogs[MQTT_TOPIC_SIZE];
        char topicStatus[MQTT_TOPIC_SIZE];
        PubSubClient mqtt;
        QueueHandle_t mqttQueue = xQueueCreate(5, MQTT_PAYLOAD_MAX);

        void buildTopics()
        {
            snprintf(topicCmd, sizeof(topicCmd),
                     "cmd/%s", deviceId);

            snprintf(topicLogs, sizeof(topicLogs),
                     "log/%s", deviceId);

            snprintf(topicStatus, sizeof(topicStatus),
                     "sts/%s", deviceId);
        }

        void publishLog(const char *tag, const char *msg)
        {
            if (!mqttQueue)
            {
                return;
            }

            JsonDocument doc;
            doc["id"] = deviceId;
            doc["tag"] = tag;
            doc["msg"] = msg;

            if (doc.overflowed() || measureJson(doc) >= MQTT_PAYLOAD_MAX)
            {
                return;
            }

            char out[MQTT_PAYLOAD_MAX];
            serializeJson(doc, out, sizeof(out));

            xQueueSend(mqttQueue, out, 0);
        }

        void publishSystemStats()
        {
            JsonDocument doc;

            doc["cpumhz"] = ESP.getCpuFreqMHz();
            doc["rssi"] = WiFi.isConnected() ? WiFi.RSSI() : 0;
            doc["heap_free"] = ESP.getFreeHeap();
            doc["heap_min"] = ESP.getMinFreeHeap();

            if (psramFound())
            {
                doc["psram_free"] = ESP.getFreePsram();
                doc["psram_total"] = ESP.getPsramSize();
            }

            doc["stk_disp"] = TaskDisplay ? uxTaskGetStackHighWaterMark(TaskDisplay) : 0;
            doc["stk_net"] = TaskNetwork ? uxTaskGetStackHighWaterMark(TaskNetwork) : 0;
            doc["stk_mqtt"] = TaskMqtt ? uxTaskGetStackHighWaterMark(TaskMqtt) : 0;

            TaskHandle_t loopTask = xTaskGetHandle("loopTask");
            doc["stk_loop"] = loopTask ? uxTaskGetStackHighWaterMark(loopTask) : 0;

            if (doc.overflowed())
            {
                return;
            }

            static char msg[MQTT_PAYLOAD_MAX];
            serializeJson(doc, msg, sizeof(msg));

            publishLog("stats", msg);
        }

        bool connMqtt()
        {
            using namespace display;

            const TickType_t start = xTaskGetTickCount();
            const TickType_t timeout = pdMS_TO_TICKS(MQTT_TIMEOUT_MS);

            while (!isConnected())
            {
                if (!network::isConnected() || (xTaskGetTickCount() - start > timeout))
                {
                    sendDisplayMessage("MQTT falhou!", 2000, &ICON_SAD);
                    return false;
                }

                sendDisplayMessage("Conectando ao Broker...", 0, &ICON_SERVER);

                mqtt.connect(
                    deviceId,
                    MQTT_USER,
                    MQTT_PASS,
                    topicStatus,
                    1,
                    true,
                    "OFFLINE",
                    true);

                vTaskDelay(pdMS_TO_TICKS(500));
            }

            mqtt.subscribe(topicCmd, 1);
            return true;
        }

        void mqttCallback(char *topic, byte *payload, unsigned int length)
        {
            JsonDocument doc;

            if (deserializeJson(doc, payload, length))
            {
                return;
            }

            if (doc["auth"].is<bool>() && doc["auth"])
            {
                power::buzzerTriggered = true;
            }

            if (doc["stats"].is<bool>())
            {
                publishSystemStats();
            }
        }

        bool configMqtt()
        {
            buildTopics();

            mqtt.setClient(network::wifiClient);
            mqtt.setServer(MQTT_URL, MQTT_PORT);
            mqtt.setCallback(mqttCallback);

            return true;
        }
    } //

    bool isConnected()
    {
        return mqtt.connected();
    }

    void publishStatus(const char *payload)
    {
        if (isConnected())
        {
            mqtt.publish(topicStatus, payload, true);
        }
    }

    void TaskMqttCode(void *pvParameters)
    {
        changeTaskCount(1);

        configMqtt();

        const TickType_t delay = pdMS_TO_TICKS(100);

        // wait first boot and first connection
        while (checkState(SystemState::BOOTING) || checkState(SystemState::NET_OFF))
        {
            if (checkSleepEvent(delay))
            {
                break;
            }
        }

        char lastMsg[MQTT_PAYLOAD_MAX];
        TickType_t lastLogTick = 0;
        const TickType_t logInterval = pdMS_TO_TICKS(MQTT_LOG_INTERVAL_MS);

        while (true)
        {
            if (checkSleepEvent(delay))
            {
                break;
            }

            if (checkState(SystemState::NET_OFF))
            {
                continue;
            }

            if (!isConnected())
            {
                setState(SystemState::MQTT_OFF);

                if (connMqtt())
                {
                    setState(idleFlag ? SystemState::IDLE : SystemState::WORKING);
                }

                continue;
            }

            if ((xTaskGetTickCount() - lastLogTick) > logInterval && xQueueReceive(mqttQueue, lastMsg, 0) == pdTRUE)
            {
                lastLogTick = xTaskGetTickCount();
                mqtt.publish(topicLogs, lastMsg, false);
            }

            mqtt.loop();
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }
}

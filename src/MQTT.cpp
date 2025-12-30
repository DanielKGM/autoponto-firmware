#include "MQTT.h"
#include "Config.h"
#include "Globals.h"
#include "Network.h"
#include "Display.h"

namespace mqtt
{
    char topicCmd[MQTT_TOPIC_SIZE];
    char topicLogs[MQTT_TOPIC_SIZE];
    char topicStatus[MQTT_TOPIC_SIZE];

    namespace
    {
        struct MqttMsg
        {
            char topic[MQTT_TOPIC_SIZE];
            char payload[MQTT_PAYLOAD_MAX];
            bool retain;
        };

        PubSubClient mqtt;
        QueueHandle_t mqttQueue = xQueueCreate(5, sizeof(MqttMsg));

        void buildTopics()
        {
            snprintf(topicCmd, sizeof(topicCmd),
                     "cmd/%s", deviceId);

            snprintf(topicLogs, sizeof(topicLogs),
                     "log/%s", deviceId);

            snprintf(topicStatus, sizeof(topicStatus),
                     "sts/%s", deviceId);
        }

        void publishSystemStats()
        {
            JsonDocument doc;

            doc["id"] = deviceId;
            doc["cpu_freq"] = ESP.getCpuFreqMHz();
            doc["rssi"] = network::getRSSI();
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

            MqttMsg msg{};
            strcpy(msg.topic, topicLogs);
            serializeJson(doc, msg.payload, sizeof(msg.payload));
            msg.retain = false;

            xQueueSend(mqttQueue, &msg, 0);
        }

        bool connMqtt()
        {
            using namespace display;

            if (!network::isConnected())
            {
                return false;
            }

            sendDisplayMessage("Conectando ao Broker...", 0, &ICON_SERVER);

            bool ok = mqtt.connect(
                deviceId,
                MQTT_USER,
                MQTT_PASS,
                topicStatus,
                1,
                true,
                "OFFLINE",
                true);

            if (!ok)
            {
                sendDisplayMessage("Broker indisponivel!", 2000, &ICON_SAD);
                return false;
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

            if (doc["stats"].is<bool>())
            {
                publishSystemStats();
            }

            if (doc["fetch"].is<bool>())
            {
                clearContext();
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

    void publish(char *topic, const char *payload, bool retain)
    {
        if (!isConnected())
        {
            return;
        }

        MqttMsg msg{};
        strcpy(msg.topic, topic);
        strcpy(msg.payload, payload);
        msg.retain = retain;

        xQueueSend(mqttQueue, &msg, 0);
    }

    void TaskMqttCode(void *pvParameters)
    {
        changeTaskCount(1);

        configMqtt();

        const TickType_t delay = pdMS_TO_TICKS(100);

        // wait boot and first connection
        while (checkState(SystemState::BOOTING) || checkState(SystemState::NET_OFF))
        {
            if (checkSleepEvent(delay))
            {
                break;
            }
        }

        MqttMsg lastMsg{};
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
                mqtt.loop();
                continue;
            }

            if (!isConnected())
            {
                setState(SystemState::MQTT_OFF);

                if (connMqtt())
                {
                    vTaskDelay(pdMS_TO_TICKS(500)); // flapping
                    setState(SystemState::FETCHING);
                }

                mqtt.loop();
                continue;
            }

            if ((xTaskGetTickCount() - lastLogTick) > logInterval && xQueueReceive(mqttQueue, &lastMsg, 0) == pdTRUE)
            {
                lastLogTick = xTaskGetTickCount();
                mqtt.publish(lastMsg.topic, lastMsg.payload, lastMsg.retain);
            }

            mqtt.loop();
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }
}

#include "MQTT.h"
#include "Globals.h"
#include "Network.h"
#include "Display.h"
#include "Power.h"
#include "Config.h"

namespace mqtt
{
    char topicCmd[MQTT_TOPIC_SIZE];
    char topicLogs[MQTT_TOPIC_SIZE];

    namespace
    {
        constexpr const char *STATUS_OFFLINE_PAYLOAD = "{\"kind\":\"status\",\"status\":\"offline\"}";

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
        }

        void publishSystemStats()
        {
            JsonDocument doc;
            TickType_t nowTick = xTaskGetTickCount();

            doc["kind"] = "metrics";
            doc["heap_free"] = ESP.getFreeHeap();
            doc["psram_free"] = psramFound() ? ESP.getFreePsram() : 0;
            doc["now_ms"] = esp_timer_get_time() / 1000ULL;
            doc["rssi"] = network::getRSSI();
            doc["heap_min"] = ESP.getMinFreeHeap();
            doc["lesson"] = context.lesson_name;
            doc["remaining_ms"] = context.msRemaining > 0
                                      ? getRemainingMs(nowTick, context.msRemaining, context.fetchTick)
                                      : 0;
            doc["next_ms"] = context.msForNext > 0
                                 ? getRemainingMs(nowTick, context.msForNext, context.fetchTick)
                                 : 0;

            if (doc.overflowed())
            {
                return;
            }

            MqttMsg msg{};
            strcpy(msg.topic, topicLogs);

            size_t n = measureJson(doc);

            if (n >= sizeof(msg.payload))
            {
                return;
            }

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
                topicLogs,
                1,
                true,
                STATUS_OFFLINE_PAYLOAD,
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

            if (doc["auth"].is<bool>() && checkState(SystemState::WAITING_SERVER))
            {
                setState(SystemState::WORKING);

                if (doc["auth"] && !power::checkIdle())
                {
                    display::sendDisplayMessage(doc["msg"] | "", 3000, &display::ICON_HAPPY);
                    power::buzzerTriggered = true;
                }
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
            mqtt.setBufferSize(1024);

            return true;
        }
    } //

    bool isConnected()
    {
        return mqtt.connected();
    }

    void publish(const char *topic, const char *payload, bool retain)
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

            TickType_t now = xTaskGetTickCount();
            if ((now - lastLogTick) >= pdMS_TO_TICKS(MQTT_LOG_INTERVAL_MS))
            {
                lastLogTick = now;
                publishSystemStats();
            }

            if (xQueueReceive(mqttQueue, &lastMsg, 0) == pdTRUE)
            {
                mqtt.publish(lastMsg.topic, lastMsg.payload, lastMsg.retain);
            }

            mqtt.loop();
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }
}

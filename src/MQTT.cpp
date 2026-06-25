#include "MQTT.h"
#include "Globals.h"
#include "Network.h"
#include "Display.h"
#include "Power.h"
#include "Config.h"
#include "esp_timer.h"

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
        char lastStatus[16] = "";

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
            uint32_t averagesUs[TASK_METRIC_COUNT] = {};
            snapshotAndResetTaskAverages(averagesUs);

            doc["kind"] = "metrics";

            doc["idle"] = power::checkIdle();
            doc["heap_free"] = ESP.getFreeHeap();
            doc["heap_min"] = ESP.getMinFreeHeap();
            doc["heap_max"] = ESP.getMaxAllocHeap();
            doc["psram_free"] = psramFound() ? ESP.getFreePsram() : 0;
            doc["psram_min"] = psramFound() ? ESP.getMinFreePsram() : 0;
            doc["psram_max"] = psramFound() ? ESP.getMaxAllocPsram() : 0;
            doc["rssi"] = network::getRSSI();
            doc["post_max_ms"] = snapshotAndResetRestPostMax();

            JsonObject avg = doc["avg_us"].to<JsonObject>();
            avg["loop"] = averagesUs[static_cast<uint8_t>(TaskMetric::LOOP_TASK)];
            avg["mqtt"] = averagesUs[static_cast<uint8_t>(TaskMetric::MQTT_TASK)];
            avg["network"] = averagesUs[static_cast<uint8_t>(TaskMetric::NETWORK_TASK)];
            avg["camera"] = averagesUs[static_cast<uint8_t>(TaskMetric::CAMERA_TASK)];
            avg["display"] = averagesUs[static_cast<uint8_t>(TaskMetric::DISPLAY_TASK)];

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

        bool enqueue(const char *topic, const char *payload, bool retain)
        {
            MqttMsg msg{};
            strcpy(msg.topic, topic);
            strcpy(msg.payload, payload);
            msg.retain = retain;

            return xQueueSend(mqttQueue, &msg, 0) == pdPASS;
        }

        void drainQueue()
        {
            if (!isConnected())
            {
                return;
            }

            MqttMsg msg{};
            while (xQueueReceive(mqttQueue, &msg, 0) == pdTRUE)
            {
                mqtt.publish(msg.topic, msg.payload, msg.retain);
                mqtt.loop();
            }
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

            lastStatus[0] = '\0';
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

        enqueue(topic, payload, retain);
    }

    bool publishStatus(const char *status)
    {
        if (!isConnected())
        {
            return false;
        }

        if (strcmp(lastStatus, status) == 0)
        {
            return true;
        }

        char payload[64];
        snprintf(payload, sizeof(payload),
                 "{\"kind\":\"status\",\"status\":\"%s\"}",
                 status);

        if (!enqueue(topicLogs, payload, true))
        {
            return false;
        }

        strlcpy(lastStatus, status, sizeof(lastStatus));
        return true;
    }

    void TaskMqttCode(void *pvParameters)
    {
        changeTaskCount(1);

        configMqtt();

        const TickType_t delay = pdMS_TO_TICKS(99);
        const TickType_t metricsInterval = pdMS_TO_TICKS(MQTT_LOG_INTERVAL_MS);

        // wait boot and first connection
        while (checkState(SystemState::BOOTING) || checkState(SystemState::NET_OFF))
        {
            if (checkSleepEvent(delay))
            {
                break;
            }
        }

        TickType_t lastLogTick = 0;

        while (true)
        {
            if (checkSleepEvent(delay))
            {
                drainQueue();
                break;
            }

            int64_t cycleStart = esp_timer_get_time();

            if (checkState(SystemState::NET_OFF))
            {
                mqtt.loop();
                recordTaskRuntime(TaskMetric::MQTT_TASK, static_cast<uint32_t>(esp_timer_get_time() - cycleStart));
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
                recordTaskRuntime(TaskMetric::MQTT_TASK, static_cast<uint32_t>(esp_timer_get_time() - cycleStart));
                continue;
            }

            TickType_t now = xTaskGetTickCount();
            if ((now - lastLogTick) >= metricsInterval)
            {
                lastLogTick = now;
                publishSystemStats();
            }

            drainQueue();

            mqtt.loop();
            recordTaskRuntime(TaskMetric::MQTT_TASK, static_cast<uint32_t>(esp_timer_get_time() - cycleStart));
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }
}

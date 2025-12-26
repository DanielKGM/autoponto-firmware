#include "MQTT.h"
#include "Config.h"
#include "Globals.h"
#include "Network.h"
#include "Display.h"
#include "Power.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>

#define MQTT_TOPIC_SIZE 17 // deviceId + '/' + ('log' | 'cmd') + '/0'
#define MQTT_PAYLOAD_MAX 256

namespace mqtt
{
    namespace
    {
        volatile bool loggerFlag;
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
            if (!loggerFlag || !mqttQueue)
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
            char msg[DISPLAY_MSG_MAX_LEN];

            const TickType_t start = xTaskGetTickCount();
            const TickType_t timeout = pdMS_TO_TICKS(MQTT_TIMEOUT_MS);

            while (!mqtt.connected())
            {
                if ((WiFi.status() != WL_CONNECTED) || (xTaskGetTickCount() - start > timeout))
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
                power::setBuzzerTriggered(true);
            }

            if (doc["log"].is<bool>())
            {
                bool newValue = doc["log"];

                if (loggerFlag != newValue)
                {
                    setLogFlag(newValue);
                }
            }
        }

        bool initMqtt()
        {
            buildTopics();
            loadLogFlag();

            mqtt.setClient(wifiClient);
            mqtt.setServer(MQTT_URL, MQTT_PORT);
            mqtt.setCallback(mqttCallback);

            return true;
        }

        void loadLogFlag()
        {
            Preferences prefs;
            prefs.begin("mqtt", true);

            loggerFlag = prefs.getBool("log", MQTT_LOG_DEFAULT);

            prefs.end();
        }

        void setLogFlag(bool value)
        {
            Preferences prefs;
            prefs.begin("mqtt", false);

            prefs.putBool("log", value);

            prefs.end();

            loggerFlag = value;
        }
    } //

    bool publishStatus(const char *payload)
    {
        mqtt.publish(topicStatus, payload, true);
    }

    void TaskMqttCode(void *pvParameters)
    {
        changeTaskCount(1);

        initMqtt();

        const TickType_t delay = pdMS_TO_TICKS(100);
        char lastMsg[MQTT_PAYLOAD_MAX];
        TickType_t lastStatsTick = 0;
        const TickType_t logInterval = pdMS_TO_TICKS(MQTT_LOG_INTERVAL_MS);

        while (true)
        {
            bool isConnected = mqtt.connected();

            if (!isConnected && !(checkSystemState(SystemState::NET_OFF) || checkSystemState(SystemState::BOOTING)))
            {
                setSystemState(SystemState::MQTT_OFF);

                if (connMqtt())
                {
                    setSystemState(SystemState::WORKING);
                }
            }

            if (isConnected)
            {
                if (xQueueReceive(mqttQueue, lastMsg, 0) == pdTRUE)
                {
                    mqtt.publish(topicLogs, lastMsg, false);
                }

                if (loggerFlag && (xTaskGetTickCount() - lastStatsTick) > logInterval)
                {
                    lastStatsTick = xTaskGetTickCount();
                    publishSystemStats();
                }
            }

            mqtt.loop();

            if (checkSleepEvent(delay))
            {
                break;
            }
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }
}

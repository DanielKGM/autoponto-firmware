#include "MQTT.h"
#include "Globals.h"
#include "Network.h"
#include "Display.h"
#include "Power.h"

PubSubClient mqtt;
volatile bool loggerFlag;

char topicCmd[MQTT_TOPIC_SIZE];
char topicLogs[MQTT_TOPIC_SIZE];
char topicStatus[MQTT_TOPIC_SIZE];
QueueHandle_t mqttQueue = xQueueCreate(5, sizeof(MQTT_PAYLOAD_MAX));

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

bool connMqtt()
{
    char msg[DISPLAY_MSG_MAX_LEN];

    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(MQTT_TIMEOUT_MS);

    while (!mqtt.connected())
    {
        if ((WiFi.status() != WL_CONNECTED) || (xTaskGetTickCount() - start > timeout))
        {
            sendDisplayMessage("MQTT falhou!", 2000);
            return false;
        }

        sendDisplayMessage("Conectando ao Broker...");

        /*
        mqtt.connect(
            deviceId,
            MQTT_USER,
            MQTT_PASS,
            topicStatus,
            1,
            true,
            "OFFLINE",
            true);
        */

        mqtt.connect(deviceId);

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    mqtt.subscribe(topicCmd, 1);
    return true;
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

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    JsonDocument doc;

    if (deserializeJson(doc, payload, length))
    {
        return;
    }

    if (doc["auth"].is<bool>())
    {
        setBuzzerTriggered(doc["auth"]);
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

void TaskMqttCode(void *pvParameters)
{
    changeTaskCount(1);

    initMqtt();

    const TickType_t tickDelay = pdMS_TO_TICKS(100);

    char lastMsg[MQTT_PAYLOAD_MAX];

    while (systemState != SystemState::SLEEPING)
    {
        bool isConnected = mqtt.connected();

        if (!isConnected && (systemState == SystemState::NET_ON || systemState == SystemState::MQTT_OFF))
        {
            setSystemState(SystemState::MQTT_OFF);

            if (!connMqtt())
            {
                vTaskDelay(tickDelay);
                continue;
            }

            mqtt.publish(topicStatus, "READY", true);
            setSystemState(SystemState::READY);
        }

        mqtt.loop();

        if (isConnected && xQueueReceive(mqttQueue, lastMsg, 0) == pdTRUE)
        {
            mqtt.publish(topicLogs, lastMsg, false);
        }

        vTaskDelay(tickDelay);
    }

    mqtt.publish(topicStatus, "SLEEPING", true);

    changeTaskCount(-1);
    vTaskDelete(nullptr);
}

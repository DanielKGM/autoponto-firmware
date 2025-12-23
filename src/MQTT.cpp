#include "MQTT.h"
#include "Globals.h"
#include "Network.h"
#include "Display.h"

PubSubClient mqtt;
volatile bool loggerFlag;
char topicStatus[17];
char topicCmd[17];
char topicLogs[17];

void buildTopics()
{
    snprintf(topicStatus, sizeof(topicStatus),
             "%s/sts", deviceId);

    snprintf(topicCmd, sizeof(topicCmd),
             "%s/cmd", deviceId);

    snprintf(topicLogs, sizeof(topicLogs),
             "%s/log", deviceId);
}

bool connMqtt()
{
    char msg[DISPLAY_MSG_MAX_LEN];

    short int dots = 0;

    while (!mqtt.connected())
    {
        dots = (dots + 1) % 4;

        snprintf(msg, sizeof(msg),
                 "Conectando ao Broker%.*s",
                 dots, "...");

        sendDisplayMessage(msg);

        mqtt.connect(
            deviceId,
            MQTT_USER,
            MQTT_PASS,
            topicStatus,
            1,
            true,
            "OFFLINE",
            true);

        vTaskDelay(pdMS_TO_TICKS(CONN_WAIT_INTERVAL_MS));
    }

    return true;
}

bool initMqtt()
{
    buildTopics();
    loadLogFlag();

    mqtt.setClient(wifiClient);
    mqtt.setServer(MQTT_URL, MQTT_PORT);
    mqtt.setCallback(mqttCallback);

    return connMqtt();
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
    StaticJsonDocument<128> doc;

    if (deserializeJson(doc, payload, length))
    {
        return;
    }

    if (doc.containsKey("log"))
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

    mqtt.publish(topicStatus, "ONLINE", true);
    mqtt.subscribe(topicCmd, 1);

    const TickType_t tickDelay = pdMS_TO_TICKS(100);

    while (currentState != SystemState::SLEEPING)
    {
        if (!mqtt.connected())
        {
            setSystemState(SystemState::DISCONNECTED);
            if (connMqtt())
            {
                setSystemState(SystemState::READY);
                continue;
            }
        }

        mqtt.loop();
        vTaskDelay(tickDelay);
    }

    changeTaskCount(-1);
    vTaskDelete(nullptr);
}

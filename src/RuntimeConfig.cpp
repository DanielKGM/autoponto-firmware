#include "RuntimeConfig.h"
#include "Globals.h"
#include "Config.h"

// extern
RuntimeConfig runtimeConfig;

namespace configs
{
    namespace
    {
        constexpr const char *NS = "config";

        void copyStr(char *dst, size_t dstSize, const char *src)
        {
            strlcpy(dst, src ? src : "", dstSize);
        }

        void loadString(Preferences &prefs, const char *key, char *dst, size_t dstSize)
        {
            String value = prefs.getString(key, "");
            if (!value.isEmpty())
            {
                copyStr(dst, dstSize, value.c_str());
            }
        }
    }

    void setDefaults()
    {
        // Config.h -> runtimeConfig -> preferences

        copyStr(runtimeConfig.wifiSsid, sizeof(runtimeConfig.wifiSsid), WIFI_SSID);
        copyStr(runtimeConfig.wifiPass, sizeof(runtimeConfig.wifiPass), WIFI_PASS);

        copyStr(runtimeConfig.restUrl, sizeof(runtimeConfig.restUrl), REST_URL);
        copyStr(runtimeConfig.restPass, sizeof(runtimeConfig.restPass), REST_PASS);
        copyStr(runtimeConfig.restPostPath, sizeof(runtimeConfig.restPostPath), REST_POST_PATH);
        copyStr(runtimeConfig.restFetchPath, sizeof(runtimeConfig.restFetchPath), REST_FETCH_PATH);

        copyStr(runtimeConfig.mqttUrl, sizeof(runtimeConfig.mqttUrl), MQTT_URL);
        runtimeConfig.mqttPort = MQTT_PORT;
        copyStr(runtimeConfig.mqttUser, sizeof(runtimeConfig.mqttUser), MQTT_USER);
        copyStr(runtimeConfig.mqttPass, sizeof(runtimeConfig.mqttPass), MQTT_PASS);

        saveConfigs();
    }

    bool loadConfigs()
    {
        // preferences -> runtimeConfig (overrides defaults)

        Preferences prefs;
        if (!prefs.begin(NS, false))
        {
            return false;
        }

        loadString(prefs, "wifi_ssid", runtimeConfig.wifiSsid, sizeof(runtimeConfig.wifiSsid));
        loadString(prefs, "wifi_pass", runtimeConfig.wifiPass, sizeof(runtimeConfig.wifiPass));

        loadString(prefs, "rest_url", runtimeConfig.restUrl, sizeof(runtimeConfig.restUrl));
        loadString(prefs, "rest_pass", runtimeConfig.restPass, sizeof(runtimeConfig.restPass));
        loadString(prefs, "rest_post_p", runtimeConfig.restPostPath, sizeof(runtimeConfig.restPostPath));
        loadString(prefs, "rest_fetch_p", runtimeConfig.restFetchPath, sizeof(runtimeConfig.restFetchPath));

        loadString(prefs, "mqtt_url", runtimeConfig.mqttUrl, sizeof(runtimeConfig.mqttUrl));
        runtimeConfig.mqttPort = prefs.getUShort("mqtt_port", runtimeConfig.mqttPort);
        loadString(prefs, "mqtt_user", runtimeConfig.mqttUser, sizeof(runtimeConfig.mqttUser));
        loadString(prefs, "mqtt_pass", runtimeConfig.mqttPass, sizeof(runtimeConfig.mqttPass));

        prefs.end();
        return true;
    }

    bool saveConfigs()
    {
        // runtimeConfig -> preferences

        Preferences prefs;

        if (!prefs.begin(NS, false))
        {
            return false;
        }

        prefs.putString("wifi_ssid", runtimeConfig.wifiSsid);
        prefs.putString("wifi_pass", runtimeConfig.wifiPass);

        prefs.putString("rest_url", runtimeConfig.restUrl);
        prefs.putString("rest_pass", runtimeConfig.restPass);
        prefs.putString("rest_post_p", runtimeConfig.restPostPath);
        prefs.putString("rest_fetch_p", runtimeConfig.restFetchPath);
        prefs.putString("mqtt_url", runtimeConfig.mqttUrl);
        prefs.putUShort("mqtt_port", runtimeConfig.mqttPort);
        prefs.putString("mqtt_user", runtimeConfig.mqttUser);
        prefs.putString("mqtt_pass", runtimeConfig.mqttPass);

        prefs.end();
        return true;
    }

    bool importJson(const JsonVariantConst &json)
    {
        // json -> runtimeConfig (overrides current config)

        if (!json.is<JsonObjectConst>())
        {
            return false;
        }

        JsonObjectConst obj = json.as<JsonObjectConst>();

        if (obj["wifiSsid"].is<const char *>())
            copyStr(runtimeConfig.wifiSsid, sizeof(runtimeConfig.wifiSsid), obj["wifiSsid"]);
        if (obj["wifiPass"].is<const char *>())
            copyStr(runtimeConfig.wifiPass, sizeof(runtimeConfig.wifiPass), obj["wifiPass"]);
        if (obj["restUrl"].is<const char *>())
            copyStr(runtimeConfig.restUrl, sizeof(runtimeConfig.restUrl), obj["restUrl"]);
        if (obj["restPass"].is<const char *>())
            copyStr(runtimeConfig.restPass, sizeof(runtimeConfig.restPass), obj["restPass"]);
        if (obj["restPostPath"].is<const char *>())
            copyStr(runtimeConfig.restPostPath, sizeof(runtimeConfig.restPostPath), obj["restPostPath"]);
        if (obj["restFetchPath"].is<const char *>())
            copyStr(runtimeConfig.restFetchPath, sizeof(runtimeConfig.restFetchPath), obj["restFetchPath"]);

        if (obj["mqttUrl"].is<const char *>())
            copyStr(runtimeConfig.mqttUrl, sizeof(runtimeConfig.mqttUrl), obj["mqttUrl"]);
        if (obj["mqttPort"].is<uint16_t>())
            runtimeConfig.mqttPort = obj["mqttPort"];
        if (obj["mqttUser"].is<const char *>())
            copyStr(runtimeConfig.mqttUser, sizeof(runtimeConfig.mqttUser), obj["mqttUser"]);
        if (obj["mqttPass"].is<const char *>())
            copyStr(runtimeConfig.mqttPass, sizeof(runtimeConfig.mqttPass), obj["mqttPass"]);

        return true;
    }

    bool exportJson(JsonDocument &doc)
    {
        // runtimeConfig -> json

        JsonObject root = doc.to<JsonObject>();
        root["deviceId"] = deviceId;

        JsonObject config = root["config"].to<JsonObject>();
        config["wifiSsid"] = runtimeConfig.wifiSsid;

        config["restUrl"] = runtimeConfig.restUrl;
        config["restPostPath"] = runtimeConfig.restPostPath;
        config["restFetchPath"] = runtimeConfig.restFetchPath;

        config["mqttUrl"] = runtimeConfig.mqttUrl;
        config["mqttPort"] = runtimeConfig.mqttPort;
        config["mqttUser"] = runtimeConfig.mqttUser;

        return !doc.overflowed();
    }
}
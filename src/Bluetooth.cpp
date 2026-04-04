#include "Bluetooth.h"
#include "Config.h"
#include "Display.h"
#include "Globals.h"
#include "RuntimeConfig.h"

namespace bluetooth
{
    namespace
    {
        BluetoothSerial SerialBT;
        constexpr size_t RX_MAX = 512;

        void releasePin()
        {
            gpio_reset_pin((gpio_num_t)CONFIG_PIN);
        }

        void sendSimpleResponse(bool success, const char *message)
        {
            JsonDocument resp;
            resp["success"] = success;
            resp["message"] = message;
            serializeJson(resp, SerialBT);
            SerialBT.println();
        }

        void sendInfoResponse()
        {
            JsonDocument exported;
            if (!configs::exportJson(exported))
            {
                sendSimpleResponse(false, "falha ao exportar configuracoes");
                return;
            }

            JsonDocument resp;
            resp["success"] = true;
            resp["message"] = "configuracoes atuais";
            resp["data"] = exported.as<JsonVariantConst>();

            serializeJson(resp, SerialBT);
            SerialBT.println();
        }

        bool handleCommand(const JsonDocument &req)
        {
            if (strcmp(req["token"] | "", BLUETOOTH_TOKEN) != 0)
            {
                sendSimpleResponse(false, "token invalido");
                return false;
            }

            const char *command = req["command"] | "";

            if (command[0] == '\0')
            {
                sendSimpleResponse(false, "campo command ausente");
                return false;
            }

            if (strcmp(command, "info") == 0)
            {
                sendInfoResponse();
                return true;
            }

            if (strcmp(command, "update") == 0)
            {
                JsonVariantConst dados = req["dados"];

                if (dados.isNull())
                {
                    sendSimpleResponse(false, "campo dados ausente");
                    return false;
                }

                if (!configs::importJson(dados))
                {
                    sendSimpleResponse(false, "dados invalidos");
                    return false;
                }

                if (!configs::saveConfigs())
                {
                    sendSimpleResponse(false, "falha ao salvar configuracoes");
                    return false;
                }

                sendSimpleResponse(true, "atualizado com sucesso");
                return true;
            }

            if (strcmp(command, "exit") == 0)
            {
                sendSimpleResponse(true, "saindo do modo configuracao");
                return true;
            }

            sendSimpleResponse(false, "comando desconhecido");
            return false;
        }

        void showBluetoothStatus(bool connected)
        {
            if (connected)
            {
                display::sendDisplayMessage("Bluetooth conectado", 0, &display::ICON_SERVER);
            }
            else
            {
                display::sendDisplayMessage("Aguardando Bluetooth", 0, &display::ICON_SERVER);
            }
        }

        bool readBluetoothLine(String &line)
        {
            static String buffer;

            while (SerialBT.available() > 0)
            {
                char ch = static_cast<char>(SerialBT.read());

                if (ch == '\r')
                {
                    continue;
                }

                if (ch == '\n')
                {
                    line = buffer;
                    buffer = "";
                    return !line.isEmpty();
                }

                if (buffer.length() >= RX_MAX - 1)
                {
                    buffer = "";
                    sendSimpleResponse(false, "payload muito grande");
                    return false;
                }

                buffer += ch;
            }

            return false;
        }

        bool processBluetoothMessage(const String &line)
        {
            JsonDocument request;
            DeserializationError err = deserializeJson(request, line);

            if (err)
            {
                sendSimpleResponse(false, "json invalido");
                return false;
            }

            return handleCommand(request);
        }

    }

    bool shouldEnterConfigMode()
    {
        pinMode(CONFIG_PIN, INPUT_PULLUP);

        const TickType_t start = xTaskGetTickCount();
        bool pressed = false;

        while (xTaskGetTickCount() - start < pdMS_TO_TICKS(CONFIG_CHECK_MS))
        {
            if (digitalRead(CONFIG_PIN) == LOW)
            {
                vTaskDelay(pdMS_TO_TICKS(30)); // debounce simples
                if (digitalRead(CONFIG_PIN) == LOW)
                {
                    pressed = true;
                    break;
                }
            }

            vTaskDelay(pdMS_TO_TICKS(5));
        }

        releasePin();
        return pressed;
    }

    void runConfigMode()
    {
        setState(SystemState::CONFIGURING);

        char btName[40];
        snprintf(btName, sizeof(btName), "autoponto-%s", deviceId);

        if (!SerialBT.begin(btName))
        {
            display::sendDisplayMessage("Falha ao iniciar Bluetooth", 3000, &display::ICON_SAD);
            vTaskDelay(pdMS_TO_TICKS(3000));
            return;
        }

        TickType_t lastActivity = xTaskGetTickCount();
        const TickType_t timeout = pdMS_TO_TICKS(CONFIG_TIMEOUT_MS);

        bool wasConnected = false;
        String receivedLine;

        showBluetoothStatus(false);

        while (true)
        {
            bool connected = SerialBT.hasClient();

            if (connected != wasConnected)
            {
                wasConnected = connected;
                lastActivity = xTaskGetTickCount();
                showBluetoothStatus(connected);
            }

            if (readBluetoothLine(receivedLine))
            {
                lastActivity = xTaskGetTickCount();

                bool shouldExit = processBluetoothMessage(receivedLine);

                if (shouldExit)
                {
                    break;
                }
            }

            if ((xTaskGetTickCount() - lastActivity) > timeout)
            {
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }

        SerialBT.flush();
        vTaskDelay(pdMS_TO_TICKS(150));
        SerialBT.end();
    }

}
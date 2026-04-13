#include "Bluetooth.h"
#include "Config.h"
#include "Display.h"
#include "Globals.h"
#include "RuntimeConfig.h"

namespace bluetooth
{
    namespace
    {
        constexpr const char *SERVICE_UUID = "9d8f6b10-3d8d-4d1b-9c7e-5e6a3c8b1001";
        constexpr const char *RX_UUID = "9d8f6b10-3d8d-4d1b-9c7e-5e6a3c8b1002";
        constexpr const char *TX_UUID = "9d8f6b10-3d8d-4d1b-9c7e-5e6a3c8b1003";

        constexpr size_t RX_MAX = 512;

        NimBLEServer *gServer = nullptr;
        NimBLEService *gService = nullptr;
        NimBLECharacteristic *gRxChar = nullptr;
        NimBLECharacteristic *gTxChar = nullptr;
        NimBLEAdvertising *gAdvertising = nullptr;

        volatile bool gClientConnected = false;
        volatile bool gShouldExit = false;
        volatile bool gHasPendingRequest = false;

        portMUX_TYPE gBleMux = portMUX_INITIALIZER_UNLOCKED;

        char gPendingPayload[RX_MAX] = {0};

        void setPendingPayload(const char *payload)
        {
            portENTER_CRITICAL(&gBleMux);
            strlcpy(gPendingPayload, payload ? payload : "", sizeof(gPendingPayload));
            gHasPendingRequest = true;
            portEXIT_CRITICAL(&gBleMux);
        }

        bool takePendingPayload(char *dst, size_t dstSize)
        {
            bool hasData = false;

            portENTER_CRITICAL(&gBleMux);
            if (gHasPendingRequest)
            {
                strlcpy(dst, gPendingPayload, dstSize);
                gPendingPayload[0] = '\0';
                gHasPendingRequest = false;
                hasData = true;
            }
            portEXIT_CRITICAL(&gBleMux);

            return hasData;
        }

        void notifyResponse(const char *json)
        {
            if (!gTxChar || !json)
                return;

            gTxChar->setValue(json);

            if (gClientConnected)
            {
                gTxChar->notify();
            }
        }

        void sendSimpleResponse(bool success, const char *message)
        {
            JsonDocument resp;
            resp["success"] = success;
            resp["message"] = message ? message : "";

            char out[RX_MAX];
            size_t written = serializeJson(resp, out, sizeof(out));
            if (written >= sizeof(out))
            {
                const char *fallback = R"({"success":false,"message":"resposta grande demais"})";
                notifyResponse(fallback);
                return;
            }

            notifyResponse(out);
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

            char out[RX_MAX];
            size_t written = serializeJson(resp, out, sizeof(out));
            if (written >= sizeof(out))
            {
                sendSimpleResponse(false, "resposta grande demais");
                return;
            }

            notifyResponse(out);
        }

        bool handleCommand(const JsonDocument &req)
        {
            if (strcmp(req["token"] | "", BLUETOOTH_TOKEN) != 0)
            {
                std::string msg = std::string("token invalido: ") + (req["token"] | "");
                sendSimpleResponse(false, msg.c_str());
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
                return false;
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
                return false;
            }

            if (strcmp(command, "exit") == 0)
            {
                sendSimpleResponse(true, "saindo do modo configuracao");
                return true;
            }

            sendSimpleResponse(false, "comando desconhecido");
            return false;
        }

        bool processRequestJson(const char *payload)
        {
            if (!payload || payload[0] == '\0')
            {
                sendSimpleResponse(false, "payload vazio");
                return false;
            }

            JsonDocument request;
            DeserializationError err = deserializeJson(request, payload);

            if (err)
            {
                sendSimpleResponse(false, "json invalido");
                return false;
            }

            return handleCommand(request);
        }

        void showBluetoothStatus(bool connected)
        {
            if (connected)
            {
                display::sendDisplayMessage("Bluetooth conectado!", 0, &display::ICON_HAPPY);
            }
            else
            {
                display::sendDisplayMessage("Aguardando Bluetooth...", 0, &display::ICON_BLUETOOTH);
            }
        }

        class ServerCallbacks : public NimBLEServerCallbacks
        {
        public:
            void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
            {
                (void)pServer;
                (void)connInfo;
                gClientConnected = true;
            }

            void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
            {
                (void)connInfo;
                (void)reason;
                gClientConnected = false;

                if (pServer)
                {
                    pServer->startAdvertising();
                }
            }
        };

        class RxCallbacks : public NimBLECharacteristicCallbacks
        {
        public:
            void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
            {
                (void)connInfo;

                if (!pCharacteristic)
                    return;

                std::string value = pCharacteristic->getValue();

                if (value.empty())
                {
                    sendSimpleResponse(false, "payload vazio");
                    return;
                }

                if (value.size() >= RX_MAX)
                {
                    sendSimpleResponse(false, "payload muito grande");
                    return;
                }

                char local[RX_MAX];
                memcpy(local, value.data(), value.size());
                local[value.size()] = '\0';

                while (value.size() > 0 &&
                       (local[strlen(local) - 1] == '\n' || local[strlen(local) - 1] == '\r'))
                {
                    local[strlen(local) - 1] = '\0';
                }

                setPendingPayload(local);
            }
        };

        bool startBleServer(const char *deviceName)
        {
            NimBLEDevice::init(deviceName);

            NimBLEDevice::setPower(9);
            NimBLEDevice::setMTU(256);

            gAdvertising = NimBLEDevice::getAdvertising();

            NimBLEAdvertisementData advData;
            advData.setName(deviceName);
            advData.addServiceUUID(SERVICE_UUID);

            gAdvertising->setAdvertisementData(advData);

            NimBLEAdvertisementData scanData;
            scanData.setName(deviceName);
            gAdvertising->setScanResponseData(scanData);

            gAdvertising->start();

            gServer = NimBLEDevice::createServer();
            if (!gServer)
                return false;

            gServer->setCallbacks(new ServerCallbacks());

            gService = gServer->createService(SERVICE_UUID);
            if (!gService)
                return false;

            gTxChar = gService->createCharacteristic(
                TX_UUID,
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

            gRxChar = gService->createCharacteristic(
                RX_UUID,
                NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);

            if (!gTxChar || !gRxChar)
                return false;

            gRxChar->setCallbacks(new RxCallbacks());
            gTxChar->setValue(R"({"success":true,"message":"ble pronto"})");

            gServer->start();

            gAdvertising = NimBLEDevice::getAdvertising();
            if (!gAdvertising)
                return false;

            gAdvertising->addServiceUUID(SERVICE_UUID);

            gAdvertising->enableScanResponse(true);

            return gAdvertising->start();
        }

        void stopBleServer()
        {
            if (gAdvertising)
            {
                gAdvertising->stop();
            }

            NimBLEDevice::deinit(true);

            gServer = nullptr;
            gService = nullptr;
            gRxChar = nullptr;
            gTxChar = nullptr;
            gAdvertising = nullptr;

            gClientConnected = false;
            gShouldExit = false;
            gHasPendingRequest = false;
            gPendingPayload[0] = '\0';
        }
    }

    void runConfigMode()
    {
        setState(SystemState::CONFIGURING);

        char bleName[40];
        snprintf(bleName, sizeof(bleName), "autoponto-%s", deviceId);

        if (!startBleServer(bleName))
        {
            display::sendDisplayMessage("Falha ao iniciar BLE", 3000, &display::ICON_SAD);
            vTaskDelay(pdMS_TO_TICKS(3000));
            stopBleServer();
            return;
        }

        TickType_t lastActivity = xTaskGetTickCount();
        const TickType_t timeout = pdMS_TO_TICKS(CONFIG_TIMEOUT_MS);

        bool lastConnected = false;
        char request[RX_MAX];

        showBluetoothStatus(false);

        while (true)
        {
            bool connected = gClientConnected;

            if (connected != lastConnected)
            {
                lastConnected = connected;
                lastActivity = xTaskGetTickCount();
                showBluetoothStatus(connected);
            }

            if (takePendingPayload(request, sizeof(request)))
            {
                lastActivity = xTaskGetTickCount();

                bool shouldExit = processRequestJson(request);
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

        stopBleServer();
    }
}

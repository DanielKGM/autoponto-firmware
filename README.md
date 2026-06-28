# AutoPonto Firmware

Firmware PlatformIO/Arduino para um dispositivo de borda do AutoPonto baseado em
ESP32-CAM. O dispositivo captura frames JPEG, envia imagens para um servidor
REST, recebe respostas operacionais por MQTT, mostra feedback em um display TFT
redondo e usa um sensor PIR para alternar entre uso ativo, idle e deep sleep.

O objetivo do firmware e manter um ponto de chamada simples na borda:

- identificar o dispositivo pelo MAC eFuse do ESP32;
- conectar ao WiFi e ao broker MQTT configurados em build time;
- buscar no backend o contexto de aula/chamada atual;
- exibir preview da camera ou mensagens de estado no TFT;
- enviar frames JPEG periodicamente enquanto existe chamada ativa;
- receber feedback de autenticacao pelo MQTT;
- publicar status, metricas de saude e eventos de presenca;
- economizar energia quando nao ha movimento detectado pelo PIR.

## Plataforma

- Board: `esp32cam`
- Framework: Arduino sobre ESP32 via PlatformIO
- Ambiente: `env:esp32cam`
- Display: TFT GC9A01 240x240 via `TFT_eSPI`
- Camera: ESP32-CAM com JPEG 240x240 em PSRAM
- Comunicacao: WiFi, HTTP REST e MQTT
- Sensor de presenca: PIR com wake-up de deep sleep
- Feedback local: display, icones, buzzer/LED positivo

Dependencias principais em `platformio.ini`:

- `bodmer/TFT_eSPI`
- `bitbank2/JPEGDEC`
- `bblanchon/ArduinoJson`
- `knolleary/PubSubClient`

## Configuracao local

O codigo inclui `Config.h`, mas esse arquivo nao deve ser versionado. Ele fica
em `include/Config.h` e esta ignorado pelo Git porque contem credenciais,
enderecos e parametros de ambiente.

Para preparar um ambiente local:

```powershell
Copy-Item Config_Model.h include\Config.h
```

Depois edite `include/Config.h` com os valores reais:

- `WIFI_SSID` e `WIFI_PASS`
- `REST_URL`, `REST_PASS`, `REST_POST_PATH`, `REST_FETCH_PATH`
- `MQTT_URL`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASS`

`Config_Model.h` tambem documenta os parametros fixos de build, como pinos,
timeouts, intervalo de POST REST, intervalo de metricas MQTT, tamanho do display
e duracao do feedback positivo. Nao coloque credenciais reais no README nem em
arquivos versionados.

Nao ha fluxo ativo de configuracao por Bluetooth ou configuracao em runtime: a
configuracao atual vem de constantes de build em `include/Config.h`.

## Fluxo de execucao

`src/main.cpp` e o ponto de entrada.

1. Desativa brownout, configura pinos e marca o estado `BOOTING`.
2. Gera `deviceId` a partir do MAC eFuse do ESP32.
3. Inicializa TFT, sprites de desenho, sleep e camera.
4. Cria tarefas FreeRTOS para display, camera, rede e MQTT.
5. No `loop()`, expira contexto de aula, processa PIR, aciona feedback positivo,
   entra/sai de idle e dispara deep sleep quando aplicavel.

As tarefas principais rodam em paralelo:

- `TaskDisplay`: desenha mensagens, preview da camera e informacoes de contexto.
- `TaskCamera`: captura frames da camera, alimenta preview e snapshots REST.
- `TaskNetwork`: conecta WiFi, busca contexto REST e envia frames JPEG.
- `TaskMqtt`: conecta ao broker, assina comandos e publica status/metricas.

## Estados

Os estados ficam em `include/Globals.h`:

- `BOOTING`: inicializacao local.
- `NET_OFF`: WiFi desconectado.
- `MQTT_OFF`: WiFi ativo, MQTT desconectado.
- `FETCHING`: precisa buscar contexto de aula no REST.
- `WORKING`: contexto valido e operacao normal.
- `WAITING_SERVER`: frame enviado, aguardando retorno pelo MQTT ou timeout.
- `SLEEPING`: preparando deep sleep.

`setState()` em `src/Globals.cpp` tambem publica status MQTT retido quando o
broker esta conectado:

- `working`
- `fetching`
- `sleep`

O diagrama em `docs/FSM_DIAGRAM.mmd` e a imagem `docs/FSM_DIAGRAM.png` ajudam a
visualizar a FSM. O codigo atual nao possui fluxo de configuracao Bluetooth; se
essa etapa aparecer no diagrama, trate como referencia historica a revisar. Se o
fluxo de estados mudar, atualize esses dois artefatos.

## Contexto de aula

O contexto compartilhado e `AttendanceContext`:

```cpp
struct AttendanceContext
{
    char lesson_name[LESSON_NAME_LENGTH];
    uint64_t msForNext;
    uint64_t msRemaining;
    TickType_t fetchTick;
};
```

Campos esperados do endpoint REST de contexto:

- `lesson_name`: nome da turma/aula.
- `msForNext`: tempo ate a proxima chamada.
- `msRemaining`: tempo restante da chamada atual.

`fetchTick` e o instante local em que o contexto foi recebido. O display,
expiracao no `loop()` e telemetria devem usar esse mesmo ponto de referencia
para evitar divergencia entre UI, validade do contexto e metricas.

Quando `msForNext == 0` e `msRemaining == 0`, o firmware trata como ausencia de
turma proxima, limpa o contexto e segue tentando buscar novamente em intervalo
controlado.

## Contrato REST

`src/Network.cpp` concentra o trafego HTTP.

### Buscar contexto

Requisicao:

- metodo: `GET`
- URL: `REST_URL + REST_FETCH_PATH`
- headers:
  - `X-Device-Id: <deviceId>`
  - `X-Auth: <REST_PASS>`

Resposta esperada:

```json
{
  "lesson_name": "Nome da turma",
  "msForNext": 0,
  "msRemaining": 300000
}
```

### Enviar frame

Requisicao:

- metodo: `POST`
- URL: `REST_URL + REST_POST_PATH`
- body: JPEG cru capturado pela camera
- headers:
  - `Content-Type: image/jpeg`
  - `X-Device-Id: <deviceId>`
  - `X-Auth: <REST_PASS>`
  - `Connection: close`

O envio so acontece quando:

- o dispositivo esta em `WORKING`;
- ha chamada ativa (`context.msRemaining > 0`);
- o dispositivo nao esta em idle;
- nao ha mensagem fullscreen ativa no display.

Depois do POST bem-sucedido, o estado passa para `WAITING_SERVER`. Se nao chegar
feedback MQTT dentro de `RESPONSE_WAIT_TIMEOUT_MS`, volta para `WORKING`.

## Contrato MQTT

`src/MQTT.cpp` concentra topicos, conexao e payloads.

O `deviceId` tem 12 caracteres hexadecimais derivados do MAC eFuse. A partir
dele, os topicos sao:

- comandos: `cmd/{deviceId}`
- logs/status/telemetria: `log/{deviceId}`

### Status

O firmware publica status retido em `log/{deviceId}`:

```json
{"kind":"status","status":"working"}
```

O Last Will and Testament tambem usa `log/{deviceId}` com payload retido:

```json
{"kind":"status","status":"offline"}
```

### Metricas

A cada `MQTT_LOG_INTERVAL_MS`, publica `kind=metrics` sem retain:

```json
{
  "kind": "metrics",
  "idle": false,
  "heap_free": 0,
  "heap_min": 0,
  "heap_max": 0,
  "psram_free": 0,
  "psram_min": 0,
  "psram_max": 0,
  "rssi": 0,
  "post_max_ms": 0,
  "avg_us": {
    "loop": 0,
    "mqtt": 0,
    "network": 0,
    "camera": 0,
    "display": 0
  },
  "avg_count": {
    "loop": 0,
    "mqtt": 0,
    "network": 0,
    "camera": 0,
    "display": 0
  }
}
```

### PIR

Quando o PIR detecta movimento:

```json
{"kind":"pir","presenca":true}
```

Esse evento tambem tira o dispositivo do idle.

### Comandos

O firmware assina `cmd/{deviceId}` e aceita JSON.

Resposta de autenticacao durante `WAITING_SERVER`:

```json
{
  "auth": true,
  "msg": "Presenca registrada"
}
```

Quando `auth` e verdadeiro e o dispositivo nao esta em idle, ele mostra a
mensagem no display e aciona o feedback positivo.

Forcar nova busca de contexto:

```json
{
  "fetch": true
}
```

Esse comando chama `clearContext()` e retorna o firmware para `FETCHING`.

## Display

`src/Display.cpp` usa `TFT_eSPI`, sprites off-screen e `JPEGDEC`.

Responsabilidades:

- mostrar mensagens fullscreen com icone e spinner opcional;
- renderizar preview da camera;
- sobrepor texto de contexto no preview durante chamada ativa;
- mostrar tela de contexto quando ainda falta tempo para a proxima chamada;
- limpar fila e apagar tela ao entrar em idle;
- sinalizar `fullscreenMessageActive` para impedir POST REST durante mensagens.

Textos de contexto alternam entre nome da turma e contagem regressiva:

- chamada ativa: `Chamada: <turma>` / `Restam <tempo>`
- proxima chamada: `Proxima turma: <turma>` / `Chamada em <tempo>`

## Camera

`src/Camera.cpp` configura a camera com os pinos de `include/Config_Camera.h`.

Ela captura JPEG 240x240 em PSRAM e mantem duas filas:

- `previewQueue`: frames para o display;
- `snapshotQueue`: frames solicitados pela rede para POST REST.

Os frames sao clonados para memoria PSRAM e liberados com `releaseFrame()`.

## Energia, PIR e feedback

`src/Power.cpp` cuida de pinos, PIR, idle, feedback positivo e deep sleep.

- `PIR_PIN` gera interrupcao e wake-up de deep sleep.
- `IDLE_TIMEOUT_MS` coloca o display em off e ativa economia de WiFi.
- `SLEEP_TIMEOUT_MS` dispara deep sleep quando ja esta idle e sem movimento.
- `POSITIVE_FB_PIN` liga buzzer/LED por `POSITIVE_FB_DURATION_MS`.

Idle e sleep sao coordenados pelo `loop()` e pelo event group `systemEvents`.
Antes de dormir, as tarefas encerram seus loops quando recebem `EVT_SLEEP`.

## Mapa de arquivos

```text
.
|-- platformio.ini              Configuracao PlatformIO, board, libs e build flags TFT
|-- Config_Model.h              Modelo de configuracao local sem segredos
|-- README.md                   Este guia
|-- docs/
|   |-- FSM_DIAGRAM.mmd         Diagrama Mermaid da maquina de estados
|   `-- FSM_DIAGRAM.png         Render do diagrama
|-- include/
|   |-- Camera.h                API da camera e FrameBuffer
|   |-- Config.h                Config local ignorada pelo Git
|   |-- Config_Camera.h         Pinos e parametros da camera
|   |-- Display.h               API do display e icones
|   |-- Font.h                  Fonte customizada usada no TFT
|   |-- Globals.h               Estados, contexto, tasks e metricas compartilhadas
|   |-- Icons.h                 Declaracoes dos icones RGB565
|   |-- MQTT.h                  API MQTT, topicos e limites de payload
|   |-- Network.h               API WiFi/REST
|   `-- Power.h                 API de energia, PIR, idle e feedback
`-- src/
    |-- Camera.cpp              Inicializacao/captura da camera e filas de frame
    |-- Display.cpp             Renderizacao TFT, mensagens e preview
    |-- Globals.cpp             Estado global, contexto, metricas e helpers
    |-- Icons.cpp               Bitmaps RGB565 dos icones
    |-- MQTT.cpp                Conexao MQTT, comandos, status e metricas
    |-- Network.cpp             WiFi, fetch de contexto e POST de frames
    |-- Power.cpp               Pinos, PIR, idle, feedback e deep sleep
    `-- main.cpp                Bootstrap, criacao das tasks e loop principal
```

## Build, upload e monitor

No Windows deste ambiente, o executavel absoluto do PlatformIO costuma ser o
caminho mais confiavel:

```powershell
C:\Users\danie\.platformio\penv\Scripts\pio.exe run
```

Comandos usuais:

```powershell
# Compilar
C:\Users\danie\.platformio\penv\Scripts\pio.exe run

# Enviar para a placa
C:\Users\danie\.platformio\penv\Scripts\pio.exe run -t upload

# Monitor serial
C:\Users\danie\.platformio\penv\Scripts\pio.exe device monitor
```

`platformio.ini` define `monitor_port = COM5` e `monitor_speed = 115200`.

## Cuidados ao alterar

- Preserve `include/Config.h` como arquivo local ignorado.
- Ao mexer em tempo de contexto, mantenha `fetchTick`, `msForNext` e
  `msRemaining` coerentes entre `Network.cpp`, `Display.cpp`, `main.cpp` e
  `MQTT.cpp`.
- Ao mudar topicos/payloads MQTT, confira `buildTopics()`, `publishStatus()`,
  `publishSystemStats()` e o publish PIR no `loop()`.
- Ao mudar display, evite desenhar diretamente em multiplos passos no TFT; o
  padrao atual compoe em sprite e empurra o frame pronto.
- Ao remover features, rode um `rg` pelos identificadores antigos para evitar
  sobras.
- Depois de alteracoes de firmware, valide pelo menos com `pio run`.

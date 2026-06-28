# Sugestao objetiva para atualizar o README

Esta secao pode entrar no README como um resumo voltado ao TCC, sem substituir
os detalhes operacionais ja existentes.

## Evidencias para o TCC

### Papel do firmware na arquitetura do AutoPonto

O firmware e o componente de borda do AutoPonto executado no ESP32-CAM. Ele
identifica o dispositivo pelo MAC eFuse, conecta ao WiFi, sincroniza o contexto
da chamada com o backend REST, captura frames JPEG para autenticacao, recebe
feedback operacional por MQTT, exibe o estado local no display TFT e usa PIR,
idle e deep sleep para reduzir consumo quando nao ha presenca.

### Tarefas FreeRTOS

- `TaskDisplay`: atualiza display TFT, mensagens fullscreen, preview da camera
  e textos de contexto.
- `TaskCamera`: captura frames JPEG, alimenta a fila de preview e entrega
  snapshots sob demanda para envio REST.
- `TaskNetwork`: gerencia WiFi, busca contexto por HTTP GET e envia frames por
  HTTP POST.
- `TaskMqtt`: conecta ao broker, assina comandos, publica status, metricas e
  eventos PIR.
- `loop`: coordena expiracao do contexto, interrupcao PIR, idle, deep sleep e
  feedback positivo.

### Estados do firmware

- `BOOTING`: inicializacao de pinos, display, camera e tarefas.
- `NET_OFF`: WiFi indisponivel ou em reconexao.
- `MQTT_OFF`: WiFi conectado, mas broker MQTT indisponivel.
- `FETCHING`: firmware precisa buscar contexto de aula no backend REST.
- `WORKING`: contexto valido e operacao normal.
- `WAITING_SERVER`: frame enviado por HTTP, aguardando resposta MQTT ou timeout.
- `SLEEPING`: tarefas estao sendo encerradas antes do deep sleep.

### Fluxo HTTP/MQTT

No HTTP, o firmware busca o contexto em `REST_FETCH_PATH` usando `X-Device-Id`
e `X-Auth`. Quando ha chamada ativa, nao esta em idle e nao existe mensagem
fullscreen, a rede solicita um snapshot da camera e envia o JPEG cru para
`REST_POST_PATH`. Depois de um POST bem-sucedido, o estado passa para
`WAITING_SERVER`.

No MQTT, o firmware publica tudo em `log/{deviceId}` e assina comandos em
`cmd/{deviceId}`. O topico de log recebe status retido, LWT `offline`, metricas
periodicas e evento PIR. O topico de comando aceita retorno de autenticacao
(`auth` e `msg`) e solicitacao de nova sincronizacao (`fetch`).

### Metricas disponiveis

As metricas MQTT incluem estado de idle, heap livre/minimo/maximo, PSRAM
livre/minima/maxima, RSSI, maior tempo de POST REST no periodo (`post_max_ms`),
media de tempo de execucao por tarefa (`avg_us`) e quantidade de amostras por
tarefa (`avg_count`). O evento PIR e enviado como `{"kind":"pir","presenca":true}`.

### Limitacoes conhecidas

- A configuracao e fixa em `include/Config.h`; nao ha configuracao Bluetooth ou
  configuracao em runtime no firmware atual.
- O firmware depende de WiFi, backend REST e broker MQTT disponiveis; falhas
  geram retries e mensagens locais, mas nao ha fila persistente offline.
- O envio de imagem usa JPEG cru e depende de PSRAM para clonar frames.
- O feedback de autenticacao chega por MQTT depois do POST HTTP; se ele nao
  chegar dentro do timeout, o firmware volta para `WORKING`.
- O deep sleep reinicia o ESP32 no wake por PIR, portanto o fluxo volta para a
  inicializacao normal.

### Diagramas Mermaid

- `docs/FSM_DIAGRAM.mmd`: maquina de estados atual.
- `docs/TCC_HTTP_CAPTURE_FLOW.mmd`: busca de contexto, captura e envio HTTP.
- `docs/TCC_MQTT_FLOW.mmd`: topicos, status, metricas, PIR e comandos.
- `docs/TCC_FREERTOS_TASKS.mmd`: divisao das tarefas FreeRTOS e recursos
  compartilhados.

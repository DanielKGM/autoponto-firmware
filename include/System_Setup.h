// =======================================================
//  CONFIGURAÇÕES DO SISTEMA
// =======================================================


// =======================================================
//  PINOS DO PROTÓTIPO
// =======================================================
// Sensor de presença (PIR)
#define PIR               33

// Buzzer sonoro
#define POSITIVE_FEEDBACK            3

// Base do Transistor NPN de habilitação do Display (liga/desliga)
#define DISPLAY_ENABLE    1


// =======================================================
//  CONFIGURAÇÕES DE TEMPO (milissegundos)
// =======================================================
// Intervalo mínimo entre ativações do buzzer
#define BUZZER_INTERVAL_MS 10000   // 10 segundos

// Tempo que o buzzer permanece ligado
#define BUZZER_ON_TIME_MS  2000    // 2 segundos

// Tempo para desligar o display por inatividade
#define DISPLAY_TIMEOUT_MS 15000   // 15 segundos

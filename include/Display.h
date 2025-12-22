#pragma once
#include "Arduino.h"
#include "esp_camera.h"
#include <JPEGDEC.h>
#include <TFT_eSPI.h>

struct DisplayMessage
{
    char text[DISPLAY_MSG_MAX_LEN];
    TickType_t duration;
};

extern QueueHandle_t messageQueue;
extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern JPEGDEC decoder;

void initTFT();
void initSprite();

int drawMCUs(JPEGDRAW *pDraw);

void showText(const char *text, bool pushToDisplay = true);
bool showCamFrame(bool push_sprite = false);

bool sendDisplayMessage(const char *text, unsigned long duration_ms = 0);
void TaskDisplayCode(void *pvParameters);

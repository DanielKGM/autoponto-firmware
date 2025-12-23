#pragma once
#include "esp_camera.h"
#include <JPEGDEC.h>
#include <TFT_eSPI.h>

using FrameBuffer = struct
{
    uint8_t *data;
    size_t len;
};

extern QueueHandle_t messageQueue;
extern QueueHandle_t frameQueue;

extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern JPEGDEC decoder;

void initTFT();
void initSprite();

int drawMCUs(JPEGDRAW *pDraw);

void showText(const char *text, bool pushToDisplay = true);
bool showCamFrame(bool pushSprite = false);

bool sendDisplayMessage(const char *text, unsigned long durationMs = 0);
void TaskDisplayCode(void *pvParameters);

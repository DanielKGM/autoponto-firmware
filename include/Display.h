#pragma once
#include "esp_camera.h"
#include <JPEGDEC.h>
#include <TFT_eSPI.h>

struct FrameBuffer
{
    uint8_t *data;
    size_t len;
};

struct Icon
{
    const uint16_t *data;
    uint16_t width;
    uint16_t height;
};

extern const Icon ICON_WIFI;
extern const Icon ICON_SAD;
extern const Icon ICON_HAPPY;
extern const Icon ICON_SERVER;

extern QueueHandle_t messageQueue;
extern QueueHandle_t frameQueue;

extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern JPEGDEC decoder;

void initTFT();
void initSprite();

int drawMCUs(JPEGDRAW *pDraw);

void showText(const char *text, const Icon *icon = nullptr);
bool showCamFrame(bool captureFrame = false);

bool sendDisplayMessage(const char *text, unsigned long durationMs = 0, const Icon *icon = nullptr);

void TaskDisplayCode(void *pvParameters);

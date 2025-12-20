#pragma once
#include "Arduino.h"
#include <TFT_eSPI.h>
#include "esp_camera.h"

extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern uint16_t *sprite_buf;

void initTFT();
void initSprite();

void showText(const char *text, bool pushToDisplay = true);
bool showCamFrame(bool push_sprite = false);

bool sendDisplayMessage(const char *text, unsigned long duration_ms = 0);
void TaskDisplayCode(void *pvParameters);

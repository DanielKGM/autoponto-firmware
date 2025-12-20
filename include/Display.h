#pragma once
#include "Arduino.h"
#include <TFT_eSPI.h>
#include "esp_camera.h"

TFT_eSPI tft;
TFT_eSprite spr;
uint16_t *sprite_buf;

void setupTFT();
void setupSprite();

void showText(const char *text, bool pushToDisplay = true);
bool showCamFrame(bool push_sprite = false);

bool sendDisplayMessage(const char *text, unsigned long duration_ms = 0);
void TaskDisplayCode(void *pvParameters);

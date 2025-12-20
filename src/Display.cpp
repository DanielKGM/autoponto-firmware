#include "Display.h"
#include "Globals.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
uint16_t *sprite_buf;

void initTFT()
{
    tft.init();
    tft.setRotation(0);
    tft.setTextDatum(MC_DATUM);
    tft.fillScreen(TFT_BLACK);
}

void initSprite()
{
    sprite_buf = (uint16_t *)spr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    spr.fillScreen(TFT_BLACK);
    spr.setRotation(0);
    spr.setSwapBytes(true);
    spr.setTextColor(TFT_WHITE, TFT_BLACK); // Cor do texto: branco, fundo preto
    spr.setTextDatum(MC_DATUM);             // Centraliza o texto
    spr.setTextFont(2);                     // Define a fonte do texto
    spr.setTextSize(1);                     // Define o tamanho do texto
    spr.setTextWrap(true, true);            // Quebra de linha horizontal
}

void showText(const char *text, bool pushToDisplay)
{
    spr.fillScreen(TFT_BLACK);

    spr.drawString(text, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);

    if (pushToDisplay)
    {
        spr.pushSprite(0, 0);
    }
}

bool showCamFrame(bool push_sprite)
{
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
        return false;
    }

    memcpy(sprite_buf, fb->buf, DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);

    esp_camera_fb_return(fb);

    if (push_sprite)
    {
        spr.pushSprite(0, 0);
    }

    return true;
}

bool sendDisplayMessage(const char *text, unsigned long duration_ms)
{
    if (should_sleep_flag || !messageQueue)
    {
        return false;
    }

    DisplayMessage msg{};
    strncpy(msg.text, text, DISPLAY_MSG_MAX_LEN - 1);
    msg.duration = pdMS_TO_TICKS(duration_ms); // 0 = persistente

    return xQueueSendToBack(messageQueue, &msg, 0) == pdPASS;
}

void TaskDisplayCode(void *pvParameters)
{
    changeAliveTaskCount(1);

    DisplayMessage msg{};
    bool overlayActive = false;
    TickType_t overlayUntil = 0;

    const TickType_t tickDelay = pdMS_TO_TICKS(50);

    while (!should_sleep_flag)
    {
        if ((overlayUntil == 0) && xQueueReceive(messageQueue, &msg, 0) == pdPASS)
        {
            overlayActive = true;
            showText(msg.text, true);

            if (msg.duration > 0)
            {
                overlayUntil = xTaskGetTickCount() + msg.duration;
            }
            else
            {
                overlayUntil = 0;
            }
        }

        if (overlayActive && msg.duration > 0 && xTaskGetTickCount() >= overlayUntil)
        {
            overlayUntil = 0;
            overlayActive = false;
        }

        if (!overlayActive)
        {
            showCamFrame(true);
        }

        vTaskDelay(tickDelay);
    }

    changeAliveTaskCount(-1);
    vTaskDelete(nullptr);
}
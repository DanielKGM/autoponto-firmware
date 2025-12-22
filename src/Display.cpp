#include "Display.h"
#include "Globals.h"

JPEGDEC decoder;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

int drawMCUs(JPEGDRAW *pDraw)
{
    spr.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
    return 1;
}

void initTFT()
{
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
}

void initSprite()
{
    spr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    spr.fillScreen(TFT_BLACK);
    spr.setRotation(0);
    spr.setSwapBytes(false);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.setTextDatum(MC_DATUM);
    spr.setTextFont(2);
    spr.setTextSize(1);
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

    int converted = 0;

    if (decoder.openRAM(fb->buf, fb->len, drawMCUs))
    {
        decoder.setPixelType(RGB565_BIG_ENDIAN);
        converted = decoder.decode(0, 0, 0);
        decoder.close();
    }

    esp_camera_fb_return(fb);

    if (converted == 1 && push_sprite)
    {
        spr.pushSprite(0, 0);
    }

    return converted == 1;
}

bool sendDisplayMessage(const char *text, unsigned long duration_ms)
{
    if (!messageQueue)
    {
        return false;
    }

    DisplayMessage msg{};
    strncpy(msg.text, text, DISPLAY_MSG_MAX_LEN - 1);
    msg.duration = pdMS_TO_TICKS(duration_ms);

    return xQueueSendToBack(messageQueue, &msg, 0) == pdPASS;
}

void TaskDisplayCode(void *pvParameters)
{
    changeTaskCount(1);

    DisplayMessage msg{};

    bool hasMessage = false;
    TickType_t overlayUntil = 0;

    const TickType_t tickDelay = pdMS_TO_TICKS(50);

    while (current_state != SystemState::SLEEPING)
    {
        TickType_t now = xTaskGetTickCount();

        if (hasMessage && now >= overlayUntil)
        {
            hasMessage = false;
        }

        if (!hasMessage && xQueueReceive(messageQueue, &msg, 0) == pdPASS)
        {
            hasMessage = true;
            overlayUntil = (msg.duration > 0) ? (now + msg.duration) : 0;
            showText(msg.text, true);
        }

        if (!hasMessage && current_state == SystemState::READY)
        {
            showCamFrame(true);
        }

        vTaskDelay(tickDelay);
    }

    changeTaskCount(-1);
    vTaskDelete(nullptr);
}
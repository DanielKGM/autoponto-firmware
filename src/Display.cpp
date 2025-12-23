#include "Display.h"
#include "Globals.h"
#include "Config.h"

using DisplayMessage = struct
{
    char text[DISPLAY_MSG_MAX_LEN];
    TickType_t duration;
};

JPEGDEC decoder;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
QueueHandle_t messageQueue = xQueueCreate(5, sizeof(DisplayMessage));
QueueHandle_t frameQueue = xQueueCreate(1, sizeof(FrameBuffer));

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

bool showCamFrame(bool pushSprite, bool captureFrame = false)
{
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
        return false;
    }

    // Buffer PSRAM
    uint8_t *fbCopy = captureFrame ? (uint8_t *)ps_malloc(fb->len) : nullptr;

    if (fbCopy)
    {
        memcpy(fbCopy, fb->buf, fb->len);

        FrameBuffer frame;
        frame.len = fb->len;
        frame.data = fbCopy;

        FrameBuffer old;
        if (xQueueReceive(frameQueue, &old, 0) == pdTRUE)
        {
            free(old.data);
        }

        xQueueOverwrite(frameQueue, &frame);
    }

    // Display
    bool converted = false;

    if (decoder.openRAM(fb->buf, fb->len, drawMCUs))
    {
        decoder.setPixelType(RGB565_BIG_ENDIAN);
        converted = decoder.decode(0, 0, 0);
        decoder.close();
    }

    esp_camera_fb_return(fb);

    if (converted && pushSprite)
    {
        spr.pushSprite(0, 0);
    }

    return converted;
}

bool sendDisplayMessage(const char *text, unsigned long durationMs)
{
    if (!messageQueue)
    {
        return false;
    }

    DisplayMessage msg{};
    strncpy(msg.text, text, DISPLAY_MSG_MAX_LEN - 1);
    msg.duration = pdMS_TO_TICKS(durationMs);

    return xQueueSendToBack(messageQueue, &msg, 0) == pdPASS;
}

void TaskDisplayCode(void *pvParameters)
{
    changeTaskCount(1);

    DisplayMessage msg{};

    bool hasMessage = false;
    TickType_t overlayUntil = 0;

    const TickType_t tickDelay = pdMS_TO_TICKS(50);

    while (systemState != SystemState::SLEEPING)
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

        if (!hasMessage && systemState == SystemState::READY)
        {
            showCamFrame(true, ulTaskNotifyTake(pdTRUE, 0) > 0);
        }

        vTaskDelay(tickDelay);
    }

    changeTaskCount(-1);
    vTaskDelete(nullptr);
}
#include "Display.h"
#include "Globals.h"
#include "Config.h"
#include "Icons.h"

struct DisplayMessage
{
    char text[DISPLAY_MSG_MAX_LEN];
    TickType_t duration;
    const Icon *icon; // nullptr = sem ícone
};

const Icon ICON_WIFI = {wifi_icon, 38, 38};
const Icon ICON_SAD = {sad_icon, 38, 38};
const Icon ICON_HAPPY = {happy_icon, 38, 38};
const Icon ICON_SERVER = {server_icon, 38, 38};

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
    tft.setPivot(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(1);
}

void initSprite()
{
    spr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    spr.fillScreen(TFT_BLACK);
    spr.setSwapBytes(false);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.setTextDatum(TL_DATUM);

    spr.setTextSize(1);
}

void showText(const char *text,
              const Icon *icon)
{
    const int spacing = 10;
    const int lineHeight = spr.fontHeight();

    int iconH = icon ? icon->height : 0;
    int iconW = icon ? icon->width : 0;

    int totalHeight = lineHeight;
    if (icon)
        totalHeight += iconH + spacing;

    int y = (DISPLAY_HEIGHT - totalHeight) / 2;

    if (icon)
    {
        int x = (DISPLAY_WIDTH - iconW) / 2;
        spr.pushImage(x, y, iconW, iconH, icon->data);
        y += iconH + spacing;
    }

    String out = text;

    while (spr.textWidth(out) > DISPLAY_WIDTH && out.length() > 3)
    {
        out.remove(out.length() - 4);
        out += "...";
    }

    int textX = (DISPLAY_WIDTH - spr.textWidth(out)) / 2;
    spr.drawString(out, textX, y);

    spr.pushRotated(270);
}

bool showCamFrame(bool pushSprite, bool captureFrame)
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

    spr.fillScreen(TFT_BLACK);

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

bool sendDisplayMessage(const char *text, unsigned long durationMs, const Icon *icon)
{
    if (!messageQueue)
    {
        return false;
    }

    DisplayMessage msg{};
    strncpy(msg.text, text, DISPLAY_MSG_MAX_LEN - 1);
    msg.duration = pdMS_TO_TICKS(durationMs);
    msg.icon = icon;

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
            showText(msg.text, msg.icon);
        }

        if (!hasMessage && (systemState == SystemState::READY))
        {
            showCamFrame(true, ulTaskNotifyTake(pdTRUE, 0) > 0);
        }

        vTaskDelay(tickDelay);
    }

    changeTaskCount(-1);
    vTaskDelete(nullptr);
}
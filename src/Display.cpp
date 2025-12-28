#include "Display.h"
#include "Globals.h"
#include "Config.h"
#include "Icons.h"
#include "Power.h"
#include "Font.h"

namespace display
{
    QueueHandle_t frameQueue = xQueueCreate(1, sizeof(FrameBuffer));

    const Icon ICON_WIFI = {wifi_icon, 38, 38, TFT_BLACK};
    const Icon ICON_SAD = {sad_icon, 38, 38, TFT_RED};
    const Icon ICON_HAPPY = {happy_icon, 38, 38, TFT_GREEN};
    const Icon ICON_SERVER = {server_icon, 38, 38, TFT_BLACK};

    namespace
    {
        struct DisplayMessage
        {
            char text[DISPLAY_MSG_MAX_LEN];
            TickType_t duration;
            const Icon *icon; // nullptr = sem ícone
        };

        JPEGDEC decoder;
        TFT_eSPI tft = TFT_eSPI();
        TFT_eSprite bgSpr = TFT_eSprite(&tft);
        TFT_eSprite spr = TFT_eSprite(&tft);
        QueueHandle_t messageQueue = xQueueCreate(5, sizeof(DisplayMessage));

        int drawMCUs(JPEGDRAW *pDraw)
        {
            tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);

            return 1;
        }

        void splitTextTwoLines(const String &text, String &l1, String &l2, int maxW)
        {
            if (spr.textWidth(text) <= maxW)
            {
                l1 = text;
                l2 = "";
                return;
            }

            int split = 0;

            for (int i = 0; i < text.length(); i++)
            {
                if (text[i] == ' ' &&
                    spr.textWidth(text.substring(0, i)) <= maxW)
                {
                    split = i;
                }
            }

            if (split == 0)
            {
                split = text.length();
            }

            l1 = text.substring(0, split);
            l2 = text.substring(split + 1);

            if (spr.textWidth(l2) > maxW)
            {
                const String ell = "...";
                int ellW = spr.textWidth(ell);

                while (!l2.isEmpty() &&
                       spr.textWidth(l2) + ellW > maxW)
                {
                    l2.remove(l2.length() - 1);
                }
                l2 += ell;
            }
        }

        void showText(const char *text, const Icon *icon, int spinnerAngle = 0)
        {
            const int spinnerThickness = 8;
            const int spriteMargin = spinnerThickness + 6;
            const int spriteW = DISPLAY_WIDTH - (spriteMargin * 2);
            const int lineH = spr.fontHeight();
            const int spacing = 10;

            String l1;
            String l2;
            splitTextTwoLines(text, l1, l2, spriteW);

            const bool hasIcon = icon != nullptr;
            const bool hasLine2 = !l2.isEmpty();

            int contentH =
                (hasIcon ? icon->height + spacing : 0) +
                lineH * (hasLine2 ? 2 : 1);

            spr.createSprite(spriteW, static_cast<int16_t>(contentH));

            int y = 0;

            // Ícone
            if (hasIcon)
            {
                spr.pushImage(
                    (spriteW - icon->width) / 2,
                    y,
                    icon->width,
                    icon->height,
                    icon->data);
                y += icon->height + spacing;
            }

            int cx = spriteW / 2;
            spr.drawString(l1, cx, y + lineH / 2);

            if (hasLine2)
            {
                spr.drawString(l2, cx, y + lineH + lineH / 2);
            }

            bgSpr.fillSprite(hasIcon ? icon->color : TFT_BLACK);

            if (spinnerAngle > 0)
            {
                const int radius = DISPLAY_WIDTH / 2;
                bgSpr.drawArc(
                    DISPLAY_WIDTH / 2,
                    DISPLAY_HEIGHT / 2,
                    radius,
                    radius - spinnerThickness,
                    0,
                    spinnerAngle,
                    TFT_WHITE,
                    TFT_WHITE);
            }

            if (hasIcon && icon->color != TFT_BLACK)
            {
                spr.pushToSprite(
                    &bgSpr,
                    spriteMargin,
                    (DISPLAY_HEIGHT - contentH) / 2,
                    TFT_BLACK);
            }
            else
            {
                spr.pushToSprite(
                    &bgSpr,
                    spriteMargin,
                    (DISPLAY_HEIGHT - contentH) / 2);
            }

            spr.deleteSprite();
            bgSpr.pushRotated(270);
        }

        bool showCamFrame(bool captureFrame)
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

            return converted;
        }
    } //

    void configTFT()
    {
        tft.init();
        tft.setPivot(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
        tft.fillScreen(TFT_BLACK);
        tft.setRotation(1);
    }

    void configSprite()
    {
        bgSpr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        spr.setTextColor(TFT_WHITE);
        spr.setTextDatum(MC_DATUM);
        spr.setFreeFont(&Determination);
    }

    bool sendDisplayMessage(const char *text, unsigned long durationMs, const Icon *icon)
    {
        if (!messageQueue || idleFlag)
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

        configTFT();
        configSprite();

        DisplayMessage msg{};
        TickType_t overlayUntil = 0;

        const TickType_t idleDelay = pdMS_TO_TICKS(1000);
        const TickType_t videoDelay = pdMS_TO_TICKS(10);
        const TickType_t normalDelay = pdMS_TO_TICKS(100);

        TickType_t currentDelay = normalDelay;

        while (true)
        {
            TickType_t now = xTaskGetTickCount();

            //
            if (idleFlag)
            {
                msg = DisplayMessage{};
                tft.fillScreen(TFT_BLACK);
                currentDelay = idleDelay;
            }
            else if (overlayUntil > now && msg.duration > 0)
            {
                int spinnerAngle =
                    360 - ((360UL * (overlayUntil - now)) / msg.duration);
                showText(msg.text, msg.icon, spinnerAngle);
            }
            else if (xQueueReceive(messageQueue, &msg, 0) == pdPASS)
            {
                //
                overlayUntil = (msg.duration > 0) ? (now + msg.duration) : 0;
                showText(msg.text, msg.icon);
                currentDelay = normalDelay;
            }
            else if (checkState(SystemState::WORKING))
            {
                showCamFrame(ulTaskNotifyTake(pdTRUE, 0) > 0);
                currentDelay = videoDelay;
            }

            if (checkSleepEvent(currentDelay))
            {
                break;
            }

            vTaskDelay(currentDelay);
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }

}

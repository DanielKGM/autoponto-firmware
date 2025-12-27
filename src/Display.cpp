#include "Display.h"
#include "Globals.h"
#include "Config.h"
#include "Icons.h"
#include "Power.h"
#include "Font.h"

namespace display
{
    QueueHandle_t frameQueue = xQueueCreate(1, sizeof(FrameBuffer));

    const Icon ICON_WIFI = {wifi_icon, 38, 38};
    const Icon ICON_SAD = {sad_icon, 38, 38};
    const Icon ICON_HAPPY = {happy_icon, 38, 38};
    const Icon ICON_SERVER = {server_icon, 38, 38};

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
        TFT_eSprite spr = TFT_eSprite(&tft);
        QueueHandle_t messageQueue = xQueueCreate(5, sizeof(DisplayMessage));

        int drawMCUs(JPEGDRAW *pDraw)
        {
            tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);

            return 1;
        }

        void splitTextTwoLines(const String &text, String &line1, String &line2, int maxWidth)
        {
            if (spr.textWidth(text) <= maxWidth)
            {
                line1 = text;
                line2 = "";
                return;
            }

            int splitIndex = -1;
            String currentText = "";

            for (int i = 0; i < text.length(); i++)
            {
                if (text[i] == ' ')
                {
                    if (spr.textWidth(text.substring(0, i)) > maxWidth)
                    {
                        break;
                    }
                    splitIndex = i;
                }
            }

            if (splitIndex == -1)
            {
                splitIndex = text.length();
            }

            line1 = text.substring(0, splitIndex);
            line2 = text.substring(splitIndex + 1);

            if (spr.textWidth(line2) > maxWidth)
            {
                String ellipsis = "...";

                int ellipsisWidth = spr.textWidth(ellipsis);

                while (line2.length() > 0 &&
                       spr.textWidth(line2) + ellipsisWidth > maxWidth)
                {
                    line2.remove(line2.length() - 1);
                }

                line2 += ellipsis;
            }
        }

        void showText(const char *text, const Icon *icon)
        {
            spr.fillScreen(TFT_BLACK);

            const int iconSpacing = 10;
            const int lineHeight = spr.fontHeight();
            const int iconH = icon ? icon->height : 0;
            const int iconW = icon ? icon->width : 0;

            String line1;
            String line2;

            splitTextTwoLines(text, line1, line2, DISPLAY_WIDTH - 10);

            bool hasLine2 = !line2.isEmpty();
            int textBlockHeight = lineHeight * (hasLine2 ? 2 : 1);
            int totalHeight = iconH + (icon ? iconSpacing : 0) + textBlockHeight;

            int startY = (DISPLAY_HEIGHT - totalHeight) / 2;

            //
            if (icon)
            {
                int iconX = (DISPLAY_WIDTH - iconW) / 2;
                spr.pushImage(iconX, startY, iconW, iconH, icon->data);
                startY += iconH + iconSpacing;
            }

            //
            int centerX = DISPLAY_WIDTH / 2;

            spr.drawString(line1, centerX, startY + (lineHeight / 2));

            if (hasLine2)
            {
                spr.drawString(line2, centerX, startY + lineHeight + (lineHeight / 2));
            }

            spr.pushRotated(270);
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
        spr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        spr.setSwapBytes(false);
        spr.setTextColor(TFT_WHITE, TFT_BLACK);
        spr.setTextDatum(MC_DATUM);
        spr.setTextWrap(true);
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

        bool hasMessage = false;
        TickType_t overlayUntil = 0;

        TickType_t delay = pdMS_TO_TICKS(100);
        const TickType_t idleDelay = pdMS_TO_TICKS(1000);
        const TickType_t videoDelay = pdMS_TO_TICKS(5);

        while (true)
        {
            TickType_t now = xTaskGetTickCount();

            if (hasMessage && now >= overlayUntil)
            {
                hasMessage = false;
            }

            if (idleFlag)
            {
                tft.fillScreen(TFT_BLACK);
            }
            else
            {
                if (!hasMessage && xQueueReceive(messageQueue, &msg, 0) == pdPASS)
                {
                    hasMessage = true;
                    overlayUntil = (msg.duration > 0) ? (now + msg.duration) : 0;
                    showText(msg.text, msg.icon);
                }

                if (!hasMessage && checkState(SystemState::WORKING))
                {
                    showCamFrame(ulTaskNotifyTake(pdTRUE, 0) > 0);
                    delay = videoDelay;
                }
            }

            if (checkSleepEvent(idleFlag ? idleDelay : delay))
            {
                break;
            }
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }
}

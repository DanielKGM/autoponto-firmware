#include "Display.h"
#include "Globals.h"
#include "Config.h"
#include "Icons.h"
#include "Power.h"
#include "Font.h"
#include "Camera.h"
#include "esp_timer.h"

namespace display
{

    const Icon ICON_WIFI = {wifi_icon, 38, 38, TFT_BLACK};
    const Icon ICON_SAD = {sad_icon, 38, 38, TFT_RED};
    const Icon ICON_HAPPY = {happy_icon, 38, 38, TFT_DARKGREEN};
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
        TFT_eSprite screenSpr = TFT_eSprite(&tft);
        TFT_eSprite textSpr = TFT_eSprite(&tft);
        QueueHandle_t messageQueue = xQueueCreate(5, sizeof(DisplayMessage));
        volatile bool fullscreenMessageActive = false;

        const TickType_t contextTextCycle = pdMS_TO_TICKS(3000);
        const uint16_t textBackgroundColor = TFT_BLACK;

        struct TextBlock
        {
            int16_t width;
            int16_t height;
        };

        int drawMCUs(JPEGDRAW *pDraw)
        {
            screenSpr.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
            return 1;
        }

        void splitTextTwoLines(const String &text, String &l1, String &l2, int maxW)
        {
            if (textSpr.textWidth(text) <= maxW)
            {
                l1 = text;
                l2 = "";
                return;
            }

            int split = 0;

            for (int i = 0; i < text.length(); i++)
            {
                if (text[i] == ' ' &&
                    textSpr.textWidth(text.substring(0, i)) <= maxW)
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

            if (textSpr.textWidth(l2) > maxW)
            {
                const String ell = "...";
                int ellW = textSpr.textWidth(ell);

                while (!l2.isEmpty() &&
                       textSpr.textWidth(l2) + ellW > maxW)
                {
                    l2.remove(l2.length() - 1);
                }
                l2 += ell;
            }
        }

        void formatMinutesSeconds(uint64_t ms, char *out, size_t outSize)
        {
            uint64_t totalSeconds = ms / 1000ULL;
            unsigned long minutes = static_cast<unsigned long>(totalSeconds / 60ULL);
            unsigned long seconds = static_cast<unsigned long>(totalSeconds % 60ULL);

            snprintf(out, outSize, "%lumin %02lus", minutes, seconds);
        }

        bool getContextText(TickType_t now, char *out, size_t outSize)
        {
            bool showName = ((now - context.fetchTick) / contextTextCycle) % 2 == 0;

            if (context.msRemaining > 0)
            {
                if (showName)
                {
                    snprintf(out, outSize, "Chamada: %s", context.lesson_name);
                    return true;
                }

                char timeText[24];
                formatMinutesSeconds(
                    getRemainingMs(now, context.msRemaining, context.fetchTick),
                    timeText,
                    sizeof(timeText));
                snprintf(out, outSize, "Restam %s", timeText);
                return true;
            }

            if (context.msForNext > 0)
            {
                if (showName)
                {
                    snprintf(out, outSize, "Proxima turma: %s", context.lesson_name);
                    return true;
                }

                char timeText[24];
                formatMinutesSeconds(
                    getRemainingMs(now, context.msForNext, context.fetchTick),
                    timeText,
                    sizeof(timeText));
                snprintf(out, outSize, "Chamada em %s", timeText);
                return true;
            }

            return false;
        }

        TextBlock buildTextBlock(
            const char *text,
            const Icon *icon,
            int maxWidth,
            int paddingX,
            int paddingY)
        {
            int blockW = maxWidth;
            if (blockW < 1)
            {
                blockW = 1;
            }

            int textMaxW = blockW - (paddingX * 2);
            if (textMaxW < 1)
            {
                textMaxW = 1;
            }

            const int lineH = textSpr.fontHeight();
            const int spacing = 10;

            String l1;
            String l2;
            splitTextTwoLines(text, l1, l2, textMaxW);

            const bool hasIcon = icon != nullptr;
            const bool hasLine2 = !l2.isEmpty();
            const int lines = hasLine2 ? 2 : 1;

            int contentH =
                (hasIcon ? icon->height + spacing : 0) +
                lineH * lines;

            int blockH = contentH + (paddingY * 2);
            if (blockH < 1)
            {
                blockH = 1;
            }

            textSpr.deleteSprite();
            textSpr.createSprite(static_cast<int16_t>(blockW), static_cast<int16_t>(blockH));
            textSpr.fillSprite(textBackgroundColor);
            textSpr.setTextColor(TFT_WHITE, textBackgroundColor);
            textSpr.setTextDatum(MC_DATUM);
            textSpr.setFreeFont(&Determination);

            int y = paddingY;

            if (hasIcon)
            {
                textSpr.pushImage(
                    (blockW - icon->width) / 2,
                    y,
                    icon->width,
                    icon->height,
                    icon->data);
                y += icon->height + spacing;
            }

            int cx = blockW / 2;
            textSpr.drawString(l1, cx, y + lineH / 2);

            if (hasLine2)
            {
                textSpr.drawString(l2, cx, y + lineH + lineH / 2);
            }

            textSpr.setPivot(blockW / 2, blockH / 2);
            return {
                static_cast<int16_t>(blockW),
                static_cast<int16_t>(blockH)};
        }

        void showText(const char *text, const Icon *icon, int spinnerAngle = 0)
        {
            const int spinnerThickness = 8;
            const int screenMargin = spinnerThickness + 6;
            const int maxBlockW = DISPLAY_WIDTH - (screenMargin * 2);

            TextBlock block = buildTextBlock(text, icon, maxBlockW, 0, 0);
            const bool hasIcon = icon != nullptr;

            screenSpr.fillSprite(hasIcon ? icon->color : TFT_BLACK);

            if (spinnerAngle > 0)
            {
                const int radius = DISPLAY_WIDTH / 2;
                screenSpr.drawArc(
                    DISPLAY_WIDTH / 2,
                    DISPLAY_HEIGHT / 2,
                    radius,
                    radius - spinnerThickness,
                    0,
                    spinnerAngle,
                    TFT_WHITE,
                    TFT_WHITE);
            }

            textSpr.pushToSprite(
                &screenSpr,
                (DISPLAY_WIDTH - block.width) / 2,
                (DISPLAY_HEIGHT - block.height) / 2,
                textBackgroundColor);

            screenSpr.pushRotated(180);
        }

        bool drawFrame()
        {
            camera::FrameBuffer frame;

            if (!camera::takePreviewFrame(frame, 0))
            {
                return false;
            }

            bool converted = false;
            screenSpr.fillSprite(TFT_BLACK);

            if (decoder.openRAM(frame.data, frame.len, drawMCUs))
            {
                decoder.setPixelType(RGB565_BIG_ENDIAN);
                converted = decoder.decode(0, 0, 0);
                decoder.close();
            }

            camera::releaseFrame(frame);
            return converted;
        }

        void drawCameraContextText(const char *text)
        {
            const int margin = 36;
            const int paddingY = 8;
            const int blockW = DISPLAY_WIDTH - (margin * 2);

            TextBlock block = buildTextBlock(
                text,
                nullptr,
                blockW,
                0,
                paddingY);

            int panelX = (DISPLAY_WIDTH - block.width) / 2;
            int panelY = (DISPLAY_HEIGHT - block.height) / 2;

            screenSpr.fillRect(panelX, panelY, block.width, block.height, textBackgroundColor);

            screenSpr.setPivot(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
            textSpr.pushRotated(&screenSpr, 180, textBackgroundColor);
        }

        void showLiveCam(TickType_t now)
        {
            char text[128];

            if (!drawFrame())
            {
                return;
            }

            if (getContextText(now, text, sizeof(text)))
            {
                drawCameraContextText(text);
            }

            screenSpr.pushSprite(0, 0);
        }

        void showContextScreen(TickType_t now)
        {
            char text[128];

            if (getContextText(now, text, sizeof(text)))
            {
                showText(text, nullptr);
            }
        }
    } //

    void configTFT()
    {
        tft.init();
        tft.fillScreen(TFT_BLACK);
        tft.setPivot(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
    }

    void configSprite()
    {
        screenSpr.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        screenSpr.setPivot(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2);
        screenSpr.setTextColor(TFT_WHITE, TFT_BLACK);
        screenSpr.setTextDatum(MC_DATUM);
        screenSpr.setFreeFont(&Determination);
        textSpr.setTextColor(TFT_WHITE);
        textSpr.setTextDatum(MC_DATUM);
        textSpr.setFreeFont(&Determination);
    }

    bool sendDisplayMessage(const char *text, unsigned long durationMs, const Icon *icon)
    {
        if (!messageQueue || power::checkIdle())
        {
            return false;
        }

        DisplayMessage msg{};
        strncpy(msg.text, text, DISPLAY_MSG_MAX_LEN - 1);
        msg.duration = pdMS_TO_TICKS(durationMs);
        msg.icon = icon;

        return xQueueSendToBack(messageQueue, &msg, 0) == pdPASS;
    }

    bool isFullscreenMessageActive()
    {
        return fullscreenMessageActive;
    }

    void TaskDisplayCode(void *pvParameters)
    {
        changeTaskCount(1);

        DisplayMessage msg{};
        TickType_t messageUntil = 0;

        const TickType_t idleDelay = pdMS_TO_TICKS(990);
        const TickType_t videoDelay = pdMS_TO_TICKS(99); // ~10 FPS

        TickType_t currentDelay = videoDelay;
        TickType_t periodStartTick = xTaskGetTickCount();
        // only trigger idle actions once
        bool idleTrigger = true;

        while (true)
        {
            int64_t cycleStart = esp_timer_get_time();
            TickType_t now = xTaskGetTickCount();

            //

            if (!power::checkIdle() && !idleTrigger)
            {
                idleTrigger = true;
                currentDelay = videoDelay;
            }

            if (power::checkIdle() && idleTrigger)
            {
                xQueueReset(messageQueue);
                msg = DisplayMessage{};
                tft.fillScreen(TFT_BLACK);
                currentDelay = idleDelay;
                messageUntil = 0;
                fullscreenMessageActive = false;
                idleTrigger = false;
            }
            else if (messageUntil > now && msg.duration > 0)
            {
                fullscreenMessageActive = true;
                int spinnerAngle =
                    360 - ((360UL * (messageUntil - now)) / msg.duration);
                showText(msg.text, msg.icon, spinnerAngle);
            }
            else if (xQueueReceive(messageQueue, &msg, 0) == pdPASS)
            {
                //
                messageUntil = (msg.duration > 0) ? (now + msg.duration) : 0;
                fullscreenMessageActive = msg.duration > 0;
                showText(msg.text, msg.icon);
                currentDelay = videoDelay;
            }
            else if (checkState(SystemState::WAITING_SERVER) ||
                     (checkState(SystemState::WORKING) && context.msRemaining > 0))
            {
                fullscreenMessageActive = false;
                showLiveCam(now);
                currentDelay = videoDelay;
            }
            else if (checkState(SystemState::WORKING) && context.msForNext > 0)
            {
                fullscreenMessageActive = false;
                showContextScreen(now);
                currentDelay = videoDelay;
            }
            else
            {
                fullscreenMessageActive = false;
            }

            recordTaskRuntime(TaskMetric::DISPLAY_TASK, static_cast<uint32_t>(esp_timer_get_time() - cycleStart));

            if (waitForNextPeriodOrSleep(periodStartTick, currentDelay))
            {
                break;
            }
        }

        changeTaskCount(-1);
        vTaskDelete(nullptr);
    }

}

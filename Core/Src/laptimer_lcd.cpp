#include "laptimer_lcd.h"
#include "DEV_Config.h"

#define DEFAULT_LINE LINE_SOLID
#define DEFAULT_PIXEL DOT_PIXEL_1X1

void lcdInit()
{
    LCD_Init(D2U_L2R);
}

void lcdClear()
{
    LCD_Clear(BLACK);
}
void lcdCopy()
{
    LCD_Copy();
}

bool lcdPrintString(uint16_t posX, uint16_t posY, const char *string, int fontSize, uint16_t colorBackground, uint16_t colorForeground)
{
    if (string == NULL || posY > LCD_HEIGHT || posX > LCD_WIDTH)
        return 1;
    sFONT *font;
    switch (fontSize)
    {
    case 8:
        font = &Font8;
        break;
    case 12:
        font = &Font12;
        break;
    case 16:
        font = &Font16;
        break;
    case 20:
        font = &Font20;
        break;
    case 24:
        font = &Font24;
        break;
    default:
        return 1;
    }
    LCD_DisplayString(posX, posY, string, font, colorBackground, colorForeground);
    return 0;
}

bool lcdPrintLine(uint16_t posX0, uint16_t posY0, uint16_t posX1, uint16_t posY1, uint16_t color)
{
    if (posX0 > LCD_WIDTH || posX1 > LCD_WIDTH || posY0 > LCD_HEIGHT || posY1 > LCD_HEIGHT || posX0 > posX1 || posY0 > posY1)
        return 1;
    LCD_DrawLine(posX0, posY0, posX1, posY1, color, DEFAULT_LINE, DEFAULT_PIXEL);
    return 0;
}

bool lcdIsBusy()
{
    return dmaBusyFlag;
}

void lcdSetFree()
{
    dmaBusyFlag = false;
}

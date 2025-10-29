#pragma once

#include <cstdint>

#define LCD_WIDTH 160
#define LCD_HEIGHT 128

#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x1F00
#define BRED 0x1FF8
#define GRED 0xE0FF
#define GBLUE 0xFF07
#define RED 0x00F8
#define MAGENTA 0x1FF8
#define GREEN 0xE007
#define CYAN 0xFF7F
#define YELLOW 0xE0FF
#define BROWN 0x40BC
#define BRRED 0x07FC
#define GRAY 0x3084

#define PADDING 5
#define LAPLIST_POS_X 5
#define LAPLIST_POS_Y 40
#define LAPLIST_SPACING 15
#define CURRENT_LAPTIME_POS_X 5
#define CURRENT_LAPTIME_POS_Y 15

#define CURRENT_LAPTIME_FONT 16
#define LAPLIST_FONT 8

void lcdInit();
void lcdClear();
void lcdCopy();

bool lcdPrintString(uint16_t posX, uint16_t posY, const char *string, int fontSize, uint16_t colorBackground, uint16_t colorForeground);
bool lcdPrintLine(uint16_t posX0, uint16_t posY0, uint16_t posX1, uint16_t posY1, uint16_t color);
bool lcdIsBusy();
void lcdSetFree();

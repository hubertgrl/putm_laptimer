#include "laptimer_functions.h"
#include "usart.h"
#include "gpio.h"

LapTime lapTimeCurrent{1, 0};
LapTime lapTimeSaved{1, 0};

LapTime lapListTop[LAPLIST_SIZE];
LapTime lapListLast[LAPLIST_SIZE];

volatile LapMode lapMode = ONE_GATE_MODE;

volatile bool lapResetFlag = true;

LapMode checkMode()
{
    if (HAL_GPIO_ReadPin(LAP_MODE_GPIO_Port, LAP_MODE_Pin) == GPIO_PIN_RESET)
        return ONE_GATE_MODE;
    else
        return TWO_GATE_MODE;
}

void saveLapTime()
{
    for (int i = LAPLIST_SIZE - 1; i > 0; i--)
    {
        lapListLast[i] = lapListLast[i - 1];
    }
    lapListLast[0] = lapTimeSaved;

    for (int i = 0; i < LAPLIST_SIZE; i++)
    {
        if (lapTimeSaved.time < lapListTop[i].time || lapListTop[i].time == 0)
        {
            for (int j = LAPLIST_SIZE - 1; j > i; j--)
            {
                lapListTop[j] = lapListTop[j - 1];
            }
            lapListTop[i] = lapTimeSaved;
            break;
        }
    }
    lapTimeSaved = {0, 0};
}

void convertLapTime(LapTime lapTime, char *lapTimeString, size_t size)
{
    if (lapTimeString == NULL)
        return;

    if (lapTime.time == 0)
    {
        snprintf(lapTimeString, size, "--. --:--:--");
        return;
    }

    unsigned int mm = (lapTime.time / 6000) % 60;
    unsigned int ss = (lapTime.time / 100) % 60;
    unsigned int ms = lapTime.time % 100;
    if (size == 13U)
        snprintf(lapTimeString, size, "%02u. %02u:%02u:%02u", lapTime.count, mm, ss, ms);
    else if (size == 14U)
        snprintf(lapTimeString, size, "%02u. %02u:%02u:%02u\n", lapTime.count, mm, ss, ms);
}

void sendLapTime(char *lapTimeString, size_t size)
{
    if (lapTimeString == NULL)
        return;
    HAL_UART_Transmit_DMA(&huart3, (uint8_t *)lapTimeString, size);
}

void showLapTime(LapTime lapTime, int posX, int posY, int font)
{
    char lapTimeString[13];
    convertLapTime(lapTime, lapTimeString, sizeof(lapTimeString));
    if (lcdPrintString(posX, posY, lapTimeString, font, BLACK, WHITE))
        lcdPrintString(10, 10, "ERROR", 24, BLACK, RED);
}

void showUI()
{
    lcdPrintString(PADDING, PADDING, "CURRENT LAP", UI_FONT, BLACK, WHITE);
    lcdPrintString(LAPLIST_POS_X, LAPLIST_POS_Y, "LAST " STR(LAPLIST_SIZE_SHOW) " LAPS", UI_FONT, BLACK, WHITE);
    lcdPrintString(LAPLIST_POS_X + LCD_WIDTH / 2, LAPLIST_POS_Y, "TOP " STR(LAPLIST_SIZE_SHOW) " LAPS", UI_FONT, BLACK, WHITE);
    lcdPrintLine(0, LAPLIST_POS_Y - PADDING, LCD_WIDTH, LAPLIST_POS_Y - PADDING, WHITE);
    lcdPrintLine(LCD_WIDTH / 2, LAPLIST_POS_Y - PADDING, LCD_WIDTH / 2, LCD_HEIGHT, WHITE);
}
void showMode()
{
    switch (lapMode)
    {
    case ONE_GATE_MODE:
        lcdPrintString(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, "1 GATE", UI_FONT, BLACK, GRAY);
        break;
    case TWO_GATE_MODE:
        lcdPrintString(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, "2 GATE", UI_FONT, BLACK, GRAY);
        break;
    default:
        break;
    }
    if (lapResetFlag)
        lcdPrintString(LCD_WIDTH / 2, PADDING, "STOP", UI_FONT, BLACK, RED);
    else
        lcdPrintString(LCD_WIDTH / 2, PADDING, "    ", UI_FONT, BLACK, BLACK);
}

void showLapLists()
{
    for (int i = 0; i < LAPLIST_SIZE_SHOW; i++)
    {
        showLapTime(lapListLast[i], LAPLIST_POS_X, LAPLIST_POS_Y + LAPLIST_SPACING + i * LAPLIST_SPACING, UI_FONT);
    }
    for (int i = 0; i < LAPLIST_SIZE_SHOW; i++)
    {
        showLapTime(lapListTop[i], LCD_WIDTH / 2 + LAPLIST_POS_X, LAPLIST_POS_Y + LAPLIST_SPACING + i * LAPLIST_SPACING, UI_FONT);
    }
}

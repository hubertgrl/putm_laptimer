#pragma once

#include <cstdio>
#include "laptimer_lcd.h"

#define LAPLIST_SIZE 20

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

struct LapTime
{
    unsigned int count;
    unsigned long time;
};

enum LapMode
{
    ONE_GATE_MODE,
    TWO_GATE_MODE,
};

extern LapTime lapTimeCurrent;
extern LapTime lapTimeSaved;

extern LapTime lapListTop[LAPLIST_SIZE];
extern LapTime lapListLast[LAPLIST_SIZE];

extern volatile LapMode lapMode;

extern volatile bool lapResetFlag;

LapMode checkMode();
void saveLapTime();
void convertLapTime(LapTime lapTime, char *lapTimeString, size_t size);
void sendLapTime(char *lapTimeString, size_t size);

void showLapTime(LapTime lapTime, int posX, int posY, int font);
void showUI();
void showMode();
void showLapLists();

/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tim.h"
#include "dma.h"
#include "spi.h"
#include "rtc.h"

#include "stdio.h"
#include "DEV_Config.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LAPLIST_SIZE 5

#define LAPTIME_MIN 5000

#define PADDING 5
#define LAPLIST_POS_X 5
#define LAPLIST_POS_Y 40
#define CURRENT_LAPTIME_POS_X 5
#define CURRENT_LAPTIME_POS_Y 15

#define CURRENT_LAPTIME_FONT &Font16
#define LAPLIST_FONT &Font8
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

struct LapTime
{
    unsigned int count;
    unsigned long time;
};

LapTime lapTimeCurrent = {1, 0};
LapTime lapTimeSaved = {1, 0};

LapTime lapListTop[LAPLIST_SIZE];
LapTime lapListLast[LAPLIST_SIZE];

volatile bool lapResetFlag = true;

void rtcReset()
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    time.Hours = 0;
    time.Minutes = 0;
    time.Seconds = 0;
    time.SubSeconds = 0;
    time.TimeFormat = RTC_HOURFORMAT12_AM;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    date.WeekDay = RTC_WEEKDAY_MONDAY;
    date.Month = RTC_MONTH_JANUARY;
    date.Date = 1;
    date.Year = 0;

    HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
}

unsigned int rtcGetTime()
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    return ((time.Hours * 3600 + time.Minutes * 60 + time.Seconds) * 1000 + ((255 - time.SubSeconds) * 1000) / 256) / 10;
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
            lapTimeSaved = {0, 0};
            break;
        }
    }
}

bool convertLapTime(LapTime lapTime, char *lapTimeString)
{
    if (lapTimeString == NULL)
        return 1;

    if (lapTime.time == 0)
    {
        sprintf(lapTimeString, "--. --:--:--");
        return 0;
    }

    unsigned int mm = 0;
    unsigned int ss = 0;
    unsigned int ms = 0;
    mm = (lapTime.time / 6000) % 60;
    ss = (lapTime.time / 100) % 60;
    ms = lapTime.time % 100;

    sprintf(lapTimeString, "%02u. %02u:%02u:%02u", lapTime.count, mm, ss, ms);
    return 0;
}

void printLapTime(LapTime lapTime, int posX, int posY, sFONT *font)
{
    char lapTimeString[13] = "\0";
    convertLapTime(lapTime, lapTimeString);
    LCD_DisplayString(posX, posY, lapTimeString, font, BLACK, WHITE);
}

void printUI()
{
    LCD_DisplayString(PADDING, PADDING, "CURRENT LAP", &Font8, BLACK, WHITE);
    LCD_DisplayString(LAPLIST_POS_X, LAPLIST_POS_Y, "LAST 5 LAPS", &Font8, BLACK, WHITE);
    LCD_DisplayString(LAPLIST_POS_X + LCD_WIDTH / 2, LAPLIST_POS_Y, "TOP 5 LAPS", &Font8, BLACK, WHITE);
    LCD_DrawLine(0, LAPLIST_POS_Y - PADDING, LCD_WIDTH, LAPLIST_POS_Y - PADDING, WHITE, LINE_SOLID, DOT_PIXEL_1X1);
    LCD_DrawLine(LCD_WIDTH / 2, LAPLIST_POS_Y - PADDING, LCD_WIDTH / 2, LCD_HEIGHT, WHITE, LINE_SOLID, DOT_PIXEL_1X1);
}

void printLapLists()
{
    for (int i = 0; i < LAPLIST_SIZE; i++)
    {
        printLapTime(lapListLast[i], LAPLIST_POS_X, LAPLIST_POS_Y + 15 + i * 15, LAPLIST_FONT);
    }
    for (int i = 0; i < LAPLIST_SIZE; i++)
    {
        printLapTime(lapListTop[i], LCD_WIDTH / 2 + LAPLIST_POS_X, LAPLIST_POS_Y + 15 + i * 15, LAPLIST_FONT);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1 && dmaBusyFlag == true)
    {
        dmaBusyFlag = false;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == LAP_SAVE_Pin)
    {
        if (lapResetFlag == false && lapTimeCurrent.time > LAPTIME_MIN / 10)
        {
            lapTimeSaved = lapTimeCurrent;
            rtcReset();
            lapTimeCurrent.time = 0;
            lapTimeCurrent.count++;
        }
        else if (lapResetFlag == true)
        {
            lapResetFlag = false;
            rtcReset();
        }
    }

    else if (GPIO_Pin == LAP_RESET_Pin)
    {
        lapTimeCurrent.time = 0;
        lapResetFlag = true;
        printLapTime(lapTimeCurrent, CURRENT_LAPTIME_POS_X, CURRENT_LAPTIME_POS_Y, CURRENT_LAPTIME_FONT);
    }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_RTC_Init();

    /* USER CODE BEGIN 2 */
    LCD_Init(D2U_L2R);
    HAL_Delay(50);
    LCD_Clear(BLACK);
    printUI();
    printLapTime(lapTimeCurrent, CURRENT_LAPTIME_POS_X, CURRENT_LAPTIME_POS_Y, CURRENT_LAPTIME_FONT);
    printLapLists();
    LCD_Copy();
    HAL_TIM_Base_Start_IT(&htim2);
    rtcReset();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        if (!lapResetFlag)
        {
            lapTimeCurrent.time = rtcGetTime();
            printLapTime(lapTimeCurrent, CURRENT_LAPTIME_POS_X, CURRENT_LAPTIME_POS_Y, CURRENT_LAPTIME_FONT);
        }

        if (lapTimeSaved.time)
        {
            saveLapTime();
            printLapLists();
        }
        if (dmaBusyFlag == false)
            LCD_Copy();
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 100;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

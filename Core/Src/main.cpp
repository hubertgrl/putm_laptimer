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
#include "dma.h"
#include "spi.h"
#include "rtc.h"
#include "usart.h"

#include "stdio.h"
#include "laptimer_functions.h"
#include "laptimer_lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LAPTIME_MIN 5000

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

    unsigned int time10ms = ((time.Hours * 3600 + time.Minutes * 60 + time.Seconds) * 1000 + ((255 - time.SubSeconds) * 1000) / 256) / 10;

    return time10ms;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1 && lcdIsBusy())
    {
        lcdSetFree();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == LAP_GATE_1_Pin)
    {
        switch (lapMode)
        {
        case ONE_GATE_MODE:
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
            break;
        case TWO_GATE_MODE:
            if (lapResetFlag == true)
            {
                lapResetFlag = false;
                rtcReset();
            }
            break;
        default:
            break;
        }
    }
    else if (GPIO_Pin == LAP_GATE_2_Pin)
    {
        switch (lapMode)
        {
        case ONE_GATE_MODE:
            break;
        case TWO_GATE_MODE:
            if (lapResetFlag == false && lapTimeCurrent.time > LAPTIME_MIN / 10)
            {
                lapTimeSaved = lapTimeCurrent;
                lapResetFlag = true;
                rtcReset();
                lapTimeCurrent.time = 0;
                lapTimeCurrent.count++;
            }
            break;
        default:
            break;
        }
    }

    else if (GPIO_Pin == LAP_RESET_Pin)
    {
        lapTimeCurrent.time = 0;
        lapResetFlag = true;
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
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_RTC_Init();
    MX_USART3_UART_Init();

    /* USER CODE BEGIN 2 */
    char lapTimeSavedString[14];
    bool lapResetFlagOld = true;
    lcdInit();
    HAL_Delay(50);
    lcdClear();
    showUI();
    showMode();
    showLapTime(lapTimeCurrent, CURRENT_LAPTIME_POS_X, CURRENT_LAPTIME_POS_Y, CURRENT_LAPTIME_FONT);
    showLapLists();

    lcdCopy();
    rtcReset();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        if (!lapResetFlag)
            lapTimeCurrent.time = rtcGetTime();
        if (lapResetFlag != lapResetFlagOld)
        {
            lapMode = checkMode();
            showMode();
            lapResetFlagOld = lapResetFlag;
        }

        showLapTime(lapTimeCurrent, CURRENT_LAPTIME_POS_X, CURRENT_LAPTIME_POS_Y, CURRENT_LAPTIME_FONT);

        if (lapTimeSaved.time)
        {
            convertLapTime(lapTimeSaved, lapTimeSavedString, sizeof(lapTimeSavedString));
            sendLapTime(lapTimeSavedString, sizeof(lapTimeSavedString));
            saveLapTime();
            showLapLists();
        }
        if (!lcdIsBusy() && (lapTimeCurrent.time % 2 == 0))
            lcdCopy();
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
    RCC_OscInitStruct.PLL.PLLN = 80;
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
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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

/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Modified for Final Lab
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* USER CODE BEGIN Includes */
#include "sys.h"
#include "delay.h"  
#include "lcd.h"
#include "touch.h" 
#include "pic.h" 
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
// ????????
#define MAX_ALARMS 5

typedef struct {
    uint8_t enable; // 0: Disabled, 1: Enabled
    uint8_t hour;
    uint8_t minute;
} Alarm_t;

Alarm_t alarms[MAX_ALARMS]; // ????
uint8_t currentAlarmIndex = 0; // ???UI????????

// ???????
#define RX_BUFFER_SIZE 128
uint8_t UART_RxBuffer[RX_BUFFER_SIZE];
uint8_t UART_RxByte;
uint8_t UART_RxIndex = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void Parse_UART_Command(char *cmd);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
uint8_t touchflag = 0;
#define BEEP              	1
#define LED     						7

const uint16_t origX = 20;
const uint16_t origY = 100;

// UI ????
int dateYearInt, dateMonthInt, dateDayInt;
int timeHourInt, timeMinuteInt, timeSecondInt;

uint8_t dateYearStr[5];
uint8_t dateMonthStr[5];
uint8_t dateDayStr[5];

const uint16_t dateYearWidth = 50;
const uint16_t dateMonthWidth = 36;
const uint16_t dateDayWidth = 36;

uint8_t timeHourStr[5];
uint8_t timeMinuteStr[5];
uint8_t timeSecondStr[5];

const uint16_t timeHourWidth = 25;
const uint16_t timeMinuteWidth = 36;
const uint16_t timeSecondWidth = 36;

// ?? UI ???
uint8_t alarmIdxStr[5];   // ??????????
uint8_t alarmEnableStr[5];
uint8_t alarmHourStr[5];
uint8_t alarmMinuteStr[5];

const uint16_t alarmIdxWidth = 25;
const uint16_t alarmEnableWidth = 50;
const uint16_t alarmHourWidth = 25;
const uint16_t alarmMinuteWidth = 36;

#define MODE_CLOCK			0
#define MODE_SET_DATE		1
#define MODE_SET_TIME		2
#define MODE_SET_ALARM 	3
#define MODE_MAX        3

uint8_t modeInt = MODE_CLOCK;
char *modeStr[] = {
	"Clock    ",
	"SetDate  ",
	"SetTime  ", 
	"SetAlarm "
};

int focusInt = 0;
int focusMax = 0;
uint16_t *focusPrev = NULL;
uint16_t *focusCurr = NULL;

const uint16_t lineLabelWidth = 120;

// UI ??????
uint16_t dateFocus[][3] =  {
	{origX + lineLabelWidth,                                  origY+50+20, dateYearWidth},
	{origX + lineLabelWidth + dateYearWidth, 					  		  origY+50+20, dateMonthWidth},
	{origX + lineLabelWidth + dateYearWidth + dateMonthWidth, origY+50+20, dateDayWidth}
};

uint16_t timeFocus[][3] = {
	{origX + lineLabelWidth,                                  origY+100+20, timeHourWidth},
	{origX + lineLabelWidth + timeHourWidth,                  origY+100+20, timeMinuteWidth},
	{origX + lineLabelWidth + timeHourWidth + timeMinuteWidth,origY+100+20, timeSecondWidth}
};

// ????:0:Index, 1:Enable, 2:Hour, 3:Minute
uint16_t alarmFocus[][3] = {
    {origX + lineLabelWidth,                                    origY+150+20, alarmIdxWidth}, // Index
    {origX + lineLabelWidth + alarmIdxWidth,                    origY+150+20, alarmEnableWidth}, // Switch
	{origX + lineLabelWidth + alarmIdxWidth + alarmEnableWidth, origY+150+20, alarmHourWidth}, // Hour
	{origX + lineLabelWidth + alarmIdxWidth + alarmEnableWidth + alarmHourWidth,origY+150+20, alarmMinuteWidth} // Minute
};

uint8_t setTimeFlag = 0;
uint8_t setDateFlag = 0; // ????????

// ????????
int alarmTriggerCountdown = 0; // ?????,5? -> 0
uint8_t alarmActive = 0;       // ????????????

uint8_t rtcDateStr[100] = {0};
uint8_t rtcTimeStr[100] = {0};

void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);//??   
 	POINT_COLOR=BLUE;//??????? 
	LCD_ShowString(lcddev.width-24,0,200,16,16,"RST");
}

// ... (?????????,??????,???? gui_draw_hline ? lcd_draw_bline ?????,???????) ...
// ??????? gui_draw_hline, gui_fill_circle, my_abs, lcd_draw_bline, ctp_test ???
// ?????????????,???????????????????
// ????????,????????????????

void gui_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if(len==0)return;
	LCD_Fill(x0,y0,x0+len-1,y0,color);	
}
void gui_fill_circle(uint16_t x0,uint16_t y0,uint16_t r,uint16_t color)
{											  
	uint32_t i;
	uint32_t imax = ((uint32_t)r*707)/1000+1;
	uint32_t sqmax = (uint32_t)r*(uint32_t)r+(uint32_t)r/2;
	uint32_t x=r;
	gui_draw_hline(x0-r,y0,2*r,color);
	for (i=1;i<=imax;i++) 
	{
		if ((i*i+x*x)>sqmax)
		{
 			if (x>imax) 
			{
				gui_draw_hline (x0-i+1,y0+x,2*(i-1),color);
				gui_draw_hline (x0-i+1,y0-x,2*(i-1),color);
			}
			x--;
		}
		gui_draw_hline(x0-x,y0+i,2*x,color);
		gui_draw_hline(x0-x,y0-i,2*x,color);
	}
}  

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_FSMC_Init();
  MX_RTC_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  
	delay_init(168); 
 	LCD_Init(); 
	tp_dev.init();
	POINT_COLOR=BLUE;	 
    LCD_ShowString(30,40,210,24,24,"STM32F407 Alarm Clock");
 	Load_Drow_Dialog();	 

    // ????????
    HAL_UART_Receive_IT(&huart1, &UART_RxByte, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	
    // 1. ??? RTC ??:2025-01-01 00:00:00 WeekDay 3 (Wednesday)
	RTC_DateTypeDef date;
	date.Year = 25; // 2025 (RTC uses 2 digits)
	date.Month = 1;
	date.Date = 1;
	date.WeekDay = 3; 
	HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
	
	RTC_TimeTypeDef time;
	time.Hours = 0;
	time.Minutes = 0;
	time.Seconds = 0;
	HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
	
    // ???????
    for(int i=0; i<MAX_ALARMS; i++) {
        alarms[i].enable = 0;
        alarms[i].hour = 0;
        alarms[i].minute = 0;
    }

    // ???????????
    sprintf((char*)alarmIdxStr, "%d:", currentAlarmIndex + 1);
	sprintf((char*)alarmEnableStr, "%s", alarms[currentAlarmIndex].enable ? "On":"Off");
	sprintf((char*)alarmHourStr, "%02d", alarms[currentAlarmIndex].hour);
	sprintf((char*)alarmMinuteStr, ":%02d", alarms[currentAlarmIndex].minute);

	HAL_TIM_Base_Start_IT(&htim2);
	  	
	LCD_ShowString(origX, origY,     lineLabelWidth, 24, 24, "Mode    :");
	LCD_ShowString(origX, origY+50,  lineLabelWidth, 24, 24, "Date    :");
	LCD_ShowString(origX, origY+100, lineLabelWidth, 24, 24, "Time    :");
	LCD_ShowString(origX, origY+150, lineLabelWidth, 24, 24, "Alarm   :");
	LCD_ShowString(origX, origY+250, lineLabelWidth, 24, 24, "RTC Date:");
	LCD_ShowString(origX, origY+300, lineLabelWidth, 24, 24, "RTC Time:");
	
	char keyStr[5];
	for (int i = 0; i < 4; i++) {
		sprintf(keyStr, "Key%d", i+1);
		LCD_DrawRectangle(origX + i*120, origY+500, origX + i*120 + 80, origY+500+100);
		LCD_ShowString(origX + i*120 + 20, origY+500+40, 60, 24, 24, (uint8_t*)keyStr);
	}

	while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
		
        // ??????
		if (setTimeFlag) {
			RTC_TimeTypeDef timeSet;
			timeSet.Hours = timeHourInt;
			timeSet.Minutes = timeMinuteInt;
			timeSet.Seconds = timeSecondInt;
			HAL_RTC_SetTime(&hrtc, &timeSet, RTC_FORMAT_BIN);
			setTimeFlag = 0;
		}
        
        // ??????
        if (setDateFlag) {
            RTC_DateTypeDef dateSet;
            dateSet.Year = dateYearInt - 2000;
            dateSet.Month = dateMonthInt;
            dateSet.Date = dateDayInt;
            // ?????????,??????????,????????
            // ????????,???WeekDay????
            HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN); 
            dateSet.WeekDay = date.WeekDay; 
            HAL_RTC_SetDate(&hrtc, &dateSet, RTC_FORMAT_BIN);
            setDateFlag = 0;
        }
		
        // ??????
		if (focusPrev) {
			LCD_Fill(focusPrev[0], focusPrev[1], focusPrev[0]+focusPrev[2], focusPrev[1]+1, WHITE);
			focusPrev = NULL;
		}
		if (MODE_CLOCK != modeInt) {
			LCD_Fill(focusCurr[0], focusCurr[1], focusCurr[0]+focusCurr[2], focusCurr[1]+1, BLACK);
		}
		
		LCD_ShowString(origX + lineLabelWidth, origY, 200, 24, 24, (uint8_t*)modeStr[modeInt]);

		LCD_ShowString(origX + lineLabelWidth,                                  origY+50,  dateYearWidth,  24, 24, dateYearStr);
		LCD_ShowString(origX + lineLabelWidth + dateYearWidth,                  origY+50,  dateMonthWidth, 24, 24, dateMonthStr);
		LCD_ShowString(origX + lineLabelWidth + dateYearWidth + dateMonthWidth, origY+50,  dateDayWidth,   24, 24, dateDayStr);

		LCD_ShowString(origX + lineLabelWidth,                                  origY+100, timeHourWidth,  24, 24, timeHourStr);
		LCD_ShowString(origX + lineLabelWidth + timeHourWidth,                  origY+100, timeMinuteWidth,24, 24, timeMinuteStr);
		LCD_ShowString(origX + lineLabelWidth + timeHourWidth + timeMinuteWidth,origY+100, timeSecondWidth,24, 24, timeSecondStr);

        // ??????
        sprintf((char*)alarmIdxStr, "#%d", currentAlarmIndex + 1);
        sprintf((char*)alarmEnableStr, " %s", alarms[currentAlarmIndex].enable ? "On ":"Off");
        sprintf((char*)alarmHourStr, " %02d", alarms[currentAlarmIndex].hour);
        sprintf((char*)alarmMinuteStr, ":%02d", alarms[currentAlarmIndex].minute);

		LCD_ShowString(origX + lineLabelWidth,                                    origY+150, alarmIdxWidth,   24, 24, alarmIdxStr);
        LCD_ShowString(origX + lineLabelWidth + alarmIdxWidth,                    origY+150, alarmEnableWidth,24, 24, alarmEnableStr);
		LCD_ShowString(origX + lineLabelWidth + alarmIdxWidth + alarmEnableWidth, origY+150, alarmHourWidth,  24, 24, alarmHourStr);
		LCD_ShowString(origX + lineLabelWidth + alarmIdxWidth + alarmEnableWidth + alarmHourWidth,origY+150, alarmMinuteWidth,24, 24, alarmMinuteStr);
		
		LCD_ShowString(origX + lineLabelWidth, origY+250, 300, 24, 24, rtcDateStr);
		LCD_ShowString(origX + lineLabelWidth, origY+300, 300, 24, 24, rtcTimeStr);

		delay_ms(10); // ????????????
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
    // ... (?????????????)
    // ???????? SystemClock_Config ??
    // ??????????????
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */
int fputc(int ch, FILE *f)
{ 
  uint8_t tmp[1]={0};
	tmp[0] = (uint8_t)ch;
	HAL_UART_Transmit(&huart1,tmp,1,10);	
	return ch;
}

// ??????
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        if(UART_RxByte == '\n' || UART_RxByte == '\r') // ?????
        {
            UART_RxBuffer[UART_RxIndex] = 0; // ??????
            if(UART_RxIndex > 0)
            {
                Parse_UART_Command((char*)UART_RxBuffer); // ????
            }
            UART_RxIndex = 0; // ????
        }
        else
        {
            if(UART_RxIndex < RX_BUFFER_SIZE - 1)
            {
                UART_RxBuffer[UART_RxIndex++] = UART_RxByte;
            }
        }
        HAL_UART_Receive_IT(&huart1, &UART_RxByte, 1); // ??????
    }
}

// ??????
void Parse_UART_Command(char *cmd)
{
    int y, m, d, h, min, s, wd, idx;
    char subCmd[10];

    // ??: now 2025-01-02 13:02:45 4
    if(strncmp(cmd, "now", 3) == 0)
    {
        if(sscanf(cmd, "now %d-%d-%d %d:%d:%d %d", &y, &m, &d, &h, &min, &s, &wd) == 7)
        {
            RTC_DateTypeDef date;
            RTC_TimeTypeDef time;
            date.Year = (y > 2000) ? (y - 2000) : y;
            date.Month = m;
            date.Date = d;
            date.WeekDay = wd;
            
            time.Hours = h;
            time.Minutes = min;
            time.Seconds = s;
            
            HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
            HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
            printf("Set Time OK\r\n");
        }
    }
    // ??: alarm N ...
    else if(strncmp(cmd, "alarm", 5) == 0)
    {
        // ???? alarm 1 delete
        if(strstr(cmd, "delete"))
        {
            if(sscanf(cmd, "alarm %d delete", &idx) == 1)
            {
                if(idx >= 1 && idx <= MAX_ALARMS)
                {
                    alarms[idx-1].enable = 0;
                    printf("Alarm %d Deleted\r\n", idx);
                }
            }
        }
        // ???? alarm 1 06:30
        else
        {
            if(sscanf(cmd, "alarm %d %d:%d", &idx, &h, &min) == 3)
            {
                if(idx >= 1 && idx <= MAX_ALARMS)
                {
                    alarms[idx-1].enable = 1;
                    alarms[idx-1].hour = h;
                    alarms[idx-1].minute = min;
                    printf("Alarm %d Set to %02d:%02d\r\n", idx, h, min);
                }
            }
        }
    }
}

// ??????
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // ??????,??????,??????
	if (GPIO_Pin == KEY1_Pin) { // Key1: Mode Switch
		if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET) {
			printf("Key1: Mode Switch\r\n");
            
            // ?????????,????
			if (MODE_SET_TIME == modeInt) setTimeFlag = 1;
            if (MODE_SET_DATE == modeInt) setDateFlag = 1;
            // Alarm???????,???????

			modeInt++;
			if (MODE_MAX < modeInt) modeInt = 0;

			focusInt = 0;
			focusPrev = focusCurr;
			if (MODE_SET_DATE == modeInt) {
				focusCurr = dateFocus[0];
				focusMax = 3; // Year, Month, Day
			}
			else if (MODE_SET_TIME == modeInt) {
				focusCurr = timeFocus[0];
				focusMax = 3; // Hour, Min, Sec
			}
			else if (MODE_SET_ALARM == modeInt) {
				focusCurr = alarmFocus[0];
				focusMax = 4; // Index, Enable, Hour, Minute
			}
		}
	}
	else if (GPIO_Pin == KEY2_Pin) // Key2: Focus Switch (Tab)
	{
		if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET) {
			printf("Key2: Tab\r\n");
			if (MODE_CLOCK != modeInt) {
				focusPrev = focusCurr;
				focusInt++;
				if (focusMax == focusInt) {
					focusInt = 0;
				}
				if (MODE_SET_DATE == modeInt) {
					focusCurr = dateFocus[focusInt];
				}
				else if (MODE_SET_TIME == modeInt) {
					focusCurr = timeFocus[focusInt];
				}
				else if (MODE_SET_ALARM == modeInt) {
					focusCurr = alarmFocus[focusInt];
				}
			}
		}
	}
	else if (GPIO_Pin == KEY3_Pin) // Key3: Value Dec (-)
	{
		if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET) {
			if (MODE_SET_DATE == modeInt) {
                if (0 == focusInt && dateYearInt > 2000) dateYearInt--;
                if (1 == focusInt && dateMonthInt > 1) dateMonthInt--;
                if (2 == focusInt && dateDayInt > 1) dateDayInt--;
                
                sprintf((char*)dateYearStr, "%d", dateYearInt);
                sprintf((char*)dateMonthStr, "-%02d", dateMonthInt);
                sprintf((char*)dateDayStr, "-%02d", dateDayInt);
			}
			if (MODE_SET_TIME == modeInt) {
				if (0 == focusInt && 0 < timeHourInt) timeHourInt--;
				if (1 == focusInt && 0 < timeMinuteInt) timeMinuteInt--;
				if (2 == focusInt && 0 < timeSecondInt) timeSecondInt--;
				sprintf((char*)timeHourStr, "%02d", timeHourInt);
				sprintf((char*)timeMinuteStr, ":%02d", timeMinuteInt);
				sprintf((char*)timeSecondStr, ":%02d", timeSecondInt);
			}
            if (MODE_SET_ALARM == modeInt) {
                // Focus: 0=Idx, 1=Enable, 2=Hour, 3=Min
                if (0 == focusInt) { // Switch Alarm Index
                    if (currentAlarmIndex > 0) currentAlarmIndex--;
                }
                else if (1 == focusInt) { // Toggle Enable
                    alarms[currentAlarmIndex].enable = !alarms[currentAlarmIndex].enable;
                }
                else if (2 == focusInt && alarms[currentAlarmIndex].hour > 0) {
                    alarms[currentAlarmIndex].hour--;
                }
                else if (3 == focusInt && alarms[currentAlarmIndex].minute > 0) {
                    alarms[currentAlarmIndex].minute--;
                }
            }
		}
	}
	else if (GPIO_Pin == KEY4_Pin) // Key4: Value Inc (+)
	{
		if (HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == GPIO_PIN_SET) {
			if (MODE_SET_DATE == modeInt) {
                if (0 == focusInt && dateYearInt < 2099) dateYearInt++;
                if (1 == focusInt && dateMonthInt < 12) dateMonthInt++;
                if (2 == focusInt && dateDayInt < 31) dateDayInt++;
                
                sprintf((char*)dateYearStr, "%d", dateYearInt);
                sprintf((char*)dateMonthStr, "-%02d", dateMonthInt);
                sprintf((char*)dateDayStr, "-%02d", dateDayInt);
			}
			if (MODE_SET_TIME == modeInt) {
				if (0 == focusInt && 23 > timeHourInt) timeHourInt++;
				if (1 == focusInt && 59 > timeMinuteInt) timeMinuteInt++;
				if (2 == focusInt && 59 > timeSecondInt) timeSecondInt++;
				sprintf((char*)timeHourStr, "%02d", timeHourInt);
				sprintf((char*)timeMinuteStr, ":%02d", timeMinuteInt);
				sprintf((char*)timeSecondStr, ":%02d", timeSecondInt);
			}
			if (MODE_SET_ALARM == modeInt) {
                if (0 == focusInt) { // Switch Alarm Index
                    if (currentAlarmIndex < MAX_ALARMS - 1) currentAlarmIndex++;
                }
                else if (1 == focusInt) {
                     alarms[currentAlarmIndex].enable = !alarms[currentAlarmIndex].enable;
                }
                else if (2 == focusInt && alarms[currentAlarmIndex].hour < 23) {
                    alarms[currentAlarmIndex].hour++;
                }
                else if (3 == focusInt && alarms[currentAlarmIndex].minute < 59) {
                    alarms[currentAlarmIndex].minute++;
                }
			}
		}
	}
}

// Systick ??:???????????????
void HAL_SYSTICK_Callback(void)
{
	static int tickCnt = 0;
	//static int secondCnt = 0;
	tickCnt++;
	if (tickCnt == 1000) {
		tickCnt = 0;
		//secondCnt++;
		
		RTC_TimeTypeDef time;
		HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
		RTC_DateTypeDef date;
		HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
		
        // ??????
        if(time.Seconds == 0) // ???????0???
        {
            for(int i=0; i<MAX_ALARMS; i++) {
                if(alarms[i].enable && 
                   alarms[i].hour == time.Hours && 
                   alarms[i].minute == time.Minutes) 
                {
                    alarmTriggerCountdown = 5; // ??5????
                    alarmActive = 1;
                    break; // ????????????
                }
            }
        }

		sprintf((char*)rtcDateStr, "20%02d-%02d-%02d Weekday(%d)", date.Year, date.Month, date.Date, date.WeekDay);
		sprintf((char*)rtcTimeStr, "%02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);

		if (MODE_CLOCK == modeInt) {
            // ?Clock????????????,??Main Loop??
			sprintf((char*)dateYearStr, "20%02d", date.Year);
			sprintf((char*)dateMonthStr, "-%02d", date.Month);
			sprintf((char*)dateDayStr, "-%02d", date.Date);
            // ???int??,?????????????
            dateYearInt = 2000 + date.Year;
            dateMonthInt = date.Month;
            dateDayInt = date.Date;
			
			sprintf((char*)timeHourStr, "%02d", time.Hours);
			sprintf((char*)timeMinuteStr, ":%02d", time.Minutes);
			sprintf((char*)timeSecondStr, ":%02d", time.Seconds);
            timeHourInt = time.Hours;
            timeMinuteInt = time.Minutes;
            timeSecondInt = time.Seconds;
		}
	}
}

// ?????:??????????/LED??
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	static int tim2Cnt = 0;
	if (htim->Instance == TIM2) {
		tim2Cnt++;
        // 500ms ????,????
		if (tim2Cnt % 500 == 0) {
            if (alarmTriggerCountdown > 0) {
                // ????:??BEEP?LED
                HAL_GPIO_TogglePin(BEEP_GPIO_Port, BEEP_Pin);
                // ??LED1???PF9 (DS0),??????????LED??
                // ???FSMC???,???DS0(PF9), DS1(PF10)
                // ????LED1_GPIO_Port/Pin (???gpio.h????????GPIOF/GPIO_PIN_9)
                // ????,??????,?????main.h/gpio.h??LED??
                // HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9); 
            } else {
                // ?????:?????
                HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET); // ??????,????,????????
                alarmActive = 0;
            }
        }
        
		if (tim2Cnt >= 1000) { // 1?
			tim2Cnt = 0;
			if (alarmTriggerCountdown > 0) {
				alarmTriggerCountdown--;
			}
		}
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
}
#endif
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
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fifo.h"
#include "uart_dma.h"
#include "MY_Temper.h"
#include "MY_Step.h"
#include "MY_CO2.h"
#include "app_config.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */




/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */




/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

char rx_buffer[CMD_BUFFER_SIZE]={0};                 
char command_buffer[CMD_BUFFER_SIZE]={0};             
uint32_t ReceiveLen=0;                              
uint8_t command_received = 0;                        
char result_code[CMD_BUFFER_SIZE]={0};               


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*
 * 函数: Processcommand
 * 功能: 从 USART1 接收 FIFO 中读取上位机命令, 以 @ 作为一帧命令结束符。
 * 硬件对应:
 *  - D0: 内部培养液温控器, USART3, 目标 37.0 摄氏度。
 *  - D1: 外部观察环境温控器, USART2, 目标 30.0 摄氏度。
 *  - D2: 四路灌流泵/步进电机。
 *  - D3: CO2 传感器和浓度控制。
 */
void Processcommand(void)
{
    static char cmd_buf[CMD_BUFFER_SIZE];
    static uint16_t cmd_len = 0;
    uint8_t byte;

    while (!fifo_s_is_empty(&uart1_rx_fifo))
    {
        fifo_s_get(&uart1_rx_fifo, &byte);

        /* 命令过长时丢弃当前帧, 防止缓冲区溢出。 */
        if (cmd_len >= (sizeof(cmd_buf) - 1U))
        {
            cmd_len = 0;
        }

        cmd_buf[cmd_len++] = byte;

        if (byte == '@')
        {
            cmd_buf[cmd_len] = '\0';

            /* 按 D0/D1/D2/D3 前缀分发到对应模块。 */
            if (strncmp((char*)cmd_buf, "D0", 2) == 0)
            {
                /* D0: 内部温控器, 通过 USART3 转发。 */
                memmove(cmd_buf, cmd_buf + 2, strlen((char*)cmd_buf + 2) + 1);
                ProcessD0Command(cmd_buf);
            }
            else if (strncmp((char*)cmd_buf, "D1", 2) == 0)
            {
                /* D1: 外部温控器, 通过 USART2 转发。 */
                memmove(cmd_buf, cmd_buf + 2, strlen((char*)cmd_buf + 2) + 1);
                ProcessD1TempCommand(cmd_buf);
            }
            else if(strncmp((char*)cmd_buf, "D2", 2) == 0)
            {
                /* D2: 四路灌流泵/步进电机模块。 */
                memmove(cmd_buf, cmd_buf + 2, strlen((char*)cmd_buf + 2) + 1);
                ProcessD2Command(cmd_buf);
            }
            else if(strncmp((char*)cmd_buf, "D3", 2) == 0)
            {
                /* D3: CO2 模块。 */
                memmove(cmd_buf, cmd_buf + 2, strlen((char*)cmd_buf + 2) + 1);
                ProcessD3Command(cmd_buf);
            }
            else
            {
                const char *result_code = "InvalidCommand\r\n";
                fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
                send_data_from_tx_fifo();
            }

            cmd_len = 0;
        }
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
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_USART3_UART_Init();
  MX_UART4_Init();
  MX_IWDG_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */






  // StepperMotor_Init(100,8);
  // HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);

  HAL_Delay(1000);
  UART_System_Init();
  AppConfig_Init();
  HAL_Delay(1000);
  StepperMotor_Init();
  // HAL_Delay(1000);
  // TemperatureSensor_Init();
  HAL_Delay(1000);
  CO2Module_Init();









  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


  Processcommand();
  CO2_ControlTask();
  HAL_IWDG_Refresh(&hiwdg);

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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

#ifdef  USE_FULL_ASSERT
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

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <string.h>
#include <stdint.h>
#include "MY_TF.h"
#include "MY_Temper.h"
#include "MY_Step.h"
#include "MY_CO2.h"
#include "uart_dma.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */


/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define EN_12V_Pin GPIO_PIN_2
#define EN_12V_GPIO_Port GPIOE
#define MYTF_BY_Pin GPIO_PIN_4
#define MYTF_BY_GPIO_Port GPIOA
#define O1_IN_Pin GPIO_PIN_13
#define O1_IN_GPIO_Port GPIOD
#define O2_IN_Pin GPIO_PIN_14
#define O2_IN_GPIO_Port GPIOD
#define TIM4_DIR_Pin GPIO_PIN_0
#define TIM4_DIR_GPIO_Port GPIOD
#define TIM4_MS2_Pin GPIO_PIN_1
#define TIM4_MS2_GPIO_Port GPIOD
#define TIM4_MS1_Pin GPIO_PIN_2
#define TIM4_MS1_GPIO_Port GPIOD
#define TIM4_EN_Pin GPIO_PIN_3
#define TIM4_EN_GPIO_Port GPIOD
#define TIM3_DIR_Pin GPIO_PIN_4
#define TIM3_DIR_GPIO_Port GPIOD
#define TIM3_MS2_Pin GPIO_PIN_5
#define TIM3_MS2_GPIO_Port GPIOD
#define TIM3_MS1_Pin GPIO_PIN_6
#define TIM3_MS1_GPIO_Port GPIOD
#define TIM3_EN_Pin GPIO_PIN_7
#define TIM3_EN_GPIO_Port GPIOD
#define TIM2_DIR_Pin GPIO_PIN_3
#define TIM2_DIR_GPIO_Port GPIOB
#define TIM2_MS2_Pin GPIO_PIN_4
#define TIM2_MS2_GPIO_Port GPIOB
#define TIM2_MS1_Pin GPIO_PIN_5
#define TIM2_MS1_GPIO_Port GPIOB
#define TIM2_EN_Pin GPIO_PIN_6
#define TIM2_EN_GPIO_Port GPIOB
#define TIM1_DIR_Pin GPIO_PIN_8
#define TIM1_DIR_GPIO_Port GPIOB
#define TIM1_MS2_Pin GPIO_PIN_9
#define TIM1_MS2_GPIO_Port GPIOB
#define TIM1_MS1_Pin GPIO_PIN_0
#define TIM1_MS1_GPIO_Port GPIOE
#define TIM1_EN_Pin GPIO_PIN_1
#define TIM1_EN_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */


#define CMD_BUFFER_SIZE  218       
#define MODULENAME_SIZE  32        
#define PARAMNAME_SIZE   32       
#define PARAMVALUE_SIZE  32  


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

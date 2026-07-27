/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, EN_12V_Pin|TIM1_MS1_Pin|TIM1_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MYTF_BY_GPIO_Port, MYTF_BY_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, O1_IN_Pin|O2_IN_Pin|TIM4_DIR_Pin|TIM4_MS2_Pin
                          |TIM4_MS1_Pin|TIM4_EN_Pin|TIM3_DIR_Pin|TIM3_MS2_Pin
                          |TIM3_MS1_Pin|TIM3_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TIM2_DIR_Pin|TIM2_MS2_Pin|TIM2_MS1_Pin|TIM2_EN_Pin
                          |TIM1_DIR_Pin|TIM1_MS2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : EN_12V_Pin */
  GPIO_InitStruct.Pin = EN_12V_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EN_12V_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MYTF_BY_Pin */
  GPIO_InitStruct.Pin = MYTF_BY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MYTF_BY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : O1_IN_Pin O2_IN_Pin */
  GPIO_InitStruct.Pin = O1_IN_Pin|O2_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : TIM4_DIR_Pin TIM4_MS2_Pin TIM4_MS1_Pin TIM4_EN_Pin
                           TIM3_DIR_Pin TIM3_MS2_Pin TIM3_MS1_Pin TIM3_EN_Pin */
  GPIO_InitStruct.Pin = TIM4_DIR_Pin|TIM4_MS2_Pin|TIM4_MS1_Pin|TIM4_EN_Pin
                          |TIM3_DIR_Pin|TIM3_MS2_Pin|TIM3_MS1_Pin|TIM3_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : TIM2_DIR_Pin TIM2_MS2_Pin TIM2_MS1_Pin TIM2_EN_Pin
                           TIM1_DIR_Pin TIM1_MS2_Pin */
  GPIO_InitStruct.Pin = TIM2_DIR_Pin|TIM2_MS2_Pin|TIM2_MS1_Pin|TIM2_EN_Pin
                          |TIM1_DIR_Pin|TIM1_MS2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : TIM1_MS1_Pin TIM1_EN_Pin */
  GPIO_InitStruct.Pin = TIM1_MS1_Pin|TIM1_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

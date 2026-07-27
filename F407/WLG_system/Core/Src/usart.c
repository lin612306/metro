/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */

// #include <string.h>
// #include "stm32f4xx_hal_dma.h" 

// #define UART_RX_BUF_LEN 260
// #define UART_TX_BUF_LEN 512
// #define UART_RING_BUF_LEN 2048
// #define FRAME_END_CHAR '@' 

// static UART_Device *g_uart_devices[] = {&g_uart1_dev, &g_uart2_dev, &g_uart3_dev};

// typedef struct {
//     uint8_t *buffer;
//     uint16_t head;
//     uint16_t tail;
//     uint16_t size;
//     uint16_t count;
// } RingBuffer_t;


// typedef struct UART_Data {
//     UART_HandleTypeDef *huart;
//     uint8_t rx_buf[UART_RX_BUF_LEN];
//     uint8_t tx_buf[UART_TX_BUF_LEN];
//     RingBuffer_t rx_ring;
//     uint8_t rx_ring_buf[UART_RING_BUF_LEN];
//     volatile uint8_t tx_busy;
//     volatile uint16_t rx_old_pos;
// } UART_Data, *PUART_Data;



// /* Ring buffer implementations */
// static void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buf, uint16_t size)
// {
//     rb->buffer = buf;
//     rb->head = 0;
//     rb->tail = 0;
//     rb->size = size;
//     rb->count = 0;
// }

// static uint16_t RingBuffer_Write(RingBuffer_t *rb, uint8_t *data, uint16_t len)
// {
//     uint16_t written = 0;
    
//     if (len == 0) return 0;

//     for (uint16_t i = 0; i < len; i++) 
//     {
//         if (rb->count >= rb->size) 
//         {
//             rb->buffer[rb->head] = data[i];
//             rb->head = (rb->head + 1) % rb->size;
//             rb->tail = (rb->tail + 1) % rb->size;
//         }
//         else
//         {
//             rb->buffer[rb->head] = data[i];
//             rb->head = (rb->head + 1) % rb->size;
//             rb->count++;
//         }
//         written++;
//     }
    
//     return written;
// }

// static uint16_t RingBuffer_Read(RingBuffer_t *rb, uint8_t *data, uint16_t len)
// {
//     uint16_t read = 0;

//     if (len == 0) 
//     return 0;
    
//     for (uint16_t i = 0; i < len; i++)
//     {
//         if (rb->count == 0) 
//         {
//             break; // Buffer empty
//         }
        
//         data[i] = rb->buffer[rb->tail];
//         rb->tail = (rb->tail + 1) % rb->size;
//         rb->count--;
//         read++;
//     }
    
//     return read;
// }

// static uint16_t RingBuffer_GetCount(RingBuffer_t *rb)
// {
//     return rb->count;
// }

// static void RingBuffer_Clear(RingBuffer_t *rb)
// {
//     rb->head = 0;
//     rb->tail = 0;
//     rb->count = 0;
// }


// /* UART data instances */
// static UART_Data g_uart1_data = {
//     .huart = &huart1,
//     .tx_busy = 0,
//     .rx_old_pos = 0,
// };

// static UART_Data g_uart2_data = {
//     .huart = &huart2,
//     .tx_busy = 0,
//     .rx_old_pos = 0,
// };

// static UART_Data g_uart3_data = {
//     .huart = &huart3,
//     .tx_busy = 0,
//     .rx_old_pos = 0,
// };




/* USER CODE END 0 */

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_uart4_rx;
DMA_HandleTypeDef hdma_uart4_tx;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

/* UART4 init function */
void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 9600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}
/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 57600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspInit 0 */

  /* USER CODE END UART4_MspInit 0 */
    /* UART4 clock enable */
    __HAL_RCC_UART4_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**UART4 GPIO Configuration
    PA1     ------> UART4_RX
    PC10     ------> UART4_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* UART4 DMA Init */
    /* UART4_RX Init */
    hdma_uart4_rx.Instance = DMA1_Stream2;
    hdma_uart4_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_uart4_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart4_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart4_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart4_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart4_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart4_rx.Init.Mode = DMA_CIRCULAR;
    hdma_uart4_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_uart4_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart4_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_uart4_rx);

    /* UART4_TX Init */
    hdma_uart4_tx.Instance = DMA1_Stream4;
    hdma_uart4_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_uart4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_uart4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart4_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart4_tx.Init.Mode = DMA_NORMAL;
    hdma_uart4_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_uart4_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart4_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_uart4_tx);

    /* UART4 interrupt Init */
    HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspInit 1 */

  /* USER CODE END UART4_MspInit 1 */
  }
  else if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA2_Stream2;
    hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Stream5;
    hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART3 DMA Init */
    /* USART3_RX Init */
    hdma_usart3_rx.Instance = DMA1_Stream1;
    hdma_usart3_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart3_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart3_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart3_rx);

    /* USART3_TX Init */
    hdma_usart3_tx.Instance = DMA1_Stream3;
    hdma_usart3_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart3_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart3_tx.Init.Mode = DMA_NORMAL;
    hdma_usart3_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart3_tx);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspDeInit 0 */

  /* USER CODE END UART4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART4_CLK_DISABLE();

    /**UART4 GPIO Configuration
    PA1     ------> UART4_RX
    PC10     ------> UART4_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10);

    /* UART4 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* UART4 interrupt Deinit */
    HAL_NVIC_DisableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspDeInit 1 */

  /* USER CODE END UART4_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);

    /* USART3 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */



// static int UART_Rx_Start(PUART_Device pDev, int baud, char parity, int data_bit, int stop_bit)
// {
//     // Check device pointer
//     if (pDev == NULL)
//     {
//         return -1; // Invalid device pointer
//     }
    
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
    
//     // Check pdata validity
//     if (pdata == NULL || pdata->huart == NULL)
//     {
//         return -1; // Invalid data structure or UART handle
//     }
    
//     // Validate parameters
//     if (baud <= 0 || (parity != 'N' && parity != 'E' && parity != 'O') ||
//         (data_bit != 8 && data_bit != 9) || (stop_bit != 1 && stop_bit != 2)) {
//         return -2; // Invalid params
//     }
    
//     // Apply dynamic configuration
//     pdata->huart->Init.BaudRate = baud;
//     pdata->huart->Init.Parity = (parity == 'E') ? UART_PARITY_EVEN : (parity == 'O') ? UART_PARITY_ODD : UART_PARITY_NONE;
//     pdata->huart->Init.WordLength = (data_bit == 9) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
//     pdata->huart->Init.StopBits = (stop_bit == 2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
//     if (HAL_UART_Init(pdata->huart) != HAL_OK) 
//     return -1;

//     /* Initialize ring buffer */
//     RingBuffer_Init(&pdata->rx_ring, pdata->rx_ring_buf, UART_RING_BUF_LEN);
    
//     /* Reset status */
//     pdata->tx_busy = 0;
//     pdata->rx_old_pos = 0;
    
//     /* Start DMA reception */
//     if (HAL_UARTEx_ReceiveToIdle_DMA(pdata->huart, pdata->rx_buf, UART_RX_BUF_LEN) != HAL_OK)
//     {
//         return -1;
//     }
    
//     /* Disable half-transfer interrupt */
//     if (pdata->huart->hdmarx != NULL) // Check if hdmarx is valid
//     {
//         __HAL_DMA_DISABLE_IT(pdata->huart->hdmarx, DMA_IT_HT);
//     }
    
//     return 0;
// }


// static int UART_GetData(PUART_Device pDev, uint8_t *pData, int timeout)
// {
//     if (pDev == NULL || pData == NULL) 
//     return -1;
    
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
//     if (pdata == NULL) 
//     return -1;

//     uint32_t start_time = HAL_GetTick();
    
//     /* Wait for data availability, with timeout */
//     while (RingBuffer_GetCount(&pdata->rx_ring) == 0)
//     {
//         if (timeout >= 0 && (HAL_GetTick() - start_time) >= (uint32_t)timeout)
//         {
//             return -1; /* Timeout */
//         }
//     }
    
//     /* Read one byte from ring buffer */
//     if (RingBuffer_Read(&pdata->rx_ring, pData, 1) == 1)
//     {
//         return 0; /* Success */
//     }
    
//     return -1; /* Failure */
// }



// static int UART_Send(PUART_Device pDev, uint8_t *datas, uint16_t len, int timeout)
// {
//     if (pDev == NULL)
//     {
//         return -1;
//     }
    
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
//     uint32_t start_time = HAL_GetTick();
    
//     if (pdata == NULL || datas == NULL || len == 0)
//     {
//         return -1;
//     }
    
//     if (pdata->huart == NULL)
//     {
//         return -1;
//     }
    
//     /* Wait for previous transmission to complete */
//     while (pdata->tx_busy)
//     {
//         if (timeout >= 0 && (HAL_GetTick() - start_time) >= (uint32_t)timeout)
//         {
//             return -1; /* Timeout */
//         }
//     }
    
//     /* Check data length */
//     if (len > UART_TX_BUF_LEN)
//     {
//         return -1;
//     }
    
//     /* Copy data to transmit buffer */
//     memcpy(pdata->tx_buf, datas, len);
    
//     /* Set busy flag */
//     pdata->tx_busy = 1;
    
//     /* Start DMA transmission */
//     if (HAL_UART_Transmit_DMA(pdata->huart, pdata->tx_buf, len) != HAL_OK)
//     {
//         pdata->tx_busy = 0;
//         return -1;
//     }
    
//     /* Wait for transmission complete */
//     start_time = HAL_GetTick();
//     while (pdata->tx_busy)
//     {
//         if (timeout >= 0 && (HAL_GetTick() - start_time) >= (uint32_t)timeout)
//         {
//             /* Stop transmission on timeout */
//             HAL_UART_AbortTransmit(pdata->huart);
//             pdata->tx_busy = 0;
//             return -1;
//         }
//     }
    
//     return 0;
// }




// static int UART_Flush(PUART_Device pDev)
// {
//     // Check device pointer
//     if (pDev == NULL)
//     {
//         return -1; // Invalid device pointer
//     }
    
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
    
//     // Check pdata validity
//     if (pdata == NULL || pdata->huart == NULL)
//     {
//         return -1; // Invalid data structure or UART handle
//     }
    
//     uint16_t count = RingBuffer_GetCount(&pdata->rx_ring);
    
//     // Stop DMA reception for safe access
//     HAL_UART_DMAStop(pdata->huart);
    
//     // Clear ring buffer
//     RingBuffer_Clear(&pdata->rx_ring);
    
//     // Reset position
//     pdata->rx_old_pos = 0;
    
//     // Clear DMA reception buffer
//     memset(pdata->rx_buf, 0, UART_RX_BUF_LEN);
    
//     // Restart DMA reception
//     if (HAL_UARTEx_ReceiveToIdle_DMA(pdata->huart, pdata->rx_buf, UART_RX_BUF_LEN) == HAL_OK)
//     {
//         if (pdata->huart->hdmarx != NULL) // Check if hdmarx is valid
//         {
//             __HAL_DMA_DISABLE_IT(pdata->huart->hdmarx, DMA_IT_HT);
//         }
//     }
    
//     return count;
// }




// /* Public API Functions */
// /* Public API Functions */
// UART_Device *GetUARTDevice(char *name)
// {
//     int i = 0;
    
//     /* Check if name parameter is NULL */
//     if (name == NULL) 
//     {
//         return NULL;
//     }
    
//     /* Iterate through available UART devices */
//     for (i = 0; i < sizeof(g_uart_devices)/sizeof(g_uart_devices[0]); i++) 
//     {
//         if (g_uart_devices[i] != NULL && !strcmp(name, g_uart_devices[i]->name)) 
//         {
//             return g_uart_devices[i];
//         }
//     }
    
//     return NULL;
// }




// /* Non-blocking send function */
// int UART_SendAsync(PUART_Device pDev, uint8_t *datas, uint16_t len)
// {
//     if (pDev == NULL || datas == NULL || len == 0) {
//         return -1;
//     }
    
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
    
//     if (pdata == NULL || pdata->huart == NULL) {
//         return -1;
//     }
    
//     if (pdata->tx_busy) {
//         return -1; /* Transmission in progress */
//     }
    
//     if (len > UART_TX_BUF_LEN) {
//         return -1;
//     }
    
//     memcpy(pdata->tx_buf, datas, len);
//     pdata->tx_busy = 1;
    
//     if (HAL_UART_Transmit_DMA(pdata->huart, pdata->tx_buf, len) != HAL_OK) {
//         pdata->tx_busy = 0;
//         return -1;
//     }
    
//     return 0;
// }




// /* Check if transmission is complete */
// int UART_IsTxComplete(PUART_Device pDev)
// {
//     /* Check parameters validity */
//     if (pDev == NULL || pDev->priv_data == NULL) {
//         return -1; /* Invalid parameters */
//     }
    
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
//     return !pdata->tx_busy ? 0 : -1;
// }



// /* Get number of bytes available in receive buffer */
// int UART_GetRxCount(PUART_Device pDev)
// {
//     /* Check parameters validity */
//     if (pDev == NULL || pDev->priv_data == NULL) {
//         return -1; /* Invalid parameters */
//     }
    
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
//     return RingBuffer_GetCount(&pdata->rx_ring);
// }



// /* Read multiple bytes from receive buffer */
// int UART_ReadData(PUART_Device pDev, uint8_t *buffer, uint16_t len)
// {
//     /* Check parameters validity */
//     if (pDev == NULL || buffer == NULL || len == 0) {
//         return -1; /* Invalid parameters */
//     }
    
//     if (pDev->priv_data == NULL) {
//         return -1; /* Invalid device data */
//     }
    
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
//     return RingBuffer_Read(&pdata->rx_ring, buffer, len);
// }




// /* Peek at data in receive buffer without removing it */
// int UART_PeekData(PUART_Device pDev, uint8_t *buffer, uint16_t len)
// {
//     /* Check parameters validity */
//     if (pDev == NULL || buffer == NULL || len == 0) {
//         return -1; /* Invalid parameters */
//     }
    
//     if (pDev->priv_data == NULL) {
//         return -1; /* Invalid device data */
//     }
//     PUART_Data pdata = (PUART_Data)pDev->priv_data;
//     RingBuffer_t *rb = &pdata->rx_ring;
//     uint16_t available = RingBuffer_GetCount(rb);
//     uint16_t to_read = (len < available) ? len : available;
//     uint16_t read_count = 0;
//     uint16_t temp_tail = rb->tail;
    
//     /* Read data without moving tail pointer */
//     for (uint16_t i = 0; i < to_read; i++)
//     {
//         buffer[i] = rb->buffer[temp_tail];
//         temp_tail = (temp_tail + 1) % rb->size;
//         read_count++;
//     }
    
//     return read_count;
// }




// /* HAL Callback Functions */
// void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
// {
//     if (huart == NULL)
//     {
//         return;
//     }
    
//     PUART_Data pdata = NULL;
    
//     if (huart == &huart1)
//     {
//         pdata = &g_uart1_data;
//     }
//     else if (huart == &huart2)
//     {
//         pdata = &g_uart2_data;
//     }
//     else if (huart == &huart3)
//     {
//         pdata = &g_uart3_data;
//     }
    
//     if (pdata != NULL)
//     {
//         pdata->tx_busy = 0;
//     }
// }


// void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
// {
//     PUART_Data pdata = NULL;
    
//     if (huart == &huart1)
//     {
//         pdata = &g_uart1_data;
//     }
//     else if (huart == &huart2)
//     {
//         pdata = &g_uart2_data;
//     }
//     else if (huart == &huart3)
//     {
//         pdata = &g_uart3_data;
//     }
    
//     if (pdata != NULL && Size <= UART_RX_BUF_LEN)
//     {
//         /* Write newly received data to ring buffer */
//         uint16_t new_data_len = 0;
//         if (Size > pdata->rx_old_pos)
//         {
//             new_data_len = Size - pdata->rx_old_pos;
//             RingBuffer_Write(&pdata->rx_ring, 
//                             &pdata->rx_buf[pdata->rx_old_pos], 
//                             new_data_len);
//         }
//         else if (Size < pdata->rx_old_pos)
//         {
//             /* Handle buffer wrap-around */
//             RingBuffer_Write(&pdata->rx_ring, 
//                             &pdata->rx_buf[pdata->rx_old_pos], 
//                             UART_RX_BUF_LEN - pdata->rx_old_pos);
//             if (Size > 0)
//             {
//                 RingBuffer_Write(&pdata->rx_ring, 
//                                 &pdata->rx_buf[0], 
//                                 Size);
//             }
//         }
        
//         pdata->rx_old_pos = Size;
//     }

//     // Restart DMA for continuous reception
//     if (pdata != NULL && pdata->huart != NULL)
//     {
//         // ֹͣ��ǰDMA
//         HAL_UART_DMAStop(pdata->huart);

//         // ����DMA
//         if (HAL_UARTEx_ReceiveToIdle_DMA(pdata->huart, pdata->rx_buf, UART_RX_BUF_LEN) == HAL_OK)
//         {
//             if (pdata->huart->hdmarx != NULL) 
//             {
//                 __HAL_DMA_DISABLE_IT(pdata->huart->hdmarx, DMA_IT_HT);
//             }
//         }
//     }
// }



// void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
// {
//     // Check if huart is valid
//     if (huart == NULL)
//     {
//         return; // Invalid handle, return directly
//     }
    
//     PUART_Data pdata = NULL;
    
//     // Device identification
//     if (huart == &huart1)
//     {
//         pdata = &g_uart1_data;
//     }
//     else if (huart == &huart2)
//     {
//         pdata = &g_uart2_data;
//     }
//     else if (huart == &huart3)
//     {
//         pdata = &g_uart3_data;
//     }
    
//     if (pdata != NULL && pdata->huart != NULL)
//     {
//         /* ������д����־ */
//         __HAL_UART_CLEAR_OREFLAG(pdata->huart);
//         __HAL_UART_CLEAR_NEFLAG(pdata->huart);
//         __HAL_UART_CLEAR_FEFLAG(pdata->huart);
//         __HAL_UART_CLEAR_PEFLAG(pdata->huart);
        
//         /* ֹͣDMA */
//         HAL_UART_DMAStop(pdata->huart);
        
//         /* ����״̬ */
//         pdata->rx_old_pos = 0;
//         pdata->tx_busy = 0;  // ?? ��Ҫ���������æ��־
        
//         /* ����DMA���� */
//         if (pdata->huart->hdmarx != NULL)
//         {
//             if (HAL_UARTEx_ReceiveToIdle_DMA(pdata->huart, pdata->rx_buf, UART_RX_BUF_LEN) == HAL_OK)
//             {
//                 __HAL_DMA_DISABLE_IT(pdata->huart->hdmarx, DMA_IT_HT);
//             }
//         }
//     }
// }



// UART_Device g_uart1_dev = {
//     .name = "uart1",
//     .Init = UART_Rx_Start,
//     .Send = UART_Send,
//     .RecvByte = UART_GetData,
//     .Flush = UART_Flush,
//     .SendAsync = UART_SendAsync,
//     .IsTxComplete = UART_IsTxComplete,
//     .GetRxCount = UART_GetRxCount,
//     .ReadData = UART_ReadData,
//     .PeekData = UART_PeekData,
//     .priv_data = &g_uart1_data
// };

// UART_Device g_uart2_dev = {
//     .name = "uart2",
//     .Init = UART_Rx_Start,
//     .Send = UART_Send,
//     .RecvByte = UART_GetData,
//     .Flush = UART_Flush,
//     .SendAsync = UART_SendAsync,
//     .IsTxComplete = UART_IsTxComplete,
//     .GetRxCount = UART_GetRxCount,
//     .ReadData = UART_ReadData,
//     .PeekData = UART_PeekData,
//     .priv_data = &g_uart2_data
// };

// UART_Device g_uart3_dev = {
//     .name = "uart3",
//     .Init = UART_Rx_Start,
//     .Send = UART_Send,
//     .RecvByte = UART_GetData,
//     .Flush = UART_Flush,
//     .SendAsync = UART_SendAsync,
//     .IsTxComplete = UART_IsTxComplete,
//     .GetRxCount = UART_GetRxCount,
//     .ReadData = UART_ReadData,
//     .PeekData = UART_PeekData,
//     .priv_data = &g_uart3_data
// };

/* USER CODE END 1 */

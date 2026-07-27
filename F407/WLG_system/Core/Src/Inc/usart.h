/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdbool.h>

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */

/* UART设备结构体声明 */
typedef struct UART_Device {
    char *name;
    int (*Init)(struct UART_Device *pDev, int baud, char parity, int data_bit, int stop_bit);
    int (*Send)(struct UART_Device *pDev, uint8_t *datas, uint32_t len, int timeout);
    int (*RecvByte)(struct UART_Device *pDev, uint8_t *data, int timeout);
    int (*Flush)(struct UART_Device *pDev);
    void *priv_data;
} UART_Device, *PUART_Device;

/* 获取UART设备指针 */
UART_Device *GetUARTDevice(char *name);

/* 异步发送 */
int UART_SendAsync(PUART_Device pDev, uint8_t *datas, uint32_t len);
/* 查询发送是否完成 */
int UART_IsTxComplete(PUART_Device pDev);
/* 查询接收缓冲区可用字节数 */
int UART_GetRxCount(PUART_Device pDev);
/* 读取多个字节 */
int UART_ReadData(PUART_Device pDev, uint8_t *buffer, uint16_t len);
/* 预读数据但不移除 */
int UART_PeekData(PUART_Device pDev, uint8_t *buffer, uint16_t len);
/* 初始化设备（可选） */
void UART_Init(PUART_Device pDev);

/* 导出全局设备变量 */
extern UART_Device g_uart1_dev;
extern UART_Device g_uart2_dev;
extern UART_Device g_uart3_dev;



/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */


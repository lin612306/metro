#ifndef __MY_TEMPER_H
#define __MY_TEMPER_H

#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "fifo.h"
#include "uart_dma.h"
#include "app_config.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define CMD_BUFFER_SIZE  218
#define MODULENAME_SIZE  32
#define PARAMNAME_SIZE   32
#define PARAMVALUE_SIZE  32

typedef enum
{
    TEMP_CHANNEL_INTERNAL = 0,  /* D0: 内部培养液温控器, USART3, 目标 37.0 摄氏度 */
    TEMP_CHANNEL_EXTERNAL = 1   /* D1: 外部观察环境温控器, USART2, 目标 30.0 摄氏度 */
} TempChannelId;

extern char rx_buffer[CMD_BUFFER_SIZE];
extern char command_buffer[CMD_BUFFER_SIZE];
extern uint32_t ReceiveLen;
extern uint8_t command_received;
extern char result_code[CMD_BUFFER_SIZE];
extern char module_name[MODULENAME_SIZE];
extern char param_name[PARAMNAME_SIZE];
extern char param_value[PARAMVALUE_SIZE];

void ProcessD0Command(char *command);
void ProcessD1TempCommand(char *command);
void TemperatureSensor_Init(void);

#endif

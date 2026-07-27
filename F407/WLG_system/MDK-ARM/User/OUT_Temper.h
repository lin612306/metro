#ifndef __OUT_Temper_H
#define __OUT_Temper_H


#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "string.h"
#include "stdio.h"
#include  "ctype.h"
#include "stdlib.h"
#include <stdint.h>

#define CMD_BUFFER_SIZE  218       
#define MODULENAME_SIZE  32        
#define PARAMNAME_SIZE   32       
#define PARAMVALUE_SIZE  32        

extern char rx_buffer[CMD_BUFFER_SIZE];                 // UART接收到的命令
extern char command_buffer[CMD_BUFFER_SIZE];             // 命令处理缓冲区
extern uint32_t ReceiveLen;                               // 接收到的数据长度
extern uint8_t command_received;                        // 该标志用于确定命令是否接收完毕
extern char result_code[CMD_BUFFER_SIZE];               // 命令处理结果代码
extern char module_name[MODULENAME_SIZE];              // 模块名称
extern char param_name[PARAMNAME_SIZE];                 // 参数名称
extern char param_value[PARAMVALUE_SIZE];               // 参数值


void ProcessD0Command(char* command);
void Lock(char* command);
void Unlock(char* command);
void SetTemperature(char* command);
void GetCurrentTemperature(char* command);
void SetSpeed(char* command);
void PidMode(char* command);
void SetP(char* command);
void SetI(char* command);
void SetD(char* command);
void TemperatureSensor_Init(void);


#endif



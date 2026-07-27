#ifndef __My_CO2_H
#define __My_CO2_H

#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include <stdint.h>
#include "tim.h"
#include "fifo.h"
#include "uart_dma.h"


// MH-4RCo2传感器协议定义
#define MH4R_START_BYTE            0xFF
#define MH4R_SENSOR_ID             0x01
#define MH4R_CMD_READ_GAS          0x86  // 读取气体浓度值
#define MH4R_CMD_ZERO_CALIBRATE    0x87  // 校准传感器零点
#define MH4R_CMD_SPAN_CALIBRATE    0x88  // 校准传感器跨度点


// CO2 校验和计算函数声明
uint8_t CO2_CalculateChecksum(uint8_t *packet);
// 处理D3命令的函数声明
void ProcessD3Command(char* command);
// 获取当前CO2浓度的函数声明
void GetCurrentCO2(char* command);
// 设置CO2浓度基准值的函数声明
void SetCo2Base(char* command);
// 初始化CO2模块的函数声明
void CO2Module_Init(void);


#endif


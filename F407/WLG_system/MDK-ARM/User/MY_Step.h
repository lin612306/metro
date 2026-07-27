#ifndef __My_STEP_H
#define __My_STEP_H

#include "gpio.h"
#include <stdint.h>
#include "tim.h"
#include "fifo.h"
#include "uart_dma.h"


void StepperMotor_Init(void);
void ProcessD2Command(char* command);
// 设置步进电机细分
void Set_Subdivision(char* command);
// 设置步进电机方向
void Set_Direction(char* command);
// 设置步进电机频率
void Set_Frequency(char* command);
// 启动步进电机
void Start_StepperMotor(void);
// 停止步进电机
void Stop_StepperMotor(void);




#endif


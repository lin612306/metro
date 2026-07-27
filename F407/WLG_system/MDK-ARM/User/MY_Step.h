#ifndef __MY_STEP_H
#define __MY_STEP_H

#include "gpio.h"
#include "tim.h"
#include "fifo.h"
#include "uart_dma.h"
#include <stdint.h>

/*
 * 文件: MY_Step.h
 * 功能: 四路灌流步进电机接口声明。
 * D2 命令格式: dirN:0/1@, subN:8/16/32/64@, freqN:Hz@, startN@, stopN@, startall@, stopall@。
 * N 代表第 1..4 路灌流泵。
 */
void StepperMotor_Init(void);
void ProcessD2Command(char *command);
void Set_Subdivision(char *command);
void Set_Direction(char *command);
void Set_Frequency(char *command);
void Start_StepperMotor(void);
void Stop_StepperMotor(void);

#endif

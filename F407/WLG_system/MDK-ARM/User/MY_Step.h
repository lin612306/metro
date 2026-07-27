#ifndef __MY_STEP_H
#define __MY_STEP_H

#include "gpio.h"
#include "tim.h"
#include "fifo.h"
#include "uart_dma.h"
#include <stdint.h>

/*
 * D2 motor protocol after the leading "D2" is removed by Processcommand():
 *   dirN:0|1             N=1..4, set direction
 *   subN:8|16|32|64      set microstep subdivision
 *   freqN:Hz             set STEP pulse frequency, 1..60000 Hz
 *   startN / stopN       start or stop one motor
 *   startall / stopall   synchronized four-channel start or stop
 *
 * Legacy serial-assistant forms dir:1, sub:64, freq:9000, start:, stop:
 * are accepted as motor 1 commands.
 */
void StepperMotor_Init(void);
void ProcessD2Command(char *command);
void Set_Subdivision(char *command);
void Set_Direction(char *command);
void Set_Frequency(char *command);
void Start_StepperMotor(void);
void Stop_StepperMotor(void);

#endif

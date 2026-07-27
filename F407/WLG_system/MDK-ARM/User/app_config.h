/*
 * 文件: app_config.h
 * 功能: 定义系统掉电保存参数结构。
 * 内容: D0/D1 温控参数、D3 CO2 参数、四路灌流泵参数。
 */
#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define APP_CONFIG_MOTOR_COUNT 4U

typedef struct
{
    float target_temp;
    float kp;
    float ki;
    float kd;
    /* target_temp 由 Qt 下发, PID 参数转发给对应外部温控器。 */
} AppTempConfig;

typedef struct
{
    uint32_t target_ppm;
    float kp;
    float ki;
    uint16_t deadband_ppm;
    float min_duty_percent;
    uint16_t pwm_period_ms;
    uint8_t control_enabled;
    /* control_enabled 只保存控制状态, 实际 PWM 输出状态不保存。 */
} AppCO2Config;

typedef struct
{
    uint8_t direction;
    uint8_t subdivision;
    uint32_t frequency_hz;
    float flow_k;
    float flow_b;
    /* flow_k/flow_b 是该路泵的线性标定曲线: Hz = k * ml/h + b。 */
} AppMotorConfig;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t crc32;
    AppTempConfig d0_temp;
    AppTempConfig d1_temp;
    AppCO2Config co2;
    AppMotorConfig motor[APP_CONFIG_MOTOR_COUNT];
} AppConfig;

void AppConfig_Init(void);
void AppConfig_LoadDefaults(void);
HAL_StatusTypeDef AppConfig_Save(void);
const AppConfig *AppConfig_Get(void);
AppConfig *AppConfig_Mutable(void);

#endif

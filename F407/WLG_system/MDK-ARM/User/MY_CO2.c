/*
 * 文件: MY_CO2.c
 * 功能: D3 二氧化碳传感器读取和 CO2 闭环控制模块。
 * 硬件关系: UART4 连接 CO2 传感器, USART1 连接 Qt 上位机。
 * 输出关系: O1_IN 用于 CO2 进气阀或气泵, O2_IN 用于空气稀释阀或气泵。
 * 控制逻辑: 上位机发送 D3 命令, main.c 去掉 D3 前缀后调用 ProcessD3Command()。
 * 周期任务: main 循环调用 CO2_ControlTask(), 负责软件 PWM、PI 控制和自整定采样。
 * 注意: 如果实物接线变化, 优先修改 CubeMX 或 main.h 的 GPIO 宏。
 */
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "MY_CO2.h"
#include "app_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CO2_PREHEAT_MS        180000UL
#define CO2_CONTROL_PERIOD_MS 1000UL
#define CO2_OUTPUT_MAX        100.0f
#define CO2_OUTPUT_MIN       -100.0f
#define CO2_AUTOTUNE_DURATION_MS 180000UL
#define CO2_AUTOTUNE_STEP_DUTY   50.0f
#define CO2_AUTOTUNE_MIN_DELTA   500L

/* 记录模块初始化时刻, 用于 3 分钟预热判断。 */
static uint32_t co2_init_tick = 0UL;
static uint32_t co2_last_control_tick = 0UL;
static uint32_t co2_pwm_tick = 0UL;
static int32_t co2_latest_ppm = -1;
static float co2_integral = 0.0f;
/* 有符号占空比: 正值表示补 CO2, 负值表示空气稀释。 */
static float co2_output_percent = 0.0f;
/* 自整定状态为 1 时, 普通 PI 控制暂停。 */
static uint8_t co2_autotune_active = 0U;
static uint32_t co2_at_start_tick = 0UL;
static uint32_t co2_at_duration_ms = CO2_AUTOTUNE_DURATION_MS;
static uint32_t co2_at_l_tick = 0UL;
static uint32_t co2_at_t63_tick = 0UL;
static int32_t co2_at_start_ppm = 0L;
static int32_t co2_at_final_ppm = 0L;

extern char result_code[CMD_BUFFER_SIZE];

/*
 * MH-4R/MH-Z 类传感器校验和算法。
 * 9 字节数据包中, 第 1 到第 7 字节累加后取反加一。
 */
uint8_t CO2_CalculateChecksum(uint8_t *packet)
{
    uint8_t i;
    uint8_t checksum = 0U;

    for (i = 1U; i < 8U; i++) {
        checksum += packet[i];
    }

    checksum = 0xFFU - checksum;
    checksum += 1U;
    return checksum;
}

/*
 * 统一通过 USART1 返回上位机。
 * 所有 D3OK/D3ERR/状态文本都走这个函数, 便于 Qt 统一日志。
 */
static void CO2_SendText(const char *text)
{
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)text, strlen(text));
    send_data_from_tx_fifo();
}

static void CO2_SendInvalid(void)
{
    CO2_SendText("D3ERR:InvalidCommand\r\n");
}

static uint8_t CO2_IsPreheated(void)
{
    return ((HAL_GetTick() - co2_init_tick) >= CO2_PREHEAT_MS) ? 1U : 0U;
}

static float CO2_AbsFloat(float value)
{
    return value < 0.0f ? -value : value;
}

/*
 * 写 CO2 执行器 GPIO。
 * co2_state 控制 O1_IN, air_state 控制 O2_IN, 两路输出按逻辑互斥使用。
 */
static void CO2_WriteGasPins(GPIO_PinState co2_state, GPIO_PinState air_state)
{
    HAL_GPIO_WritePin(O1_IN_GPIO_Port, O1_IN_Pin, co2_state);
    HAL_GPIO_WritePin(O2_IN_GPIO_Port, O2_IN_Pin, air_state);
}

static float CO2_ClampDuty(float duty_percent)
{
    if (duty_percent > CO2_OUTPUT_MAX) {
        return CO2_OUTPUT_MAX;
    }
    if (duty_percent < CO2_OUTPUT_MIN) {
        return CO2_OUTPUT_MIN;
    }
    return duty_percent;
}

/*
 * 设置闭环输出占空比。
 * 输入范围为 -100 到 +100, 正值补 CO2, 负值稀释。
 * min_duty_percent 用于克服阀门或气泵的启动死区。
 */
static void CO2_SetOutput(float duty_percent)
{
    AppCO2Config *cfg = &AppConfig_Mutable()->co2;
    float abs_duty;

    duty_percent = CO2_ClampDuty(duty_percent);
    abs_duty = CO2_AbsFloat(duty_percent);

    if (abs_duty > 0.0f && abs_duty < cfg->min_duty_percent) {
        duty_percent = duty_percent > 0.0f ? cfg->min_duty_percent : -cfg->min_duty_percent;
    }

    co2_output_percent = CO2_ClampDuty(duty_percent);
}

/*
 * 软件 PWM 输出函数。
 * 本函数只把 co2_output_percent 转换成 GPIO 开合, 不计算 PI。
 */
static void CO2_UpdateSoftwarePwm(void)
{
    const AppCO2Config *cfg = &AppConfig_Get()->co2;
    uint32_t now = HAL_GetTick();
    uint32_t period = cfg->pwm_period_ms;
    uint32_t phase;
    uint32_t on_ms;
    float abs_duty;

    if (period < 200U) {
        period = 200U;
    }

    if ((now - co2_pwm_tick) >= period) {
        co2_pwm_tick = now;
    }

    if (!cfg->control_enabled && !co2_autotune_active) {
        CO2_WriteGasPins(GPIO_PIN_RESET, GPIO_PIN_RESET);
        return;
    }

    abs_duty = CO2_AbsFloat(co2_output_percent);
    if (abs_duty <= 0.0f) {
        CO2_WriteGasPins(GPIO_PIN_RESET, GPIO_PIN_RESET);
        return;
    }

    phase = now - co2_pwm_tick;
    on_ms = (uint32_t)((abs_duty * (float)period) / 100.0f);
    if (on_ms > period) {
        on_ms = period;
    }

    if (phase < on_ms) {
        if (co2_output_percent > 0.0f) {
            /* O1_IN: CO2 valve/pump; O2_IN: air dilution valve/pump. */
            CO2_WriteGasPins(GPIO_PIN_SET, GPIO_PIN_RESET);
        } else {
            CO2_WriteGasPins(GPIO_PIN_RESET, GPIO_PIN_SET);
        }
    } else {
        CO2_WriteGasPins(GPIO_PIN_RESET, GPIO_PIN_RESET);
    }
}

/*
 * 向 UART4 上的 CO2 传感器发送读取命令, 并等待 9 字节响应。
 * 返回 0 表示成功, 负数表示超时、缓冲区或校验错误。
 */
static int Send_GetCO2(uint8_t *rx_buf, size_t buf_size, int *out_len, uint32_t timeout_ms)
{
    uint32_t tx_start;
    uint32_t start_time;
    uint8_t checksum1;
    uint8_t cmd[9] = {0};

    if (rx_buf == NULL || out_len == NULL || buf_size < 10U) {
        return -2;
    }

    *out_len = 0;
    cmd[0] = MH4R_START_BYTE;
    cmd[1] = MH4R_SENSOR_ID;
    cmd[2] = MH4R_CMD_READ_GAS;
    cmd[3] = 0x00U;
    cmd[4] = 0x00U;
    cmd[5] = 0x00U;
    cmd[6] = 0x00U;
    cmd[7] = 0x00U;
    cmd[8] = 0x79U;

    checksum1 = CO2_CalculateChecksum(cmd);
    if (checksum1 != cmd[8]) {
        return -3;
    }

    fifo_s_puts(&uart4_tx_fifo, cmd, sizeof(cmd));
    send_data_from_tx_fifo();

    tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart4_tx_fifo)) {
        if ((HAL_GetTick() - tx_start) > 200UL) {
            return -4;
        }
    }

    start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms) {
        if (!fifo_s_is_empty(&uart4_rx_fifo)) {
            fifo_s_get(&uart4_rx_fifo, &rx_buf[*out_len]);
            (*out_len)++;

            if (*out_len == 9) {
                uint8_t checksum2 = CO2_CalculateChecksum(rx_buf);
                if (checksum2 != rx_buf[8]) {
                    return -5;
                }
                rx_buf[*out_len] = '\0';
                return 0;
            }

            if (*out_len >= ((int)buf_size - 1)) {
                return -6;
            }

            start_time = HAL_GetTick();
        }
    }

    return -1;
}

/*
 * 读取并解析 CO2 ppm。
 * 传感器返回的高低字节位于 response[2] 和 response[3]。
 */
static int CO2_ReadPpm(uint32_t *ppm)
{
    uint8_t response[CMD_BUFFER_SIZE] = {0};
    int len = 0;
    int ret = Send_GetCO2(response, sizeof(response), &len, 200UL);

    if (ret != 0) {
        return ret;
    }

    *ppm = ((uint32_t)response[2] * 256UL) + (uint32_t)response[3];
    co2_latest_ppm = (int32_t)(*ppm);
    return 0;
}

static int Send_SetBase(void)
{
    uint32_t tx_start;
    uint8_t checksum1;
    uint8_t cmd[9] = {0};

    cmd[0] = MH4R_START_BYTE;
    cmd[1] = MH4R_SENSOR_ID;
    cmd[2] = MH4R_CMD_ZERO_CALIBRATE;
    cmd[3] = 0x00U;
    cmd[4] = 0x00U;
    cmd[5] = 0x00U;
    cmd[6] = 0x00U;
    cmd[7] = 0x00U;
    cmd[8] = 0x78U;

    checksum1 = CO2_CalculateChecksum(cmd);
    if (checksum1 != cmd[8]) {
        return -3;
    }

    fifo_s_puts(&uart4_tx_fifo, cmd, sizeof(cmd));
    send_data_from_tx_fifo();

    tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart4_tx_fifo)) {
        if ((HAL_GetTick() - tx_start) > 200UL) {
            return -4;
        }
    }

    return 0;
}

void GetCurrentCO2(char* command)
{
    uint32_t co2_value = 0UL;
    int ret;
    (void)command;

    ret = CO2_ReadPpm(&co2_value);
    if (ret != 0) {
        snprintf(result_code, CMD_BUFFER_SIZE, "D3ERR:getco2:%d\r\n", ret);
        CO2_SendText(result_code);
        return;
    }

    snprintf(result_code, CMD_BUFFER_SIZE, "CO2Concentration:%lu ppm\r\n", (unsigned long)co2_value);
    CO2_SendText(result_code);
}

void SetCo2Base(char* command)
{
    int ret;
    (void)command;

    ret = Send_SetBase();
    if (ret != 0) {
        snprintf(result_code, CMD_BUFFER_SIZE, "D3ERR:setbase:%d\r\n", ret);
        CO2_SendText(result_code);
        return;
    }

    CO2_SendText("D3OK:setbase\r\n");
}

static void CO2_HandleSetFloat(const char *value, float *target, const char *name)
{
    if (value == NULL || target == NULL) {
        CO2_SendInvalid();
        return;
    }

    *target = (float)atof(value);
    snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:%s:%.3f\r\n", name, *target);
    CO2_SendText(result_code);
}

/* 处理 D3settarget:x@, 目标浓度限制在 400 到 60000 ppm。 */
static void CO2_HandleSetTarget(const char *value)
{
    uint32_t target;

    if (value == NULL) {
        CO2_SendInvalid();
        return;
    }

    target = (uint32_t)strtoul(value, NULL, 10);
    if (target < 400UL || target > 60000UL) {
        CO2_SendText("D3ERR:target:400-60000\r\n");
        return;
    }

    AppConfig_Mutable()->co2.target_ppm = target;
    snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:target:%lu\r\n", (unsigned long)target);
    CO2_SendText(result_code);
}

/* 设置 CO2 死区, 误差落入死区时输出归零, 避免阀门频繁抖动。 */
static void CO2_HandleSetDeadband(const char *value)
{
    uint32_t deadband;

    if (value == NULL) {
        CO2_SendInvalid();
        return;
    }

    deadband = (uint32_t)strtoul(value, NULL, 10);
    if (deadband > 20000UL) {
        CO2_SendText("D3ERR:deadband:0-20000\r\n");
        return;
    }

    AppConfig_Mutable()->co2.deadband_ppm = (uint16_t)deadband;
    snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:deadband:%lu\r\n", (unsigned long)deadband);
    CO2_SendText(result_code);
}

static void CO2_HandleSetPwm(const char *value)
{
    uint32_t period;

    if (value == NULL) {
        CO2_SendInvalid();
        return;
    }

    period = (uint32_t)strtoul(value, NULL, 10);
    if (period < 200UL || period > 10000UL) {
        CO2_SendText("D3ERR:pwm:200-10000\r\n");
        return;
    }

    AppConfig_Mutable()->co2.pwm_period_ms = (uint16_t)period;
    snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:pwm:%lu\r\n", (unsigned long)period);
    CO2_SendText(result_code);
}

static void CO2_HandleSetMinDuty(const char *value)
{
    float duty;

    if (value == NULL) {
        CO2_SendInvalid();
        return;
    }

    duty = (float)atof(value);
    if (duty < 0.0f || duty > 50.0f) {
        CO2_SendText("D3ERR:minduty:0-50\r\n");
        return;
    }

    AppConfig_Mutable()->co2.min_duty_percent = duty;
    snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:minduty:%.2f\r\n", duty);
    CO2_SendText(result_code);
}

static void CO2_SendPI(void)
{
    const AppCO2Config *co2 = &AppConfig_Get()->co2;
    snprintf(result_code, CMD_BUFFER_SIZE, "CurrentPI:Kp=%.3f, Ki=%.3f\r\n", co2->kp, co2->ki);
    CO2_SendText(result_code);
}

static void CO2_SendStatus(void)
{
    const AppCO2Config *co2 = &AppConfig_Get()->co2;
    snprintf(result_code, CMD_BUFFER_SIZE,
             "D3STATUS:preheat=%u,target=%lu,control=%u,latest=%ld,out=%.1f,deadband=%u,minduty=%.1f,pwm=%u,AT=%u\r\n",
             CO2_IsPreheated(), (unsigned long)co2->target_ppm,
             co2->control_enabled, (long)co2_latest_ppm,
             co2_output_percent, co2->deadband_ppm,
             co2->min_duty_percent, co2->pwm_period_ms,
             co2_autotune_active);
    CO2_SendText(result_code);
}

/*
 * 根据 Cohen-Coon 经验公式计算 CO2 PI 参数。
 * process_gain 为阶跃响应增益 K, dead_time 为滞后 L, time_constant 为时间常数 T。
 */
static uint8_t CO2_ApplyCohenCoon(float process_gain, float dead_time, float time_constant)
{
    float ratio;
    AppCO2Config *co2 = &AppConfig_Mutable()->co2;

    if (process_gain <= 0.0f || dead_time <= 0.0f || time_constant <= 0.0f) {
        return 0U;
    }

    ratio = dead_time / time_constant;
    co2->kp = (time_constant / (process_gain * dead_time)) * (0.9f + ratio / 12.0f);
    co2->ki = co2->kp / (dead_time * ((30.0f + 3.0f * ratio) / (9.0f + 20.0f * ratio)));
    return 1U;
}

static void CO2_HandleCohenCoon(const char *value)
{
    char temp[64];
    char *k_text;
    char *l_text;
    char *t_text;
    float process_gain;
    float dead_time;
    float time_constant;
    const AppCO2Config *co2;

    if (value == NULL || strlen(value) >= sizeof(temp)) {
        CO2_SendInvalid();
        return;
    }

    strcpy(temp, value);
    k_text = strtok(temp, ",");
    l_text = strtok(NULL, ",");
    t_text = strtok(NULL, ",");
    if (k_text == NULL || l_text == NULL || t_text == NULL) {
        CO2_SendText("D3ERR:setcc:K,L,T\r\n");
        return;
    }

    process_gain = (float)atof(k_text);
    dead_time = (float)atof(l_text);
    time_constant = (float)atof(t_text);
    if (!CO2_ApplyCohenCoon(process_gain, dead_time, time_constant)) {
        CO2_SendText("D3ERR:setcc:param\r\n");
        return;
    }

    co2 = &AppConfig_Get()->co2;
    snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:setcc:Kp=%.3f,Ki=%.3f\r\n", co2->kp, co2->ki);
    CO2_SendText(result_code);
}

/*
 * 启动 CO2 自整定。
 * D3startAT@ 使用默认 180 秒采样; D3startAT:120000@ 可指定采样时长, 单位 ms。
 */
static void CO2_StartAutotune(const char *value)
{
    uint32_t measured = 0UL;
    uint32_t duration = CO2_AUTOTUNE_DURATION_MS;

    if (value != NULL && *value != '\0') {
        duration = (uint32_t)strtoul(value, NULL, 10);
        if (duration < 60000UL || duration > 600000UL) {
            CO2_SendText("D3ERR:startAT:duration=60000-600000\r\n");
            return;
        }
    }

    if (!CO2_IsPreheated()) {
        CO2_SendText("D3ERR:startAT:preheating\r\n");
        return;
    }

    if (CO2_ReadPpm(&measured) != 0) {
        CO2_SendText("D3ERR:startAT:read\r\n");
        return;
    }

    co2_autotune_active = 1U;
    AppConfig_Mutable()->co2.control_enabled = 0U;
    co2_integral = 0.0f;
    co2_at_start_tick = HAL_GetTick();
    co2_at_duration_ms = duration;
    co2_at_l_tick = 0UL;
    co2_at_t63_tick = 0UL;
    co2_at_start_ppm = (int32_t)measured;
    co2_at_final_ppm = (int32_t)measured;
    CO2_SetOutput(CO2_AUTOTUNE_STEP_DUTY);

    snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:startAT:start=%ld,duration=%lu,step=%.0f\r\n",
             (long)co2_at_start_ppm, (unsigned long)duration, CO2_AUTOTUNE_STEP_DUTY);
    CO2_SendText(result_code);
}

/*
 * 自整定采样任务。
 * 运行期间每秒读取一次 CO2, 到时后自动计算 PI。
 */
static void CO2_AutotuneTask(uint32_t now)
{
    uint32_t measured = 0UL;
    int32_t delta;
    int32_t final_delta;
    float process_gain;
    float dead_time;
    float time_constant;
    const AppCO2Config *co2;

    if ((now - co2_last_control_tick) < CO2_CONTROL_PERIOD_MS) {
        return;
    }
    co2_last_control_tick = now;

    if (CO2_ReadPpm(&measured) != 0) {
        return;
    }

    delta = (int32_t)measured - co2_at_start_ppm;
    if ((int32_t)measured > co2_at_final_ppm) {
        co2_at_final_ppm = (int32_t)measured;
    }

    final_delta = co2_at_final_ppm - co2_at_start_ppm;
    if (co2_at_l_tick == 0UL && delta >= 100L) {
        co2_at_l_tick = now;
    }
    if (co2_at_t63_tick == 0UL && delta >= CO2_AUTOTUNE_MIN_DELTA) {
        co2_at_t63_tick = now;
    }

    if ((now - co2_at_start_tick) < co2_at_duration_ms) {
        return;
    }

    CO2_SetOutput(0.0f);
    co2_autotune_active = 0U;
    final_delta = co2_at_final_ppm - co2_at_start_ppm;
    if (final_delta < CO2_AUTOTUNE_MIN_DELTA || co2_at_l_tick == 0UL || co2_at_t63_tick == 0UL || co2_at_t63_tick <= co2_at_l_tick) {
        CO2_SendText("D3ERR:autoCC:response\r\n");
        return;
    }

    process_gain = ((float)final_delta) / CO2_AUTOTUNE_STEP_DUTY;
    dead_time = ((float)(co2_at_l_tick - co2_at_start_tick)) / 1000.0f;
    time_constant = ((float)(co2_at_t63_tick - co2_at_l_tick)) / 1000.0f;
    if (!CO2_ApplyCohenCoon(process_gain, dead_time, time_constant)) {
        CO2_SendText("D3ERR:autoCC:param\r\n");
        return;
    }

    co2 = &AppConfig_Get()->co2;
    snprintf(result_code, CMD_BUFFER_SIZE,
             "D3OK:autoCC:K=%.3f,L=%.1f,T=%.1f,Kp=%.3f,Ki=%.3f\r\n",
             process_gain, dead_time, time_constant, co2->kp, co2->ki);
    CO2_SendText(result_code);
}

/*
 * D3 命令解析入口。
 * main.c 已经识别 D3 前缀, 这里只处理 getco2、settarget、startctrl 等子命令。
 */
void ProcessD3Command(char* command)
{
    char *colon_ptr;
    char *value = NULL;
    size_t command_len;

    if (command == NULL) {
        CO2_SendInvalid();
        return;
    }

    command_len = strlen(command);
    if (command_len > 0U && command[command_len - 1U] == '@') {
        command[command_len - 1U] = '\0';
    }

    colon_ptr = strchr(command, ':');
    if (colon_ptr != NULL) {
        *colon_ptr = '\0';
        value = colon_ptr + 1;
    }

    if (strcmp(command, "getco2") == 0) {
        GetCurrentCO2(command);
    } else if (strcmp(command, "setbase") == 0) {
        SetCo2Base(command);
    } else if (strcmp(command, "setp") == 0) {
        CO2_HandleSetFloat(value, &AppConfig_Mutable()->co2.kp, "setp");
    } else if (strcmp(command, "seti") == 0) {
        CO2_HandleSetFloat(value, &AppConfig_Mutable()->co2.ki, "seti");
    } else if (strcmp(command, "getpi") == 0) {
        CO2_SendPI();
    } else if (strcmp(command, "settarget") == 0) {
        CO2_HandleSetTarget(value);
    } else if (strcmp(command, "setdeadband") == 0) {
        CO2_HandleSetDeadband(value);
    } else if (strcmp(command, "setminduty") == 0) {
        CO2_HandleSetMinDuty(value);
    } else if (strcmp(command, "setpwm") == 0) {
        CO2_HandleSetPwm(value);
    } else if (strcmp(command, "setcc") == 0) {
        CO2_HandleCohenCoon(value);
    } else if (strcmp(command, "gettarget") == 0) {
        snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:target:%lu\r\n", (unsigned long)AppConfig_Get()->co2.target_ppm);
        CO2_SendText(result_code);
    } else if (strcmp(command, "startctrl") == 0) {
        if (!CO2_IsPreheated()) {
            CO2_SendText("D3ERR:preheating\r\n");
            return;
        }
        AppConfig_Mutable()->co2.control_enabled = 1U;
        co2_autotune_active = 0U;
        co2_integral = 0.0f;
        CO2_SendText("D3OK:startctrl\r\n");
    } else if (strcmp(command, "stopctrl") == 0) {
        AppConfig_Mutable()->co2.control_enabled = 0U;
        co2_autotune_active = 0U;
        CO2_SetOutput(0.0f);
        CO2_WriteGasPins(GPIO_PIN_RESET, GPIO_PIN_RESET);
        CO2_SendText("D3OK:stopctrl\r\n");
    } else if (strcmp(command, "startAT") == 0) {
        CO2_StartAutotune(value);
    } else if (strcmp(command, "status") == 0) {
        CO2_SendStatus();
    } else if (strcmp(command, "save") == 0) {
        if (AppConfig_Save() == HAL_OK) {
            CO2_SendText("D3OK:save\r\n");
        } else {
            CO2_SendText("D3ERR:save\r\n");
        }
    } else {
        CO2_SendInvalid();
    }
}

/*
 * CO2 闭环周期任务。
 * 先刷新软件 PWM, 再根据状态执行自整定、预热保护或 PI 控制。
 */
void CO2_ControlTask(void)
{
    AppCO2Config *co2 = &AppConfig_Mutable()->co2;
    uint32_t now = HAL_GetTick();
    uint32_t measured = 0UL;
    float error;
    float output;

    CO2_UpdateSoftwarePwm();

    if (co2_autotune_active) {
        CO2_AutotuneTask(now);
        return;
    }

    if (!co2->control_enabled) {
        return;
    }

    if (!CO2_IsPreheated()) {
        CO2_SetOutput(0.0f);
        return;
    }

    if ((now - co2_last_control_tick) < CO2_CONTROL_PERIOD_MS) {
        return;
    }
    co2_last_control_tick = now;

    if (CO2_ReadPpm(&measured) != 0) {
        CO2_SetOutput(0.0f);
        return;
    }

    error = (float)((int32_t)co2->target_ppm - (int32_t)measured);
    if (CO2_AbsFloat(error) <= (float)co2->deadband_ppm) {
        co2_integral = 0.0f;
        CO2_SetOutput(0.0f);
        return;
    }

    co2_integral += error;
    if (co2_integral > 100000.0f) {
        co2_integral = 100000.0f;
    } else if (co2_integral < -100000.0f) {
        co2_integral = -100000.0f;
    }

    output = co2->kp * error + co2->ki * co2_integral;
    CO2_SetOutput(output);
}

void CO2Module_Init(void)
{
    co2_init_tick = HAL_GetTick();
    co2_last_control_tick = co2_init_tick;
    co2_pwm_tick = co2_init_tick;
    co2_latest_ppm = -1;
    co2_integral = 0.0f;
    co2_output_percent = 0.0f;
    co2_autotune_active = 0U;
    CO2_WriteGasPins(GPIO_PIN_RESET, GPIO_PIN_RESET);
    CO2_SendText("D3OK:init preheat=180s\r\n");
}

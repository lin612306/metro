/*
 * 文件: MY_Step.c
 * 功能: D2 灌流步进电机控制模块。
 * 作用: 管理四路灌流泵的方向、细分、频率、启动和停止。
 * 协议: 上位机发送 D2 命令, main.c 去掉 D2 前缀后传入 ProcessD2Command()。
 * 示例: D2freq1:9000@, D2sub1:64@, D2startall@, D2stopall@。
 */
#include "MY_Step.h"
#include "app_config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STEPPER_COUNT           4U
#define STEPPER_TIM1_CLK_HZ     1000000UL
#define STEPPER_TIM234_CLK_HZ   500000UL
#define STEPPER_MIN_FREQ_HZ     1UL
#define STEPPER_MAX_FREQ_HZ     60000UL
#define STEPPER_DEFAULT_FREQ_HZ 9000UL
#define STEPPER_START_FREQ_HZ   1000UL
#define STEPPER_RAMP_STEPS      20U
#define STEPPER_RAMP_DELAY_MS   2U
#define STEPPER_DEFAULT_SUB     64U
#define STEPPER_DEFAULT_DIR     1U

/*
 * 单路步进电机运行参数。
 * 每一路电机都使用同一个结构体, 便于后续增加标定曲线和流量换算。
 */
typedef struct
{
    TIM_HandleTypeDef *htim;      /* PWM 定时器句柄 */
    uint32_t channel;             /* PWM 通道 */
    GPIO_TypeDef *dir_port;       /* 方向 GPIO 端口 */
    uint16_t dir_pin;             /* 方向 GPIO 引脚 */
    GPIO_TypeDef *ms1_port;       /* 细分 MS1 GPIO 端口 */
    uint16_t ms1_pin;             /* 细分 MS1 GPIO 引脚 */
    GPIO_TypeDef *ms2_port;       /* 细分 MS2 GPIO 端口 */
    uint16_t ms2_pin;             /* 细分 MS2 GPIO 引脚 */
    GPIO_TypeDef *en_port;        /* 使能 GPIO 端口 */
    uint16_t en_pin;              /* 使能 GPIO 引脚, 低电平使能 */
    uint32_t timer_clock_hz;      /* 定时器计数频率, 用于 Hz 到 ARR 换算 */
    uint32_t frequency_hz;        /* 当前输出脉冲频率 */
    uint8_t subdivision;          /* 当前细分, 默认 64 细分 */
    uint8_t direction;            /* 当前方向, 0 和 1 对应两个转向 */
    uint8_t running;              /* 运行状态, 1 为正在输出 PWM */
} StepperMotor;

/* main.c 中的通用返回缓冲区, 本模块复用它组装 D2OK/D2ERR 应答。 */
extern char result_code[CMD_BUFFER_SIZE];

/*
 * 四路灌流泵硬件映射表。
 * 第1路: TIM1_CH1, DIR PB8,  MS1 PE0, MS2 PB9, EN PE1
 * 第2路: TIM2_CH1, DIR PB3,  MS1 PB5, MS2 PB4, EN PB6
 * 第3路: TIM3_CH1, DIR PD4,  MS1 PD6, MS2 PD5, EN PD7
 * 第4路: TIM4_CH1, DIR PD0,  MS1 PD2, MS2 PD1, EN PD3
 * 注意: 上位机使用 motor_id=1..4 选择对应泵路。
 */
static StepperMotor stepper_motors[STEPPER_COUNT] = {
    {&htim1, TIM_CHANNEL_1, TIM1_DIR_GPIO_Port, TIM1_DIR_Pin, TIM1_MS1_GPIO_Port, TIM1_MS1_Pin, TIM1_MS2_GPIO_Port, TIM1_MS2_Pin, TIM1_EN_GPIO_Port, TIM1_EN_Pin, STEPPER_TIM1_CLK_HZ, STEPPER_DEFAULT_FREQ_HZ, STEPPER_DEFAULT_SUB, STEPPER_DEFAULT_DIR, 0},
    {&htim2, TIM_CHANNEL_1, TIM2_DIR_GPIO_Port, TIM2_DIR_Pin, TIM2_MS1_GPIO_Port, TIM2_MS1_Pin, TIM2_MS2_GPIO_Port, TIM2_MS2_Pin, TIM2_EN_GPIO_Port, TIM2_EN_Pin, STEPPER_TIM234_CLK_HZ, STEPPER_DEFAULT_FREQ_HZ, STEPPER_DEFAULT_SUB, STEPPER_DEFAULT_DIR, 0},
    {&htim3, TIM_CHANNEL_1, TIM3_DIR_GPIO_Port, TIM3_DIR_Pin, TIM3_MS1_GPIO_Port, TIM3_MS1_Pin, TIM3_MS2_GPIO_Port, TIM3_MS2_Pin, TIM3_EN_GPIO_Port, TIM3_EN_Pin, STEPPER_TIM234_CLK_HZ, STEPPER_DEFAULT_FREQ_HZ, STEPPER_DEFAULT_SUB, STEPPER_DEFAULT_DIR, 0},
    {&htim4, TIM_CHANNEL_1, TIM4_DIR_GPIO_Port, TIM4_DIR_Pin, TIM4_MS1_GPIO_Port, TIM4_MS1_Pin, TIM4_MS2_GPIO_Port, TIM4_MS2_Pin, TIM4_EN_GPIO_Port, TIM4_EN_Pin, STEPPER_TIM234_CLK_HZ, STEPPER_DEFAULT_FREQ_HZ, STEPPER_DEFAULT_SUB, STEPPER_DEFAULT_DIR, 0},
};

static void Stepper_SendText(const char *text)
{
    fifo_s_puts(&uart1_tx_fifo, (const uint8_t *)text, (uint16_t)strlen(text));
    send_data_from_tx_fifo();
}

static void Stepper_SendInvalid(void)
{
    Stepper_SendText("D2ERR:InvalidCommand\r\n");
}

static uint8_t Stepper_IsDigits(const char *text)
{
    if (text == NULL || *text == '\0') {
        return 0;
    }

    while (*text != '\0') {
        if (!isdigit((unsigned char)*text)) {
            return 0;
        }
        text++;
    }

    return 1;
}

static StepperMotor *Stepper_GetMotor(uint8_t motor_id)
{
    if (motor_id < 1U || motor_id > STEPPER_COUNT) {
        return NULL;
    }

    return &stepper_motors[motor_id - 1U];
}

/*
 * 设置细分引脚。默认 64 细分, 用于低流量稳定灌流。
 */
static AppMotorConfig *Stepper_GetConfig(uint8_t motor_id)
{
    if (motor_id < 1U || motor_id > STEPPER_COUNT) {
        return NULL;
    }

    return &AppConfig_Mutable()->motor[motor_id - 1U];
}

static uint8_t Stepper_IsValidSubdivision(uint8_t subdivision)
{
    return (subdivision == 8U || subdivision == 16U || subdivision == 32U || subdivision == 64U);
}

static void Stepper_SetSubdivisionPins(StepperMotor *motor, uint8_t subdivision)
{
    GPIO_PinState ms1 = GPIO_PIN_RESET;
    GPIO_PinState ms2 = GPIO_PIN_RESET;

    switch (subdivision) {
    case 8U:
        ms1 = GPIO_PIN_RESET;
        ms2 = GPIO_PIN_RESET;
        break;
    case 16U:
        ms1 = GPIO_PIN_SET;
        ms2 = GPIO_PIN_SET;
        break;
    case 32U:
        ms1 = GPIO_PIN_SET;
        ms2 = GPIO_PIN_RESET;
        break;
    case 64U:
        ms1 = GPIO_PIN_RESET;
        ms2 = GPIO_PIN_SET;
        break;
    default:
        return;
    }

    HAL_GPIO_WritePin(motor->ms1_port, motor->ms1_pin, ms1);
    HAL_GPIO_WritePin(motor->ms2_port, motor->ms2_pin, ms2);
    motor->subdivision = subdivision;
}

static void Stepper_SetDirectionPin(StepperMotor *motor, uint8_t direction)
{
    HAL_GPIO_WritePin(motor->dir_port, motor->dir_pin, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
    motor->direction = direction ? 1U : 0U;
}

/*
 * 将 Hz 频率换算成定时器 ARR/CCR, CCR 取 ARR 的一半形成 50% 占空比。
 */
static uint8_t Stepper_ApplyFrequencyHz(StepperMotor *motor, uint32_t frequency_hz)
{
    uint32_t arr;

    if (frequency_hz < STEPPER_MIN_FREQ_HZ || frequency_hz > STEPPER_MAX_FREQ_HZ) {
        return 0;
    }

    arr = (motor->timer_clock_hz / frequency_hz);
    if (arr == 0U) {
        arr = 1U;
    }

    __HAL_TIM_SET_AUTORELOAD(motor->htim, arr - 1U);
    __HAL_TIM_SET_COMPARE(motor->htim, motor->channel, arr / 2U);
    __HAL_TIM_SET_COUNTER(motor->htim, 0U);
    motor->frequency_hz = frequency_hz;

    return 1;
}

/*
 * 加减速过程, 减少灌流泵启动瞬间冲击和失步风险。
 */
static uint8_t Stepper_RampFrequencyHz(StepperMotor *motor, uint32_t target_hz)
{
    uint32_t start_hz;
    uint32_t step;
    uint32_t i;

    if (target_hz < STEPPER_MIN_FREQ_HZ || target_hz > STEPPER_MAX_FREQ_HZ) {
        return 0;
    }

    if (motor->frequency_hz == target_hz) {
        return Stepper_ApplyFrequencyHz(motor, target_hz);
    }

    if (!motor->running) {
        return Stepper_ApplyFrequencyHz(motor, target_hz);
    }

    start_hz = motor->frequency_hz;
    for (i = 1U; i <= STEPPER_RAMP_STEPS; i++) {
        if (target_hz >= start_hz) {
            step = start_hz + ((target_hz - start_hz) * i) / STEPPER_RAMP_STEPS;
        } else {
            step = start_hz - ((start_hz - target_hz) * i) / STEPPER_RAMP_STEPS;
        }
        Stepper_ApplyFrequencyHz(motor, step);
        HAL_Delay(STEPPER_RAMP_DELAY_MS);
    }

    return Stepper_ApplyFrequencyHz(motor, target_hz);
}

static void Stepper_StartMotor(uint8_t motor_id)
{
    StepperMotor *motor = Stepper_GetMotor(motor_id);
    uint32_t target_hz;

    if (motor == NULL) {
        Stepper_SendInvalid();
        return;
    }

    target_hz = motor->frequency_hz;
    if (motor->frequency_hz > STEPPER_START_FREQ_HZ) {
        Stepper_ApplyFrequencyHz(motor, STEPPER_START_FREQ_HZ);
    }
    HAL_GPIO_WritePin(motor->en_port, motor->en_pin, GPIO_PIN_RESET);
    HAL_TIM_PWM_Start(motor->htim, motor->channel);
    motor->running = 1U;
    Stepper_RampFrequencyHz(motor, target_hz);

    snprintf(result_code, CMD_BUFFER_SIZE, "D2OK:start%d\r\n", motor_id);
    Stepper_SendText(result_code);
}

static void Stepper_StopMotor(uint8_t motor_id)
{
    StepperMotor *motor = Stepper_GetMotor(motor_id);

    if (motor == NULL) {
        Stepper_SendInvalid();
        return;
    }

    HAL_TIM_PWM_Stop(motor->htim, motor->channel);
    HAL_GPIO_WritePin(motor->en_port, motor->en_pin, GPIO_PIN_SET);
    motor->running = 0U;

    snprintf(result_code, CMD_BUFFER_SIZE, "D2OK:stop%d\r\n", motor_id);
    Stepper_SendText(result_code);
}

/*
 * 四路同步启动: 先开启全部 PWM, 再统一爬升到各自目标频率。
 */
static void Stepper_StartAll(void)
{
    uint8_t i;
    uint32_t targets[STEPPER_COUNT];
    uint32_t start_values[STEPPER_COUNT];
    uint32_t step;
    uint32_t freq;
    uint32_t start_hz;
    uint32_t target_hz;
    StepperMotor *motor;

    for (i = 1U; i <= STEPPER_COUNT; i++) {
        motor = Stepper_GetMotor(i);
        targets[i - 1U] = motor->frequency_hz;
        if (motor->frequency_hz > STEPPER_START_FREQ_HZ) {
            Stepper_ApplyFrequencyHz(motor, STEPPER_START_FREQ_HZ);
        }
        start_values[i - 1U] = motor->frequency_hz;
        HAL_GPIO_WritePin(motor->en_port, motor->en_pin, GPIO_PIN_RESET);
        HAL_TIM_PWM_Start(motor->htim, motor->channel);
        motor->running = 1U;
    }

    for (step = 1U; step <= STEPPER_RAMP_STEPS; step++) {
        for (i = 1U; i <= STEPPER_COUNT; i++) {
            motor = Stepper_GetMotor(i);
            start_hz = start_values[i - 1U];
            target_hz = targets[i - 1U];

            if (target_hz >= start_hz) {
                freq = start_hz + ((target_hz - start_hz) * step) / STEPPER_RAMP_STEPS;
            } else {
                freq = start_hz - ((start_hz - target_hz) * step) / STEPPER_RAMP_STEPS;
            }
            Stepper_ApplyFrequencyHz(motor, freq);
        }
        HAL_Delay(STEPPER_RAMP_DELAY_MS);
    }

    Stepper_SendText("D2OK:startall\r\n");
}

static void Stepper_StopAll(void)
{
    uint8_t i;
    StepperMotor *motor;

    for (i = 1U; i <= STEPPER_COUNT; i++) {
        motor = Stepper_GetMotor(i);
        HAL_TIM_PWM_Stop(motor->htim, motor->channel);
        HAL_GPIO_WritePin(motor->en_port, motor->en_pin, GPIO_PIN_SET);
        motor->running = 0U;
    }

    Stepper_SendText("D2OK:stopall\r\n");
}

static uint8_t Stepper_ParseIndexedCommand(const char *command, const char *name, uint8_t *motor_id, const char **value)
{
    size_t name_len = strlen(name);
    const char *p = command + name_len;

    if (strncmp(command, name, name_len) != 0) {
        return 0;
    }

    if (*p >= '1' && *p <= '4') {
        *motor_id = (uint8_t)(*p - '0');
        p++;
    } else {
        *motor_id = 1U;
    }

    if (value != NULL) {
        if (*p != ':') {
            return 0;
        }
        *value = p + 1;
    } else if (*p == ':' && p[1] == '\0') {
        return 1;
    } else if (*p != '\0') {
        return 0;
    }

    return 1;
}

static void Stepper_HandleDir(const char *command)
{
    uint8_t motor_id;
    const char *value;
    int direction;
    StepperMotor *motor;

    if (!Stepper_ParseIndexedCommand(command, "dir", &motor_id, &value) || !Stepper_IsDigits(value)) {
        Stepper_SendInvalid();
        return;
    }

    direction = atoi(value);
    if (direction != 0 && direction != 1) {
        Stepper_SendText("D2ERR:DirectionMustBe0Or1\r\n");
        return;
    }

    motor = Stepper_GetMotor(motor_id);
    if (motor == NULL) {
        Stepper_SendInvalid();
        return;
    }

    Stepper_SetDirectionPin(motor, (uint8_t)direction);
    snprintf(result_code, CMD_BUFFER_SIZE, "D2OK:dir%d:%d\r\n", motor_id, direction);
    Stepper_SendText(result_code);
}

static void Stepper_HandleSub(const char *command)
{
    uint8_t motor_id;
    const char *value;
    int subdivision;
    StepperMotor *motor;

    if (!Stepper_ParseIndexedCommand(command, "sub", &motor_id, &value) || !Stepper_IsDigits(value)) {
        Stepper_SendInvalid();
        return;
    }

    subdivision = atoi(value);
    if (subdivision != 8 && subdivision != 16 && subdivision != 32 && subdivision != 64) {
        Stepper_SendText("D2ERR:SubdivisionMustBe8_16_32_64\r\n");
        return;
    }

    motor = Stepper_GetMotor(motor_id);
    if (motor == NULL) {
        Stepper_SendInvalid();
        return;
    }

    Stepper_SetSubdivisionPins(motor, (uint8_t)subdivision);
    snprintf(result_code, CMD_BUFFER_SIZE, "D2OK:sub%d:%d\r\n", motor_id, subdivision);
    Stepper_SendText(result_code);
}

static void Stepper_HandleFreq(const char *command)
{
    uint8_t motor_id;
    const char *value;
    uint32_t frequency_hz;
    StepperMotor *motor;

    if (!Stepper_ParseIndexedCommand(command, "freq", &motor_id, &value) || !Stepper_IsDigits(value)) {
        Stepper_SendInvalid();
        return;
    }

    frequency_hz = (uint32_t)strtoul(value, NULL, 10);
    motor = Stepper_GetMotor(motor_id);
    if (motor == NULL) {
        Stepper_SendInvalid();
        return;
    }

    if (!Stepper_RampFrequencyHz(motor, frequency_hz)) {
        Stepper_SendText("D2ERR:FrequencyMustBe1To60000Hz\r\n");
        return;
    }

    snprintf(result_code, CMD_BUFFER_SIZE, "D2OK:freq%d:%luHz\r\n", motor_id, (unsigned long)frequency_hz);
    Stepper_SendText(result_code);
}

static void Stepper_HandleStartStop(const char *command, uint8_t start)
{
    uint8_t motor_id;

    if (start && strcmp(command, "startall") == 0) {
        Stepper_StartAll();
        return;
    }

    if (!start && strcmp(command, "stopall") == 0) {
        Stepper_StopAll();
        return;
    }

    if (!Stepper_ParseIndexedCommand(command, start ? "start" : "stop", &motor_id, NULL)) {
        Stepper_SendInvalid();
        return;
    }

    if (start) {
        Stepper_StartMotor(motor_id);
    } else {
        Stepper_StopMotor(motor_id);
    }
}

/*
 * D2 命令入口。支持 dirN:x, subN:x, freqN:x, startN, stopN, startall, stopall。
 */
static void Stepper_SaveConfig(void)
{
    if (AppConfig_Save() == HAL_OK) {
        Stepper_SendText("D2OK:save\r\n");
    } else {
        Stepper_SendText("D2ERR:save\r\n");
    }
}

void ProcessD2Command(char *command)
{
    size_t len;

    if (command == NULL) {
        Stepper_SendInvalid();
        return;
    }

    len = strlen(command);
    if (len > 0U && command[len - 1U] == '@') {
        command[len - 1U] = '\0';
    }

    if (strcmp(command, "save") == 0) {
        Stepper_SaveConfig();
    } else if (strncmp(command, "dir", 3) == 0) {
        Stepper_HandleDir(command);
    } else if (strncmp(command, "sub", 3) == 0) {
        Stepper_HandleSub(command);
    } else if (strncmp(command, "freq", 4) == 0) {
        Stepper_HandleFreq(command);
    } else if (strncmp(command, "start", 5) == 0) {
        Stepper_HandleStartStop(command, 1U);
    } else if (strncmp(command, "stop", 4) == 0) {
        Stepper_HandleStartStop(command, 0U);
    } else {
        Stepper_SendInvalid();
    }
}

void Set_Subdivision(char *command)
{
    ProcessD2Command(command);
}

void Set_Direction(char *command)
{
    ProcessD2Command(command);
}

void Set_Frequency(char *command)
{
    ProcessD2Command(command);
}

void Start_StepperMotor(void)
{
    Stepper_StartMotor(1U);
}

void Stop_StepperMotor(void)
{
    Stepper_StopMotor(1U);
}

/*
 * 电机模块初始化: 四路默认 64 细分、9000Hz、停止状态。
 */
void StepperMotor_Init(void)
{
    uint8_t i;
    StepperMotor *motor;

    for (i = 1U; i <= STEPPER_COUNT; i++) {
        AppMotorConfig *cfg = Stepper_GetConfig(i);
        uint8_t subdivision = cfg->subdivision;
        uint32_t frequency = cfg->frequency_hz;

        if (!Stepper_IsValidSubdivision(subdivision)) {
            subdivision = STEPPER_DEFAULT_SUB;
            cfg->subdivision = subdivision;
        }
        if (frequency < STEPPER_MIN_FREQ_HZ || frequency > STEPPER_MAX_FREQ_HZ) {
            frequency = STEPPER_DEFAULT_FREQ_HZ;
            cfg->frequency_hz = frequency;
        }

        motor = Stepper_GetMotor(i);
        Stepper_SetSubdivisionPins(motor, subdivision);
        Stepper_SetDirectionPin(motor, cfg->direction ? 1U : 0U);
        Stepper_ApplyFrequencyHz(motor, frequency);
        HAL_TIM_PWM_Stop(motor->htim, motor->channel);
        HAL_GPIO_WritePin(motor->en_port, motor->en_pin, GPIO_PIN_SET);
        motor->running = 0U;
    }

    snprintf(result_code, CMD_BUFFER_SIZE, "D2OK:init motors=4 sub=%u freq=%luHz\r\n", stepper_motors[0].subdivision, (unsigned long)stepper_motors[0].frequency_hz);
    Stepper_SendText(result_code);
}

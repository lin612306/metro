
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
#define CO2_OUTPUT_MIN        0.0f

static uint32_t co2_init_tick = 0UL;
static uint32_t co2_last_control_tick = 0UL;
static int32_t co2_latest_ppm = -1;
static float co2_integral = 0.0f;

extern char result_code[CMD_BUFFER_SIZE];

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

static void CO2_SetOutput(float duty_percent)
{
    if (duty_percent < CO2_OUTPUT_MIN) {
        duty_percent = CO2_OUTPUT_MIN;
    }
    if (duty_percent > CO2_OUTPUT_MAX) {
        duty_percent = CO2_OUTPUT_MAX;
    }

#if defined(CO2_VALVE_GPIO_Port) && defined(CO2_VALVE_Pin)
    HAL_GPIO_WritePin(CO2_VALVE_GPIO_Port, CO2_VALVE_Pin,
                      duty_percent > 0.0f ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
    (void)duty_percent;
#endif
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
             "D3STATUS:preheat=%u,target=%lu,control=%u,latest=%ld\r\n",
             CO2_IsPreheated(), (unsigned long)co2->target_ppm,
             co2->control_enabled, (long)co2_latest_ppm);
    CO2_SendText(result_code);
}

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
    } else if (strcmp(command, "gettarget") == 0) {
        snprintf(result_code, CMD_BUFFER_SIZE, "D3OK:target:%lu\r\n", (unsigned long)AppConfig_Get()->co2.target_ppm);
        CO2_SendText(result_code);
    } else if (strcmp(command, "startctrl") == 0) {
        if (!CO2_IsPreheated()) {
            CO2_SendText("D3ERR:preheating\r\n");
            return;
        }
        AppConfig_Mutable()->co2.control_enabled = 1U;
        co2_integral = 0.0f;
        CO2_SendText("D3OK:startctrl\r\n");
    } else if (strcmp(command, "stopctrl") == 0) {
        AppConfig_Mutable()->co2.control_enabled = 0U;
        CO2_SetOutput(0.0f);
        CO2_SendText("D3OK:stopctrl\r\n");
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

void CO2_ControlTask(void)
{
    AppCO2Config *co2 = &AppConfig_Mutable()->co2;
    uint32_t now = HAL_GetTick();
    uint32_t measured = 0UL;
    float error;
    float output;

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
    co2_latest_ppm = -1;
    co2_integral = 0.0f;
    CO2_SetOutput(0.0f);
    CO2_SendText("D3OK:init preheat=180s\r\n");
}

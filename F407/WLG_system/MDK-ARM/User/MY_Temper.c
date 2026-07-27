/*
 * 文件: MY_Temper.c
 * 功能: D0/D1 双路温控通信模块。
 * 硬件关系: USART2 连接外部温控器, USART3 连接内部温控器, USART1 连接 Qt 上位机。
 * 设计说明: 按论文硬件总框图, STM32 当前不直接采集 ADC, 也不直接输出加热 PWM。
 */
#include "MY_Temper.h"
#include <stdarg.h>

#define TEMP_REPLY_TIMEOUT_MS 200U
#define TEMP_TX_TIMEOUT_MS    200U

typedef struct
{
    TempChannelId id;
    const char *prefix;          /* 上位机协议前缀: D0/D1 */
    const char *name;            /* 通道名称 */
    fifo_s *rx_fifo;             /* 温控器接收 FIFO */
    fifo_s *tx_fifo;             /* 温控器发送 FIFO */
    AppTempConfig *config;       /* 掉电保存参数 */
} TempController;

static TempController Temp_GetController(TempChannelId id)
{
    AppConfig *cfg = AppConfig_Mutable();

    if (id == TEMP_CHANNEL_EXTERNAL) {
        TempController controller = {
            TEMP_CHANNEL_EXTERNAL,
            "D1",
            "external",
            &uart2_rx_fifo,
            &uart2_tx_fifo,
            &cfg->d1_temp
        };
        return controller;
    }

    {
        TempController controller = {
            TEMP_CHANNEL_INTERNAL,
            "D0",
            "internal",
            &uart3_rx_fifo,
            &uart3_tx_fifo,
            &cfg->d0_temp
        };
        return controller;
    }
}

static void Temp_SendToHost(const TempController *controller, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(result_code, CMD_BUFFER_SIZE, format, args);
    va_end(args);

    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}

static int Temp_IsNumber(const char *text)
{
    uint8_t has_digit = 0U;
    uint8_t has_dot = 0U;

    if (text == NULL || *text == '\0') {
        return 0;
    }

    if (*text == '-' || *text == '+') {
        text++;
    }

    while (*text != '\0') {
        if (isdigit((unsigned char)*text)) {
            has_digit = 1U;
        } else if (*text == '.' && has_dot == 0U) {
            has_dot = 1U;
        } else {
            return 0;
        }
        text++;
    }

    return has_digit != 0U;
}

static int Temp_ParseValue(char *command, const char **value)
{
    char *colon = strchr(command, ':');

    if (colon == NULL || colon[1] == '\0') {
        return 0;
    }

    *value = colon + 1;
    return Temp_IsNumber(*value);
}

static void Temp_StripFrameEnd(char *command)
{
    size_t len;

    if (command == NULL) {
        return;
    }

    len = strlen(command);
    if (len > 0U && command[len - 1U] == '@') {
        command[len - 1U] = '\0';
    }
}

static void Temp_GetCommandName(char *command, char *name, size_t name_size)
{
    char *colon = strchr(command, ':');
    size_t len = (colon == NULL) ? strlen(command) : (size_t)(colon - command);

    if (len >= name_size) {
        len = name_size - 1U;
    }

    memcpy(name, command, len);
    name[len] = '\0';
}

static int Temp_SendAndWait(const TempController *controller,
                            const char *device_command,
                            uint8_t *rx_buf,
                            size_t rx_size,
                            int *out_len,
                            uint32_t timeout_ms)
{
    uint32_t start_time;
    uint32_t tx_start;

    if (controller == NULL || device_command == NULL || rx_buf == NULL ||
        out_len == NULL || rx_size == 0U) {
        return -2;
    }

    *out_len = 0;
    fifo_s_puts(controller->tx_fifo, (const uint8_t *)device_command, strlen(device_command));
    send_data_from_tx_fifo();

    tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(controller->tx_fifo)) {
        if ((HAL_GetTick() - tx_start) > TEMP_TX_TIMEOUT_MS) {
            return -3;
        }
    }

    start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms) {
        if (!fifo_s_is_empty(controller->rx_fifo)) {
            fifo_s_get(controller->rx_fifo, &rx_buf[*out_len]);

            if (rx_buf[*out_len] == '\r' || rx_buf[*out_len] == '\n') {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)rx_size - 1) {
                break;
            }

            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) {
        return -1;
    }

    rx_buf[*out_len] = '\0';
    return 0;
}

static void Temp_BuildDeviceCommand(char *out, size_t out_size, const char *key, const char *value)
{
    if (value == NULL) {
        snprintf(out, out_size, "%s\r", key);
    } else {
        snprintf(out, out_size, "%s%s\r", key, value);
    }
}

static float Temp_ExtractFloat(const uint8_t *response, int len, int *ok)
{
    char buf[CMD_BUFFER_SIZE];
    char *scan;
    char *endptr;
    float value;

    *ok = 0;
    if (response == NULL || len <= 0) {
        return 0.0f;
    }

    if (len >= CMD_BUFFER_SIZE) {
        len = CMD_BUFFER_SIZE - 1;
    }

    memcpy(buf, response, (size_t)len);
    buf[len] = '\0';

    scan = buf;
    while (*scan != '\0') {
        if ((*scan >= '0' && *scan <= '9') || *scan == '-' || *scan == '+') {
            value = (float)strtod(scan, &endptr);
            if (endptr != scan) {
                *ok = 1;
                return value;
            }
        }
        scan++;
    }

    return 0.0f;
}

static void Temp_HandleDeviceAck(const TempController *controller,
                                 const uint8_t *response,
                                 int len,
                                 const char *action)
{
    int error_code;

    if (len <= 0) {
        Temp_SendToHost(controller, "%sERR:%s:no_response\r\n", controller->prefix, action);
        return;
    }

    if (strncmp((const char *)response, "CMD:REPLY=", 10) == 0) {
        error_code = atoi((const char *)&response[10]);
        if (error_code == 1 || error_code == 8) {
            Temp_SendToHost(controller, "%sOK:%s\r\n", controller->prefix, action);
        } else {
            Temp_SendToHost(controller, "%sERR:%s:code%d\r\n", controller->prefix, action, error_code);
        }
        return;
    }

    Temp_SendToHost(controller, "%sOK:%s\r\n", controller->prefix, action);
}

static void Temp_SetDeviceValue(const TempController *controller,
                                char *command,
                                const char *device_key,
                                const char *action,
                                float *config_value)
{
    const char *value;
    char device_command[64];
    uint8_t response[CMD_BUFFER_SIZE];
    int response_len = 0;
    int ret;

    if (!Temp_ParseValue(command, &value)) {
        Temp_SendToHost(controller, "%sERR:%s:bad_value\r\n", controller->prefix, action);
        return;
    }

    Temp_BuildDeviceCommand(device_command, sizeof(device_command), device_key, value);
    ret = Temp_SendAndWait(controller, device_command, response, sizeof(response),
                           &response_len, TEMP_REPLY_TIMEOUT_MS);
    if (ret != 0) {
        Temp_SendToHost(controller, "%sERR:%s:timeout%d\r\n", controller->prefix, action, ret);
        return;
    }

    if (config_value != NULL) {
        *config_value = (float)strtod(value, NULL);
    }

    Temp_HandleDeviceAck(controller, response, response_len, action);
}

static void Temp_ProcessCommand(TempChannelId id, char *command)
{
    TempController controller = Temp_GetController(id);
    char name[32];

    Temp_StripFrameEnd(command);
    Temp_GetCommandName(command, name, sizeof(name));

    if (strcmp(name, "gettemp") == 0) {
        uint8_t response[CMD_BUFFER_SIZE];
        int response_len = 0;
        int ret;
        int ok = 0;
        float temp;

        ret = Temp_SendAndWait(&controller, "TC1:TCACTTEMP?\r", response, sizeof(response),
                               &response_len, TEMP_REPLY_TIMEOUT_MS);
        if (ret != 0) {
            Temp_SendToHost(&controller, "%sERR:gettemp:timeout%d\r\n", controller.prefix, ret);
            return;
        }

        temp = Temp_ExtractFloat(response, response_len, &ok);
        if (ok) {
            Temp_SendToHost(&controller, "%sTEMP:%.2f\r\n", controller.prefix, temp);
        } else {
            Temp_SendToHost(&controller, "%sRAW:%s\r\n", controller.prefix, response);
        }
    } else if (strcmp(name, "settemp") == 0) {
        Temp_SetDeviceValue(&controller, command, "TC1:TCADJTEMP=", "settemp", &controller.config->target_temp);
    } else if (strcmp(name, "setspeed") == 0) {
        Temp_SetDeviceValue(&controller, command, "TC1:TCRAMPSPEED=", "setspeed", NULL);
    } else if (strcmp(name, "pidmode") == 0) {
        Temp_SetDeviceValue(&controller, command, "TC1:TCPIDTYPE=", "pidmode", NULL);
    } else if (strcmp(name, "setp") == 0) {
        Temp_SetDeviceValue(&controller, command, "TC1:TCPIDP=", "setp", &controller.config->kp);
    } else if (strcmp(name, "seti") == 0) {
        Temp_SetDeviceValue(&controller, command, "TC1:TCPIDTI=", "seti", &controller.config->ki);
    } else if (strcmp(name, "setd") == 0) {
        Temp_SetDeviceValue(&controller, command, "TC1:TCPIDTD=", "setd", &controller.config->kd);
    } else if (strcmp(name, "lock") == 0) {
        uint8_t response[CMD_BUFFER_SIZE];
        int response_len = 0;
        int ret = Temp_SendAndWait(&controller, "MEMORY:MEMWP=1\r", response, sizeof(response),
                                   &response_len, TEMP_REPLY_TIMEOUT_MS);
        if (ret != 0) {
            Temp_SendToHost(&controller, "%sERR:lock:timeout%d\r\n", controller.prefix, ret);
        } else {
            Temp_HandleDeviceAck(&controller, response, response_len, "lock");
        }
    } else if (strcmp(name, "unlock") == 0) {
        uint8_t response[CMD_BUFFER_SIZE];
        int response_len = 0;
        int ret = Temp_SendAndWait(&controller, "MEMORY:MEMWP=0\r", response, sizeof(response),
                                   &response_len, TEMP_REPLY_TIMEOUT_MS);
        if (ret != 0) {
            Temp_SendToHost(&controller, "%sERR:unlock:timeout%d\r\n", controller.prefix, ret);
        } else {
            Temp_HandleDeviceAck(&controller, response, response_len, "unlock");
        }
    } else if (strcmp(name, "startctrl") == 0) {
        Temp_SendToHost(&controller, "%sOK:startctrl\r\n", controller.prefix);
    } else if (strcmp(name, "stopctrl") == 0) {
        Temp_SendToHost(&controller, "%sOK:stopctrl\r\n", controller.prefix);
    } else if (strcmp(name, "startAT") == 0) {
        Temp_SendToHost(&controller, "%sERR:startAT:unsupported\r\n", controller.prefix);
    } else if (strcmp(name, "save") == 0) {
        if (AppConfig_Save() == HAL_OK) {
            Temp_SendToHost(&controller, "%sOK:save\r\n", controller.prefix);
        } else {
            Temp_SendToHost(&controller, "%sERR:save\r\n", controller.prefix);
        }
    } else if (strcmp(name, "getcfg") == 0) {
        Temp_SendToHost(&controller, "%sCFG:target=%.2f,Kp=%.3f,Ti=%.3f,Td=%.3f\r\n",
                        controller.prefix,
                        controller.config->target_temp,
                        controller.config->kp,
                        controller.config->ki,
                        controller.config->kd);
    } else {
        Temp_SendToHost(&controller, "%sERR:invalid\r\n", controller.prefix);
    }
}

/* D0 命令入口: 外部温控器, 论文目标一般为 30 摄氏度。 */
void ProcessD0Command(char *command)
{
    Temp_ProcessCommand(TEMP_CHANNEL_INTERNAL, command);
}

void ProcessD1TempCommand(char *command)
{
    Temp_ProcessCommand(TEMP_CHANNEL_EXTERNAL, command);
}

void TemperatureSensor_Init(void)
{
    const AppConfig *cfg = AppConfig_Get();
    char command[32];

    snprintf(command, sizeof(command), "settemp:%.1f", cfg->d0_temp.target_temp);
    Temp_ProcessCommand(TEMP_CHANNEL_INTERNAL, command);

    snprintf(command, sizeof(command), "settemp:%.1f", cfg->d1_temp.target_temp);
    Temp_ProcessCommand(TEMP_CHANNEL_EXTERNAL, command);
}

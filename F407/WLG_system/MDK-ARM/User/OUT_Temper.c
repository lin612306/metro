/*
 * File: OUT_Temper.c
 * Purpose: Legacy external temperature-controller protocol wrapper.
 * It converts host commands such as settemp/gettemp/setp/seti/setd into the serial protocol of the controller.
 * Architecture note: new dual-temperature routing is in MY_Temper.c; avoid enabling two D0 handlers together.
 */
#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_hal.h"              // HAL header for GPIO definitions
#include "OUT_Temper.h"


/* Legacy D0 command dispatcher. It splits the command name before colon and calls the matching temperature action. */
void ProcessD0Command(char* command)
{
    size_t command_len = strlen(command);      // 获取命令的长度

    // 如果最后一个字符是'@'，去掉它
    if (command[command_len - 1] == '@') 
    {
        command[command_len - 1] = '\0';
        command_len--;
    }

    // 查找命令中的冒号分隔符
    char *colon_ptr = strchr(command, ':');
    char prefix[64];  // 用于存储命令前缀
    
    if (colon_ptr != NULL)
    {
        // 提取命令的前缀
        size_t prefix_len = colon_ptr - command;
        strncpy(prefix, command, prefix_len);
        prefix[prefix_len] = '\0';
    }
    else
    {
        // 如果没有冒号，前缀就是整个命令
        strcpy(prefix, command);
    }

    // 判断命令前缀并调用对应的函数
    if (strcmp(prefix, "settemp") == 0)
    {
        SetTemperature(command);  // 设置温度命令
    }
    else if (strcmp(prefix, "gettemp") == 0)
    {
        GetCurrentTemperature(command);  // 获取当前温度命令
    }
    else if (strcmp(prefix, "setspeed") == 0)
    {
        SetSpeed(command);  // 设置速度命令
    }
    else if (strcmp(prefix, "setp") == 0)
    {
        SetP(command);  // 设置PID的P参数命令
    }
    else if (strcmp(prefix, "seti") == 0)
    {
        SetI(command);  // 设置PID的I参数命令
    }
    else if (strcmp(prefix, "setd") == 0)
    {
        SetD(command);  // 设置PID的D参数命令
    }
    else if (strcmp(prefix, "pidmode") == 0)
    {
        PidMode(command);  // 设置PID模式命令
    }
    else if (strcmp(prefix, "lock") == 0)
    {
        Lock(command);  // 锁定命令
    }
    else if (strcmp(prefix, "unlock") == 0)
    {
        Unlock(command);  // 解锁命令
    }
    else
    {
        // 处理无效命令
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));
        send_data_from_tx_fifo();
    }
}





/* Translate raw controller response bytes into the shared result_code text returned to Qt. */
static void ProcessResponse(uint8_t *response, uint32_t length) 
{
    // 检查响应长度
    if (length == 0)
    {
        char message[] = "NoResponse\r\n";
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)message, strlen(message));
        send_data_from_tx_fifo();
        return;
    }

    // 检查是否为 CMD:REPLY= 格式的响应
    if (strncmp((char*)response, "CMD:REPLY=", 10) == 0) 
    {
        // 提取错误码（应该是数字字符）
        if (length < 11)
        {
            char message[] = "InvalidResponse\r\n";
            fifo_s_puts(&uart1_tx_fifo, (uint8_t *)message, strlen(message));
            send_data_from_tx_fifo();
            return;
        }
        
        // 提取错误码数字
        int error_code = atoi((char*)&response[10]);
        
        // 根据协议文档中的错误码定义处理响应
        char message[100];

        switch (error_code) 
        {
            case 0:
                strcpy(message, "错误:未找到子模块名称或参数名称\r\n");
                break;
            case 1:
                strcpy(message, "设定命令执行成功\r\n");
                break;
            case 2:
                strcpy(message, "错误:未找到参数名称\r\n");
                break;
            case 3:
                strcpy(message, "错误:命令被禁止\r\n");
                break;
            case 4:
                strcpy(message, "错误:参数值超出范围\r\n");
                break;
            case 5:
                strcpy(message, "错误:其它或未知错误\r\n");
                break;
            case 6:
                strcpy(message, "错误:命令格式语法错误\r\n");
                break;
            case 7:
                strcpy(message, "错误:校验错误\r\n");
                break;
            case 8:
                strcpy(message, "保存命令执行成功\r\n");
                break;
            default:
                sprintf(message, "未知错误码:%d\r\n", error_code);
                break;
        }

        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)message, strlen(message));
        send_data_from_tx_fifo();
        return;
    }
    else 
    {
        // 其他响应直接转发给上位机
        
        // 添加回 \r\n 结束符（因为前面去掉了 \r）
        char forward_message[CMD_BUFFER_SIZE + 2];
        snprintf(forward_message, sizeof(forward_message), "%s\n", (char*)response);
        
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)forward_message, strlen(forward_message));
        send_data_from_tx_fifo();
        return;
    }
}




/* Send the lock command to prevent local or external changes while the experiment is controlled by STM32. */
static int Temp_Send_SetLock(uint8_t *rx_buf, size_t buf_size, int *out_len, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0) 
    return -2; 

    const char lock_command[] = "MEMORY:MEMWP=1\r";
    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)lock_command, strlen(lock_command));
    send_data_from_tx_fifo();

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
        return -3;
    }
    
    *out_len = 0;
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
    return -1; // 超时/未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}




/*
 * 功能: lock
 * 描述: 锁定温控器并设置写保护
 * 输入:
 *  - 命令 "D0lock@"
 *      - 锁定并设置保护
 */
void Lock(char *command)
{
    uint8_t response1[CMD_BUFFER_SIZE];
    int len1 = 0;
    int ret;

    // 1) 调用底层设定函数
    ret = Temp_Send_SetLock(response1, sizeof(response1), &len1, 200);
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetError: %d\r\n",ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo();
        return; // 设定阶段失败，则不上保存阶段
    }
    // 上层解析 response1
    ProcessResponse(response1, len1);

    // 清理缓冲区
    memset(response1, 0, sizeof(response1));    
}






/* Send the unlock command so the controller can accept manual/local operations again. */
static int Temp_Send_SetUnlock(uint8_t *rx_buf, size_t buf_size, int *out_len, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0) 
    return -2; // 参数错误

    const char unlock_command[] = "MEMORY:MEMWP=0\r";
    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)unlock_command, strlen(unlock_command));  
    send_data_from_tx_fifo(); // 发送解锁命令

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
        return -3; 
    }

    *out_len = 0;
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
    return -1; // 超时或未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}





/*
 * 功能: unlock
 * 描述: 解锁并解除写保护
 * 输入:
 *  - 命令 "D0unlock@"
 *      - 解锁并解除写保护
 */
void Unlock(char *command)
{
    uint8_t response1[CMD_BUFFER_SIZE] = {0};
    int len1 = 0;
    int ret;

    // 1) 调用底层解锁设置命令
    ret = Temp_Send_SetUnlock(response1, sizeof(response1), &len1, 200);
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetUnlockError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return; // 解锁设置失败，则不上保存
    }

    // 上层解析第一个响应
    ProcessResponse(response1, len1);

    // 清理缓冲区
    memset(response1, 0, sizeof(response1));    
}





/* Build and send the target-temperature command to the external temperature controller. */
static int Temp_Send_SetTemp(uint8_t *rx_buf, size_t buf_size, int *out_len, const char *temperature, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0 || temperature == NULL) 
    return -2; // 参数错误

    // 构建命令： "TC1:TCADJTEMP=X"
    char command_to_temp[50] = "TC1:TCADJTEMP=";
    strcat(command_to_temp, temperature);
    strcat(command_to_temp, "\r");

    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)command_to_temp, strlen(command_to_temp));  
    send_data_from_tx_fifo(); 

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
        return -3; 
    }

    *out_len = 0;
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
    return -1; // 超时或未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}



/* 
* 功能: SetTemperature 
* 描述: 设置温控器的温度 
* 输入: * - 命令 "D0settemp:X@" 
* - X 是温度值，例如 "37" 表示设置温度为37℃ 
*/
/* Validate host temperature text, then forward it to Temp_Send_SetTemp. */
void SetTemperature(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (!colon_pos)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *temperature_str = colon_pos + 1;

    // 检查数字格式
    int has_digit = 0, has_dot = 0;
    for (int i = 0; i < strlen(temperature_str); i++)
    {
        if (isdigit(temperature_str[i])) 
            has_digit = 1;
        else if (temperature_str[i] == '.' && !has_dot) 
            has_dot = 1;
        else
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
            send_data_from_tx_fifo(); 
            return;
        }
    }
    if (!has_digit)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    uint8_t response1[CMD_BUFFER_SIZE] = {0};
    int len1 = 0;
    int ret;

    // 传递温度值给底层函数
    ret = Temp_Send_SetTemp(response1, sizeof(response1), &len1, temperature_str, 200);
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetTempError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    // 上层解析响应
    ProcessResponse(response1, len1);

    // 清理缓冲区
    memset(response1, 0, sizeof(response1));
}





/* Request current temperature from the controller and wait for a bounded-time reply. */
static int Temp_Send_GetTemp(uint8_t *rx_buf, size_t buf_size, int *out_len, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0) 
    return -2; // 参数错误

    const char command_to_temp[] = "TC1:TCACTTEMP?\r";
    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)command_to_temp, strlen(command_to_temp));
    send_data_from_tx_fifo();

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
        return -3; 
    }

    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
    return -1; // 超时或未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}





/*
 * 功能: GetCurrentTemperature
 * 描述: 获取当前温控器的温度
 * 输入:
 *  - 命令 "D0gettemp@"
 */
/* Handle gettemp and return the measured temperature text to the Qt upper computer. */
void GetCurrentTemperature(char* command)
{
    uint8_t response[CMD_BUFFER_SIZE] = {0};
    int len = 0;
    int ret;

    // 1) 调用底层函数获取当前温度
    ret = Temp_Send_GetTemp(response, sizeof(response), &len, 200);
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "GetTempError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo();
        return; // 获取温度失败，不处理响应
    }

    // 2) 调用统一的响应处理函数
    ProcessResponse(response, len);

    // 3) 清理缓冲区
    memset(response, 0, sizeof(response));
}





static int Temp_Send_SetSpeed(uint8_t *rx_buf, size_t buf_size, int *out_len, const char *speed, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0 || speed == NULL) 
    return -2; // 参数错误

    // 构建命令： "TC1:TCRAMPSPEED=X"
    char command_to_speed[50] = "TC1:TCRAMPSPEED=";
    strcat(command_to_speed, speed);
    strcat(command_to_speed, "\r");

    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)command_to_speed, strlen(command_to_speed)); 
    send_data_from_tx_fifo(); 

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
        return -3; 
    }

    *out_len = 0;
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
    return -1; // 超时或未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}




/*
 * 功能: SetSpeed
 * 描述: 设置温控器的速度
 * 输入:
 *  - 命令 "D0setspeed:X@"
 *      - X 是速度值，例如 "0.1" 表示设置速度为0.1
 */
void SetSpeed(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (!colon_pos)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *speed_str = colon_pos + 1;

    // 检查是否为数字或小数点
    for (int i = 0; i < strlen(speed_str); i++)
    {
        if (!isdigit(speed_str[i]) && speed_str[i] != '.')
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
            send_data_from_tx_fifo(); 
            return;
        }
    }

    uint8_t response1[CMD_BUFFER_SIZE] = {0};
    int len1 = 0;
    int ret;

    // 传递速度值给底层函数
    ret = Temp_Send_SetSpeed(response1, sizeof(response1), &len1, speed_str, 200);
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetSpeedError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    // 上层解析响应
    ProcessResponse(response1, len1);

    // 清理缓冲区
    memset(response1, 0, sizeof(response1));
}





/* Send the proportional parameter to the temperature controller. */
static int Temp_Send_SetPidMode(uint8_t *rx_buf, size_t buf_size, int *out_len, const char *mode, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0 || mode == NULL) 
        return -2; // 参数错误

    // 构建命令： "TC1:TCPIDTYPE=X"
    char command_to_pid[50] = "TC1:TCPIDTYPE=";
    strcat(command_to_pid, mode);
    strcat(command_to_pid, "\r");

    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)command_to_pid, strlen(command_to_pid));
    send_data_from_tx_fifo();  // 发送命令

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
            return -4; 
    }

    *out_len = 0;
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
        return -1; // 超时或未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}


/*
 * 功能: PidMode
 * 描述: 设置PID控制器模式
 * 输入:
 *  - 命令 "D0pidmode:X@"
 *      - 0 代表 "P" 模式
 *      - 1 代表 "PI" 模式
 *      - 2 代表 "PID" 模式
 */
void PidMode(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *mode_str = colon_pos + 1;

    // 检查模式是否合法
    if (strcmp(mode_str, "0") != 0 && strcmp(mode_str, "1") != 0 && strcmp(mode_str, "2") != 0)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    uint8_t response1[CMD_BUFFER_SIZE] = {0};
    int len1 = 0;
    int ret;

    // 传递模式值给底层函数
    ret = Temp_Send_SetPidMode(response1, sizeof(response1), &len1, mode_str, 200); // 200ms超时
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetPidModeError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    // 上层解析响应
    ProcessResponse(response1, len1);

    // 清理缓冲区
    memset(response1, 0, sizeof(response1));
}





static int Temp_Send_SetP(uint8_t *rx_buf, size_t buf_size, int *out_len, const char *p_value, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0 || p_value == NULL) 
        return -2; // 参数错误

    // 构建命令： "TC1:TCPIDP=X"
    char command_to_p[50] = "TC1:TCPIDP=";
    strcat(command_to_p, p_value);
    strcat(command_to_p, "\r");

    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)command_to_p, strlen(command_to_p)); 
    send_data_from_tx_fifo();  

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
            return -4; 
    }

    *out_len = 0;
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
        return -1; // 超时或未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}






/*
 * 功能: SetP
 * 描述: 设置PID控制器的P参数
 * 输入:
 *  - 命令 "D0setp:X"
 *      - X 是P参数值
 */
void SetP(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *p_value_str = colon_pos + 1;

    // 检查是否是数字
    int has_digit = 0, has_dot = 0;
    for (int i = 0; i < strlen(p_value_str); i++)
    {
        if (isdigit(p_value_str[i])) 
            has_digit = 1;
        else if (p_value_str[i] == '.' && !has_dot) 
            has_dot = 1;
        else
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
            send_data_from_tx_fifo(); 
            return;
        }
    }
    if (!has_digit)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    uint8_t response1[CMD_BUFFER_SIZE] = {0};
    int len1 = 0;
    int ret;

    // 传递 P 值给底层函数
    ret = Temp_Send_SetP(response1, sizeof(response1), &len1, p_value_str, 200); // 200ms超时
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetPError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    // 上层解析响应
    ProcessResponse(response1, len1);

    // 清理缓冲区
    memset(response1, 0, sizeof(response1));
}






/* Send the integral/Ti parameter to the temperature controller. */
static int Temp_Send_SetI(uint8_t *rx_buf, size_t buf_size, int *out_len, const char *i_value, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0 || i_value == NULL) 
        return -2; // 参数错误

    // 构建命令： "TC1:TCPIDTI=X"
    char command_to_i[50] = "TC1:TCPIDTI=";
    strcat(command_to_i, i_value);
    strcat(command_to_i, "\r");
    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)command_to_i, strlen(command_to_i));  
    send_data_from_tx_fifo();  

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
            return -4; 
    }

    *out_len = 0;
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
        return -1; // 超时或未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}






/*
 * 函数名: SetI
 * 功能: 设置PID中的I参数
 * 参数:
 *    输入命令字符串: "D0seti:X"
 *      - X为I参数的值
 */
void SetI(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *i_value_str = colon_pos + 1;

    // 检查是否全是数字
    int has_digit = 0, has_dot = 0;
    for (int i = 0; i < strlen(i_value_str); i++)
    {
        if (isdigit(i_value_str[i])) 
            has_digit = 1;
        else if (i_value_str[i] == '.' && !has_dot) 
            has_dot = 1;
        else
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
            send_data_from_tx_fifo(); 
            return;
        }
    }
    if (!has_digit)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }


    uint8_t response1[CMD_BUFFER_SIZE] = {0};
    int len1 = 0;
    int ret;

    // 传递 I 值给底层函数
    ret = Temp_Send_SetI(response1, sizeof(response1), &len1, i_value_str, 200); // 200ms超时
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetIError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    // 上层解析响应
    ProcessResponse(response1, len1);

    // 清理缓冲区
    memset(response1, 0, sizeof(response1));
}






/* Send the derivative/Td parameter to the temperature controller. */
static int Temp_Send_SetD(uint8_t *rx_buf, size_t buf_size, int *out_len, const char *d_value, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0 || d_value == NULL) 
        return -2; // 参数错误

    // 构建命令： "TC1:TCPIDTD=X"
    char command_to_d[50] = "TC1:TCPIDTD=";
    strcat(command_to_d, d_value);
    strcat(command_to_d, "\r");

    fifo_s_puts(&uart3_tx_fifo, (uint8_t*)command_to_d, strlen(command_to_d));
    send_data_from_tx_fifo();  

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart3_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
            return -3; 
    }

    *out_len = 0;
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart3_rx_fifo))  // 改成is_empty判断
        {
            fifo_s_get(&uart3_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            // 检查是否为结束符
            if (rx_buf[*out_len] == '\r')
            {
                (*out_len)++;
                break;
            }

            (*out_len)++;
            if (*out_len >= (int)buf_size - 1)
                break; // 防止溢出

            // 重置超时计时（只要有新数据）
            start_time = HAL_GetTick();
        }
    }

    if (*out_len == 0) 
        return -1; // 超时或未收到数据
    rx_buf[*out_len] = '\0'; // 添加字符串终止符
    return 0; // 成功
}







/*
 * 函数名: SetD
 * 功能: 设置PID中的D参数
 * 参数:
 *    输入命令字符串: "D0setd:X"
 *      - X为D参数的值
 */
void SetD(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *d_value_str = colon_pos + 1;

    // 检查是否全是数字
    int has_digit = 0, has_dot = 0;
    for (int i = 0; i < strlen(d_value_str); i++)
    {
        if (isdigit(d_value_str[i])) 
            has_digit = 1;
        else if (d_value_str[i] == '.' && !has_dot) 
            has_dot = 1;
        else
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
            send_data_from_tx_fifo(); 
            return;
        }
    }
    if (!has_digit)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }


    uint8_t response1[CMD_BUFFER_SIZE] = {0};
    int len1 = 0;
    int ret;

    // 传递 D 值给底层函数
    ret = Temp_Send_SetD(response1, sizeof(response1), &len1, d_value_str, 200); // 200ms超时
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetDError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    // 上层解析响应
    ProcessResponse(response1, len1);

    // 清理缓冲区
    memset(response1, 0, sizeof(response1));
}





/* Reserved initialization entry for the legacy external temperature module. */
void TemperatureSensor_Init(void)
{
    // 1. 设置温度为默认值
    SetTemperature("settemp:37");

    // 2. 获取当前温度值
    uint8_t response[CMD_BUFFER_SIZE] = {0};
    int len = 0;
    int ret;

    ret = Temp_Send_GetTemp(response, sizeof(response), &len, 200);
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "GetTempError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;  // 获取当前温度失败，退出
    }

    // 3. 打印
    snprintf(result_code, sizeof(result_code), "Temperature Sensor Initialized successfully. Current Temperature: %s°C\r\n", response);
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}


#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_hal.h"              // HAL header for GPIO definitions
#include "MY_CO2.h"


/**
 * @brief  计算CO2传感器校验和
 * @param  packet: 数据包指针
 * @retval 校验和
 */
uint8_t CO2_CalculateChecksum(uint8_t *packet)
{
    uint8_t i, checksum = 0;
    
    // 从Byte1累加到Byte7
    for(i = 1; i < 8; i++)
    {
        checksum += packet[i];
    }
    
    // 取反加1
    checksum = 0xFF - checksum;
    checksum += 1;
    
    return checksum;
}



void ProcessD3Command(char* command)
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
    if (strcmp(prefix, "getco2") == 0)
    {
        GetCurrentCO2(command);  // 获取当前CO2浓度命令
    }
    else if (strcmp(prefix, "setbase") == 0)
    {
        SetCo2Base(command);  // 设置基准CO2浓度命令
    }
    else
    {
        // 处理无效命令
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));
        send_data_from_tx_fifo();
    }
}




static int Send_GetCO2(uint8_t *rx_buf, size_t buf_size, int *out_len, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0) 
    return -2; // 参数错误

    uint8_t cmd[9] = {0};
    // 构造读取命令
    cmd[0] = MH4R_START_BYTE;   // 起始字节
    cmd[1] = MH4R_SENSOR_ID;    // 传感器编号
    cmd[2] = MH4R_CMD_READ_GAS; // 命令
    cmd[3] = 0x00;              // 保留位
    cmd[4] = 0x00;              // 保留位
    cmd[5] = 0x00;              // 保留位
    cmd[6] = 0x00;              // 保留位
    cmd[7] = 0x00;              // 保留位
    cmd[8] = 0x79;              // 校验和

    uint8_t checksum1 = CO2_CalculateChecksum(cmd);
    if (checksum1 != cmd[8])
    {
        return -3; // 校验和错误
    }

    fifo_s_puts(&uart4_tx_fifo, cmd, sizeof(cmd));
    send_data_from_tx_fifo();

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart4_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
        return -4; 
    }

    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        if (!fifo_s_is_empty(&uart4_rx_fifo))  // 判断接收缓冲区是否为空
        {
            fifo_s_get(&uart4_rx_fifo, &rx_buf[*out_len]);  // 取出1字节

            (*out_len)++;
            if (*out_len == 9)  // 等待接收到9字节数据
            {
                // 校验和检查
                uint8_t checksum2 = CO2_CalculateChecksum(rx_buf);

                if (checksum2 != rx_buf[8])
                {
                    return -5;//校验和错误
                }
                break;  // 接收到有效数据后跳出循环
            }

            // 防止溢出
            if (*out_len >= (int)buf_size - 1)
                break;

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
 * 功能: GetCurrentCO2
 * 描述: 获取当前CO2浓度
 * 输入:
 *  - 命令 "D3getco2@"
 */
void GetCurrentCO2(char* command)
{
    uint8_t response[CMD_BUFFER_SIZE] = {0};
    int len = 0;
    int ret;

    // 1) 调用底层函数获取当前CO2浓度
    ret = Send_GetCO2(response, sizeof(response), &len, 200);
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "GetCO2Error: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo();
        return; // 获取CO2浓度失败，不处理响应
    }

    // 将高位和低位字节分别转为十进制计算
    uint16_t high_val = (uint16_t)response[2];  // 高位字节
    uint16_t low_val  = (uint16_t)response[3];  // 低位字节
    // 高位 ×256 + 低位 ×100 + 400基准线
    uint32_t co2_value = (high_val * 256 + low_val * 100) * 100;

    // 2) 格式化结果代码
    snprintf(result_code, sizeof(result_code), "CO2Concentration: %d ppm\r\n", co2_value);
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();

    // 3) 清空接收缓冲区
    memset(response, 0, sizeof(response));
}






static int Send_SetBase(uint8_t *rx_buf, size_t buf_size, int *out_len, uint32_t timeout_ms)
{
    if (rx_buf == NULL || out_len == NULL || buf_size == 0) 
    return -2; // 参数错误

    uint8_t cmd[9] = {0};
    // 构造零点校准命令
    cmd[0] = MH4R_START_BYTE;         // 起始字节
    cmd[1] = MH4R_SENSOR_ID;          // 传感器编号
    cmd[2] = MH4R_CMD_ZERO_CALIBRATE; // 命令
    cmd[3] = 0x00;                    // 保留位
    cmd[4] = 0x00;                    // 保留位
    cmd[5] = 0x00;                    // 保留位
    cmd[6] = 0x00;                    // 保留位
    cmd[7] = 0x00;                    // 保留位
    cmd[8] = 0x78;                    // 校验和

    uint8_t checksum1 = CO2_CalculateChecksum(cmd);
    if (checksum1 != cmd[8])
    {
        return -3; // 校验和错误
    }

    fifo_s_puts(&uart4_tx_fifo, cmd, sizeof(cmd));
    send_data_from_tx_fifo();

    uint32_t tx_start = HAL_GetTick();
    while (!fifo_s_is_empty(&uart4_tx_fifo))
    {
        if (HAL_GetTick() - tx_start > 200) // 超过200ms超时
        return -3; 
    }

    return 0; // 成功
}





/*
 * 功能: SetCo2Base
 * 描述: 设置CO2浓度基准值
 * 输入:
 *  - 命令 "D3setbase@"
 */
void SetCo2Base(char* command)
{
    uint8_t response[CMD_BUFFER_SIZE] = {0};
    int len = 0;
    int ret;

    // 1) 调用底层函数设置CO2浓度基准值
    ret = Send_SetBase(response, sizeof(response), &len, 200);
    if (ret != 0)
    {
        snprintf(result_code, sizeof(result_code), "SetBaseError: %d\r\n", ret);
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo();
        return; 
    }
    else
    {
        snprintf(result_code, sizeof(result_code), "SetBaseSuccess\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo();
    }

    memset(response, 0, sizeof(response));
}


void CO2Module_Init(void)
{
    // 这里不需要复杂的配置，仅需要提示用户预热时间
    snprintf(result_code, sizeof(result_code), "CO2 module initialized. Please wait for 3 minutes for preheating.\r\n");
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}






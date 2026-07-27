/*
 * File: MY_TF.c
 * Purpose: Legacy TF/storage command wrapper.
 * This file contains early file create/open/delete/read/write commands and float-array storage helpers.
 * Architecture note: current D1 is used by the internal temperature controller, so this legacy D1 entry must not be enabled at the same time.
 */
#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_hal.h"              // HAL header for GPIO definitions
#include "MY_TF.h"

float Arr1[] = { 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9,
                 2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9,
                 3.0, 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9,
                 4.0, 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8, 4.9,
                 5.0, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 5.8, 5.9,
                 6.0, 6.1, 6.2, 6.3, 6.4, 6.5, 6.6, 6.7, 6.8, 6.9,
                 7.0, 7.1, 7.2, 7.3, 7.4, 7.5, 7.6, 7.7, 7.8, 7.9,
                 8.0, 8.1, 8.2, 8.3, 8.4, 8.5, 8.6, 8.7, 8.8, 8.9,
                 9.0, 9.1, 9.2, 9.3, 9.4, 9.5, 9.6, 9.7, 9.8, 9.9,
                 10.0};

float Arr2[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
               10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
               20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
               30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
               40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
               50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
               60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
               70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
               80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
               90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
               100};

#define MAX_ARRAY_MAP_SIZE 100

typedef struct 
{
    char* name;     // 数组名称
    float* array;   // 数组指针
    size_t size;    // 数组大小
} ArrayMapping;

ArrayMapping array_map[MAX_ARRAY_MAP_SIZE] = {
    {"Arr1", Arr1, 100},
    {"Arr2", Arr2, 100},
};
size_t array_map_count = 2;

// 将数组添加到映射表
/* Register a float array by name so later read/write commands can find calibration data by text key. */
int add_array_to_map(const char* name, float* array, size_t size)
{
    if (array_map_count >= MAX_ARRAY_MAP_SIZE) {
        return 0; // 映射表已满
    }
    array_map[array_map_count].name = (char*)name;
    array_map[array_map_count].array = array;
    array_map[array_map_count].size = size;
    array_map_count++;
    return 1; // 添加成功
}

// 根据数组名获取数组
/* Return the registered float array pointer for a given name, or NULL when the name is unknown. */
float* get_array_by_name(char* array_name) 
{
    size_t num_arrays = sizeof(array_map) / sizeof(array_map[0]);
    for (size_t i = 0; i < num_arrays; i++)
    {
        if (strcmp(array_map[i].name, array_name) == 0)
        {
            return array_map[i].array;
        }
    }
    return NULL; // 未找到对应数组
}

// 处理D1命令
/* Legacy TF command dispatcher. Check command-prefix conflicts before reusing it in the current D0/D1/D2/D3 protocol. */
void ProcessD1Command(char* command)
{
    size_t command_len = strlen(command);      // 获取命令长度

    // 如果最后一个字符是'@'，去掉它
    if (command[command_len - 1] == '@') 
    {
        command[command_len - 1] = '\0';
        command_len--;
    }

    char *colon_ptr = strchr(command, ':'); 
    if (colon_ptr == NULL || strchr(colon_ptr + 1, ':') != NULL)  // 判断命令格式是否正确
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));
        send_data_from_tx_fifo();
        return;
    }
    
    // 获取命令前缀
    size_t prefix_len = colon_ptr - command;    // 获取命令前缀长度
    char prefix[prefix_len + 1];                // 存储命令前缀
    strncpy(prefix, command, prefix_len);       // 复制前缀
    prefix[prefix_len] = '\0';                  // 添加字符串结束符 '\0'

    // 判断前缀并执行相应操作
    if (strcmp(prefix, "CreateFile") == 0)
    {
        CreateFile(command);
    }
    else if (strcmp(prefix, "CreateDocument") == 0)
    {
        CreateDocument(command);
    }
    else if (strcmp(prefix, "OpenFile") == 0)
    {
        OpenFile(command);
    }
    else if (strcmp(prefix, "OpenDocument") == 0)
    {
        OpenDocument(command);
    }
    else if (strcmp(prefix, "DeleteFile") == 0)
    {
        DeleteFile(command);
    }
    else if (strcmp(prefix, "DeleteDocument") == 0)
    {
        DeleteDocument(command);
    }
    else if (strcmp(prefix, "Read") == 0)
    {
        Read(command);
    }
    else if (strcmp(prefix, "Write1") == 0)
    {
        Write1(command);
    }
    else if (strcmp(prefix, "Write2") == 0)
    {
        Write2(command);
    }
    else if (strcmp(prefix, "Write3") == 0)
    {
        Write3(command);
    }
    else
    {
        // 未知命令
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));
        send_data_from_tx_fifo();
    }
}

/* Build and send the create-file packet to the TF module, then translate the module response into result_code text. */
char* TF_SendCreateFile(uint8_t* data)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_RESET);  // 指定TF卡为接收模式
    HAL_Delay(20);

    // 使用UART2与TF卡通信
    fifo_s_puts(&uart2_tx_fifo, data, 9);  
    send_data_from_tx_fifo();

    uint8_t response[3] = {0};
    int received_count = 0;
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时
    
    while (received_count < 3) 
    {
        if ((HAL_GetTick() - start_time) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }
        
        if (!fifo_s_is_empty(&uart2_rx_fifo))
        {
            fifo_s_get(&uart2_rx_fifo, &response[received_count]);
            received_count++;
        }
    }

    // 判断TF卡响应结果
    if (response[0] == 0xC1 && response[1] == 0x01 && response[2] == 0xC2)
    {
        strcpy(result_code, "创建成功\r\n");
    }
    else if (response[0] == 0xC1 && response[1] == 0x05 && response[2] == 0xC2)
    {
        strcpy(result_code, "创建失败\r\n");
    }
    else if (response[0] == 0xC1 && response[1] == 0x07 && response[2] == 0xC2)
    {
        strcpy(result_code, "重复创建\r\n");
    }
    else
    {
        strcpy(result_code, "InvalidCommand\r\n");
    }

    return result_code;
}



/* Parse the CreateFile command from the host and call TF_SendCreateFile with validated parameters. */
void CreateFile(char* command)
{
    char* colon_pos = strchr(command, ':'); // 找到冒号的位置
    if (colon_pos == NULL) 
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
        send_data_from_tx_fifo();
        return;
    }
    
    char* number_str = colon_pos + 1;     // 获取冒号后的参数
    
    for (int i = 0; i < 8; i++)      // 检查是否全为数字
    {
        if (isdigit(number_str[i]) == 0) 
        {
            // 如果含有非数字字符返回"InvalidCommand"
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); 
            send_data_from_tx_fifo();
            return;
        }
    }

    // 检查长度是否为8
    if (strlen(number_str) != 8) 
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
        send_data_from_tx_fifo();
        return;
    }

    // 准备数据包首字节为0xE0
    unsigned char data[9] = {0xE0};  // 0xE0为数据包的第一个字节
    
    // 将每个字符的ASCII值填入data数组
    for (int i = 0; i < 8; i++) 
    {
        data[i + 1] = (unsigned char)(number_str[i]);
    }

    // 调用发送函数
    strcpy(result_code, TF_SendCreateFile(data));
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
    send_data_from_tx_fifo();
}




char* TF_SendCreateDocument(uint8_t* data1, uint8_t* data2)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_RESET);  // 指定TF卡为接收模式
    HAL_Delay(20);

    // 使用UART2与TF卡通信
    fifo_s_puts(&uart2_tx_fifo, data1, 9);  
    send_data_from_tx_fifo();

    uint8_t response1[3] = {0};
    int received_count = 0;
    uint32_t start_time1 = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时

    // 非阻塞接收TF卡响应
    while (received_count < 3)
    {
        if ((HAL_GetTick() - start_time1) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }

        if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
        {
            fifo_s_get(&uart2_rx_fifo, &response1[received_count]);  
            received_count++;
        }
    }

    // 判断TF卡响应
    if (response1[0] == 0xC1 && response1[1] == 0xD0 && response1[2] == 0xC2)
    {
        HAL_Delay(50);  // 等待一会再发送第二个数据包
        fifo_s_puts(&uart2_tx_fifo, data2, 9);   
        send_data_from_tx_fifo();

        uint8_t response2[3] = {0};
        received_count = 0;
        uint32_t start_time2 = HAL_GetTick();

        // 非阻塞接收第二个响应
        while (received_count < 3)
        {
            if ((HAL_GetTick() - start_time2) > timeout)
            {
                strcpy(result_code, "接收超时\r\n");
                return result_code;
            }
            if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
            {
                fifo_s_get(&uart2_rx_fifo, &response2[received_count]);  
                received_count++;
            }
        }

        // 判断响应数据，设置result_code
        if (response2[0] == 0xC1 && response2[1] == 0xD1 && response2[2] == 0xC2)
        {
            strcpy(result_code, "创建成功\r\n");
        }
        else if (response2[0] == 0xC1 && response2[1] == 0x04 && response2[2] == 0xC2)
        {
            strcpy(result_code, "存储失败\r\n");
        }
        else if (response2[0] == 0xC1 && response2[1] == 0x05 && response2[2] == 0xC2)
        {
            strcpy(result_code, "指令错误\r\n");
        }
        else if (response2[0] == 0xC1 && response2[1] == 0x07 && response2[2] == 0xC2)
        {
            strcpy(result_code, "重复创建\r\n");
        }
        else
        {
            strcpy(result_code, "InvalidCommand\r\n");
        }
    }
    else if (response1[0] == 0xC1 && response1[1] == 0x04 && response1[2] == 0xC2)
    {
        strcpy(result_code, "创建失败\r\n");
    }
    else if (response1[0] == 0xC1 && response1[1] == 0x05 && response1[2] == 0xC2)
    {
        strcpy(result_code, "指令错误\r\n");
    }
    else
    {
        strcpy(result_code, "InvalidCommand\r\n");
    }

    return result_code;
}





void CreateDocument(char* command)
{
    // 检查冒号和逗号格式
    char* colon_pos = strchr(command, ':');
    char* comma_pos = strchr(command, ',');
    if (colon_pos == NULL || comma_pos == NULL || strchr(comma_pos + 1, ',') != NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
        send_data_from_tx_fifo();
        return;
    }

    *comma_pos = '\0';  // 分割字符串
    char* first_str = colon_pos + 1;
    char* second_str = comma_pos + 1;

    if (strlen(first_str) != 8 || strlen(second_str) != 8)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
        send_data_from_tx_fifo();
        return;
    }

    for (int i = 0; i < 8; i++)
    {
        if (!isdigit((unsigned char)first_str[i]) || !isdigit((unsigned char)second_str[i]))
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
            send_data_from_tx_fifo();
            return;
        }
    }

    // 准备数据包
    uint8_t data1[9] = {0xE5};
    uint8_t data2[9] = {0xE6};

    for (int i = 0; i < 8; i++)
    {
        data1[i + 1] = (uint8_t)first_str[i];
        data2[i + 1] = (uint8_t)second_str[i];
    }

    // 调用底层发送和接收
    strcpy(result_code, TF_SendCreateDocument(data1, data2));

    // 通过串口1返回PC
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
    send_data_from_tx_fifo();
}




char* TF_SendOpenFile(uint8_t* data)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_RESET);  // 指定TF卡为接收模式
    HAL_Delay(20);

    // 使用UART2与TF卡通信
    fifo_s_puts(&uart2_tx_fifo, data, 9);  
    send_data_from_tx_fifo();

    uint8_t response[3] = {0};
    int received_count = 0;
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时

    // 非阻塞接收TF卡3字节响应
    while (received_count < 3)
    {
        if ((HAL_GetTick() - start_time) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }

        if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
        {
            fifo_s_get(&uart2_rx_fifo, &response[received_count]);  // 读取1字节
            received_count++;
        }
    }

    // 判断TF卡响应结果
    if (response[0] == 0xC1 && response[1] == 0x01 && response[2] == 0xC2)
    {
        strcpy(result_code, "打开失败，未找到文件或文件损坏\r\n");
    }
    else if (response[0] == 0xC1 && response[1] == 0x05 && response[2] == 0xC2)
    {
        strcpy(result_code, "指令错误\r\n");
    }
    else if (response[0] == 0xC1 && response[1] == 0x07 && response[2] == 0xC2)
    {
        strcpy(result_code, "打开成功\r\n");
    }
    else
    {
        strcpy(result_code, "InvalidCommand\r\n");
    }

    return result_code;
}





void OpenFile(char* command)
{
    char* colon_pos = strchr(command, ':'); // 找到冒号的位置
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
        send_data_from_tx_fifo();
        return;
    }

    char* number_str = colon_pos + 1;

    // 检查是否为数字并且长度为8
    for (int i = 0; i < 8; i++)
    {
        if (!isdigit(number_str[i]))
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
            send_data_from_tx_fifo();
            return;
        }
    }

    if (strlen(number_str) != 8)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
        send_data_from_tx_fifo();
        return;
    }

    // 准备数据包
    uint8_t data[9] = {0xE0};
    for (int i = 0; i < 8; i++)
    {
        data[i + 1] = (uint8_t)(number_str[i]);
    }

    // 调用发送函数
    strcpy(result_code, TF_SendOpenFile(data));
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  
    send_data_from_tx_fifo();
}




char* TF_SendOpenDocument(uint8_t* data1, uint8_t* data2)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_RESET);  // 指定TF卡为接收模式
    HAL_Delay(20);

    // 使用UART2与TF卡通信
    fifo_s_puts(&uart2_tx_fifo, data1, 9); 
    send_data_from_tx_fifo();

    uint8_t response1[3] = {0};
    int received_count = 0;
    uint32_t start_time1 = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时

    // 非阻塞接收TF卡3字节响应
    while (received_count < 3)
    {
        if ((HAL_GetTick() - start_time1) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }
        if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
        {
            fifo_s_get(&uart2_rx_fifo, &response1[received_count]);  // 读取1字节
            received_count++;
        }
    }

    if (response1[0] == 0xC1 && response1[1] == 0xD0 && response1[2] == 0xC2)
    {
        HAL_Delay(50);  // 等待一会再发送第二个数据包
        fifo_s_puts(&uart2_tx_fifo, data2, 9);  
        send_data_from_tx_fifo();

        uint8_t response2[3] = {0};
        received_count = 0;
        uint32_t start_time2 = HAL_GetTick();

        // 非阻塞接收第二个响应
        while (received_count < 3)
        {
            if ((HAL_GetTick() - start_time2) > timeout)
            {
                strcpy(result_code, "接收超时\r\n");
                return result_code;
            }
            if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
            {
                fifo_s_get(&uart2_rx_fifo, &response2[received_count]);  // 读取1字节
                received_count++;
            }
        }

        // 判断响应数据，设置result_code
        if (response2[0] == 0xC1 && response2[1] == 0xD1 && response2[2] == 0xC2)
        {
            strcpy(result_code, "打开成功\r\n");
        }
        else if (response2[0] == 0xC1 && response2[1] == 0x04 && response2[2] == 0xC2)
        {
            strcpy(result_code, "存储失败\r\n");
        }
        else if (response2[0] == 0xC1 && response2[1] == 0x05 && response2[2] == 0xC2)
        {
            strcpy(result_code, "指令错误\r\n");
        }
        else if (response2[0] == 0xC1 && response2[1] == 0x07 && response2[2] == 0xC2)
        {
            strcpy(result_code, "打开成功\r\n");
        }
        else
        {
            strcpy(result_code, "InvalidCommand\r\n");
        }
    }
    else if (response1[0] == 0xC1 && response1[1] == 0x04 && response1[2] == 0xC2)
    {
        strcpy(result_code, "打开失败\r\n");
    }
    else if (response1[0] == 0xC1 && response1[1] == 0x05 && response1[2] == 0xC2)
    {
        strcpy(result_code, "指令错误\r\n");
    }
    else
    {
        strcpy(result_code, "InvalidCommand\r\n");
    }

    return result_code;
}





void OpenDocument(char* command)
{
    // 检查命令格式是否正确
    char* comma_pos = strchr(command, ',');
    if (comma_pos == NULL || strchr(comma_pos + 1, ',') != NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    char* colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    *comma_pos = '\0';  // 分割字符串
    char* first_str = colon_pos + 1;
    char* second_str = comma_pos + 1;

    // 检查两个参数是否都是8位数字
    for (int i = 0; i < 8; i++)
    {
        if (!isdigit(first_str[i]) || !isdigit(second_str[i]))
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
            send_data_from_tx_fifo();
            return;
        }
    }

    if (strlen(first_str) != 8 || strlen(second_str) != 8)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    // 准备 data1 和 data2
    uint8_t data1[9] = {0xE5};
    uint8_t data2[9] = {0xE6};
    for (int i = 0; i < 8; i++)
    {
        data1[i + 1] = (uint8_t)(first_str[i]);
        data2[i + 1] = (uint8_t)(second_str[i]);
    }

    // 调用发送和接收函数
    strcpy(result_code, TF_SendOpenDocument(data1, data2));
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
    send_data_from_tx_fifo();
}



char* TF_SendDeleteFile(unsigned char* data)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_RESET); // 指定TF卡为接收模式
    HAL_Delay(20);

    unsigned char response[3] = {0};
    int received_count = 0;
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时

    // 使用UART2与TF卡通信
    fifo_s_puts(&uart2_tx_fifo, data, 9); 
    send_data_from_tx_fifo();

    // 非阻塞接收TF卡3字节响应
    while (received_count < 3)
    {
        if ((HAL_GetTick() - start_time) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }
        if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
        {
            fifo_s_get(&uart2_rx_fifo, &response[received_count]);  // 读取1字节
            received_count++;
        }
    }

    // 判断TF卡响应结果
    if (response[0] == 0xC1 && response[1] == 0x08 && response[2] == 0xC2)
    {
        strcpy(result_code, "删除成功\r\n");
    }
    else
    {
        strcpy(result_code, "删除失败\r\n");
    }

    return result_code;
}






void DeleteFile(char* command)
{
    char* colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }
    char* file_id = colon_pos + 1;

    // 检查参数长度是否为8
    if (strlen(file_id) != 8)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    // 检查是否全为数字
    for (int i = 0; i < 8; i++)
    {
        if (!isdigit(file_id[i]))
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
            send_data_from_tx_fifo();
            return;
        }
    }

    // 准备数据包首字节为0xE2
    unsigned char data[9] = {0xE2};
    for (int i = 0; i < 8; i++)
    {
        data[i + 1] = (unsigned char)file_id[i];
    }

    // 调用发送和接收函数
    strcpy(result_code, TF_SendDeleteFile(data));
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
    send_data_from_tx_fifo();
}




char* TF_SendDeleteDocument(unsigned char* data1, unsigned char* data2)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_RESET); // 指定TF卡为接收模式
    HAL_Delay(20);

    unsigned char response1[3] = {0};
    unsigned char response2[3] = {0};
    int received_count1 = 0;
    int received_count2 = 0;
    uint32_t start_time1 = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时

    // 使用UART2与TF卡通信
    fifo_s_puts(&uart2_tx_fifo, data1, 9); 
    send_data_from_tx_fifo();

    // 非阻塞接收TF卡3字节响应
    while (received_count1 < 3)
    {
        if ((HAL_GetTick() - start_time1) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }
        if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
        {
            fifo_s_get(&uart2_rx_fifo, &response1[received_count1]);  // 读取1字节
            received_count1++;
        }
    }

    if (response1[0] == 0xC1 && response1[1] == 0x08 && response1[2] == 0xC2)
    {
        HAL_Delay(50); // 等待50ms

        fifo_s_puts(&uart2_tx_fifo, data2, 9); // 非阻塞发送第二个数据包给TF卡
        send_data_from_tx_fifo();
        uint32_t start_time2 = HAL_GetTick();

        // 非阻塞接收第二个响应
        while (received_count2 < 3)
        {
            if ((HAL_GetTick() - start_time2) > timeout)
            {
                strcpy(result_code, "接收超时\r\n");
                return result_code;
            }
            if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
            {
                fifo_s_get(&uart2_rx_fifo, &response2[received_count2]);  // 读取1字节
                received_count2++;
            }
        }

        if (response2[0] == 0xC1 && response2[1] == 0x08 && response2[2] == 0xC2)
        {
            strcpy(result_code, "删除成功\r\n");
        }
        else
        {
            strcpy(result_code, "删除失败\r\n");
        }
    }
    else
    {
        strcpy(result_code, "删除失败\r\n");
    }

    return result_code;
}






void DeleteDocument(char* command)
{
    char* first_comma = strchr(command, ',');
    if (first_comma == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    char* colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    *first_comma = '\0';
    char* first_str = colon_pos + 1;
    char* second_str = first_comma + 1;

    if (strlen(first_str) != 8 || strlen(second_str) != 8)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        return;
    }

    for (int i = 0; i < 8; i++)
    {
        if (!isdigit(first_str[i]) || !isdigit(second_str[i]))
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
            send_data_from_tx_fifo();
            return;
        }
    }

    unsigned char data1[9] = {0xE7};
    for (int i = 0; i < 8; i++)
    {
        data1[i + 1] = (unsigned char)first_str[i];
    }

    unsigned char data2[9] = {0xE8};
    for (int i = 0; i < 8; i++)
    {
        data2[i + 1] = (unsigned char)second_str[i];
    }

    // 调用发送和接收函数
    strcpy(result_code, TF_SendDeleteDocument(data1, data2));
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
    send_data_from_tx_fifo();
}




/* Send a read command to the TF module and copy the returned payload into the caller response buffer. */
char* TF_SendRead(unsigned char* data2, int read_len, unsigned char* response)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_RESET);  // 指定TF卡为接收模式
    HAL_Delay(20);

    int received_count = 0;
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时

    // 使用UART2与TF卡通信
    fifo_s_puts(&uart2_tx_fifo, data2, 7); 
    send_data_from_tx_fifo();

    // 非阻塞接收数据
    while (received_count < read_len)
    {
        if ((HAL_GetTick() - start_time) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }
        if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
        {
            fifo_s_get(&uart2_rx_fifo, &response[received_count]);  // 读取1字节
            received_count++;
        }
    }

    // 检查是否读取到数据
    int empty = 1;
    for (int i = 0; i < read_len; i++)
    {
        if (response[i] != 0)
        {
            empty = 0;
            break;
        }
    }

    if (empty)
    {
        strcpy(result_code, "读取为空\r\n");
    }
    else
    {
        strcpy(result_code, "读取成功\r\n");
    }

    return result_code;
}





/* Parse the host Read command and forward the TF read result back to USART1. */
void Read(char* command)
{
    // 检查命令格式
    char* colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    char* first_comma = strchr(command, ',');
    if (first_comma == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    *first_comma = '\0';

    char* StartAddress = colon_pos + 1;
    char* Read_Len = first_comma + 1;

    // 检查 StartAddress 和 Read_Len 是否为有效数字
    if (!isdigit(StartAddress[0]) || !isdigit(Read_Len[0]))
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    int StartAddress_value = atoi(StartAddress);
    int read_len_value = atoi(Read_Len);

    if (StartAddress_value > 7168 || read_len_value > 7168)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    // 构建读取命令
    unsigned char data2[7] = {0xE1};
    data2[1] = (unsigned char)((StartAddress_value >> 24) & 0xFF);
    data2[2] = (unsigned char)((StartAddress_value >> 16) & 0xFF);
    data2[3] = (unsigned char)((StartAddress_value >> 8) & 0xFF);
    data2[4] = (unsigned char)(StartAddress_value & 0xFF);
    data2[5] = (unsigned char)((read_len_value >> 8) & 0xFF);
    data2[6] = (unsigned char)(read_len_value & 0xFF);

    // 调用底层读取函数
    unsigned char response[7168] = {0};  // 最大7168字节
    strcpy(result_code, TF_SendRead(data2, read_len_value, response));

    // 判断读取结果
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code)); // 非阻塞发送
    send_data_from_tx_fifo();

    if (strcmp(result_code, "读取成功\r\n") == 0)
    {
        // 将读取到的数据发送给PC
        fifo_s_puts(&uart1_tx_fifo, response, read_len_value);  // 非阻塞发送
        send_data_from_tx_fifo();
    }
}




char* TF_SendWrite1(uint8_t* data, int32_t len)
{    
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_SET);  // 指定TF卡为写入模式
    HAL_Delay(20);

    unsigned char response[3] = {0};
    int received_count = 0;
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时

    // 使用UART2与TF卡通信
    fifo_s_puts(&uart2_tx_fifo, (uint8_t *)data, len);  // 非阻塞发送数据
    send_data_from_tx_fifo();

    // 非阻塞接收响应
    while (received_count < 3)
    {
        if ((HAL_GetTick() - start_time) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }

        if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
        {
            fifo_s_get(&uart2_rx_fifo, &response[received_count]);  // 逐字节读取响应
            received_count++;
        }
    }

    if (response[0] == 0xC1 && response[1] == 0x01 && response[2] == 0xC2)
    {
        strcpy(result_code, "写入成功\r\n");
    }
    else if (response[0] == 0xC1 && response[1] == 0x04 && response[2] == 0xC2)
    {
        strcpy(result_code, "写入失败\r\n");
    }
    else
    {
        strcpy(result_code, "InvalidCommand\r\n");
    }
    return result_code;
}





/* Write one block of raw data to the storage module; kept for early storage validation. */
void Write1(char* command)
{
    char* colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }
    char* first_str = colon_pos + 1;

    if (strlen(first_str) == 0)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    int32_t first_len = strlen(first_str);

    // 调用发送函数将数据发送到TF卡并获取返回值
    strcpy(result_code, TF_SendWrite1((uint8_t*)first_str, first_len));

    // 通过UART1返回给PC
    fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
    send_data_from_tx_fifo();
}




char* TF_SendWrite2(float* arr1, float* arr2, int len)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_SET);  // 指定TF卡为写入模式
    HAL_Delay(20);

    unsigned char response1[3] = {0};
    unsigned char response2[3] = {0};
    char buffer1[32] = {0};
    char buffer2[32] = {0};

    int i = 0;
    // 非阻塞发送和接收
    for (i = 0; i < len; i++)
    {
        sprintf(buffer1, "%.1f    ", arr1[i]);
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)buffer1, strlen(buffer1));  // 非阻塞发送
        send_data_from_tx_fifo();

        // 等待响应
        int received_count = 0;
        uint32_t start_time1 = HAL_GetTick();
        uint32_t timeout = 2000; // 2秒超时
        
        while (received_count < 3)
        {
            if ((HAL_GetTick() - start_time1) > timeout)
            {
                strcpy(result_code, "接收超时\r\n");
                return result_code;
            }
            if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
            {
                fifo_s_get(&uart2_rx_fifo, &response1[received_count]);  // 非阻塞读取
                received_count++;
            }
        }

        if (response1[0] == 0xC1 && response1[1] == 0x01 && response1[2] == 0xC2)
        {
            sprintf(buffer2, "%.1f\r\n", arr2[i]);
            fifo_s_puts(&uart2_tx_fifo, (uint8_t *)buffer2, strlen(buffer2));  // 非阻塞发送
            send_data_from_tx_fifo();

            HAL_Delay(20);

            // 接收第二个响应
            received_count = 0;
            uint32_t start_time2 = HAL_GetTick();

            while (received_count < 3)
            {
                if ((HAL_GetTick() - start_time2) > timeout)
                {
                    strcpy(result_code, "接收超时\r\n");
                    return result_code;
                }
                if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
                {
                    fifo_s_get(&uart2_rx_fifo, &response2[received_count]);  // 非阻塞读取
                    received_count++;
                }
            }

            if (response2[0] == 0xC1 && response2[1] == 0x04 && response2[2] == 0xC2)
            {
                strcpy(result_code, "写入失败\r\n");
                return result_code;
            }
        }
        else if (response1[0] == 0xC1 && response1[1] == 0x04 && response1[2] == 0xC2)
        {
            strcpy(result_code, "写入失败\r\n");
            return result_code;
        }
    }

    if (i == len)
    {
        strcpy(result_code, "写入成功\r\n");
    }
    else
    {
        strcpy(result_code, "写入失败\r\n");
    }
    return result_code;
}





/* Write two float arrays, typically used for calibration or curve data storage. */
void Write2(char* command)
{
    char* first_comma = strchr(command, ',');
    char* second_comma = first_comma ? strchr(first_comma + 1, ',') : NULL;
    if (first_comma == NULL || second_comma == NULL || strchr(second_comma + 1, ',') != NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    char* colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    *first_comma = '\0';
    *second_comma = '\0';
    char* arr1_name = colon_pos + 1;
    char* arr2_name = first_comma + 1;
    char* len_str = second_comma + 1;

    int len = atoi(len_str);
    if (len == 0 || len_str[0] == '\0' || !isdigit(len_str[0]))
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    float* arr1 = get_array_by_name(arr1_name);
    float* arr2 = get_array_by_name(arr2_name);

    if (arr1 == NULL || arr2 == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    strcpy(result_code, TF_SendWrite2(arr1, arr2, len));
    fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
    send_data_from_tx_fifo();
}





char* TF_SendWrite3(float* arr1, float* arr2, int location)
{
    HAL_GPIO_WritePin(GPIOB, MYTF_BY_Pin, GPIO_PIN_SET);  // 指定TF卡为写入模式
    HAL_Delay(20);

    unsigned char response1[3] = {0};
    unsigned char response2[3] = {0};
    char buffer1[32] = {0};
    char buffer2[32] = {0};

    sprintf(buffer1, "%.1f    ", arr1[location]);
    fifo_s_puts(&uart2_tx_fifo, (uint8_t *)buffer1, strlen(buffer1));  // 非阻塞发送
    send_data_from_tx_fifo();
    
    // 等待第一个响应
    int received_count = 0;
    uint32_t start_time1 = HAL_GetTick();
    uint32_t timeout = 2000; // 2秒超时

    while (received_count < 3)
    {
        if ((HAL_GetTick() - start_time1) > timeout)
        {
            strcpy(result_code, "接收超时\r\n");
            return result_code;
        }
        if (!fifo_s_is_empty(&uart2_rx_fifo))  // 检查是否有数据
        {
            fifo_s_get(&uart2_rx_fifo, &response1[received_count]);  // 非阻塞接收
            received_count++;
        }
    }

    if (response1[0] == 0xC1 && response1[1] == 0x01 && response1[2] == 0xC2)
    {
        sprintf(buffer2, "%.1f\r\n", arr2[location]);
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)buffer2, strlen(buffer2));  // 非阻塞发送
        send_data_from_tx_fifo();
        
        // 等待第二个响应
        received_count = 0;
        uint32_t start_time2 = HAL_GetTick();
        
        while (received_count < 3)
        {
            if ((HAL_GetTick() - start_time2) > timeout)
            {
                strcpy(result_code, "接收超时\r\n");
                return result_code;
            }

            if (!fifo_s_is_empty(&uart2_rx_fifo))
            {
                fifo_s_get(&uart2_rx_fifo, &response2[received_count]);  // 非阻塞接收
                received_count++;
            }
        }

        if (response2[0] == 0xC1 && response2[1] == 0x04 && response2[2] == 0xC2)
        {
            strcpy(result_code, "写入失败\r\n");
            return result_code;
        }
        else if (response2[0] == 0xC1 && response2[1] == 0x01 && response2[2] == 0xC2)
        {
            strcpy(result_code, "写入成功\r\n");
            return result_code;
        }
    }
    else if (response1[0] == 0xC1 && response1[1] == 0x04 && response1[2] == 0xC2)
    {
        strcpy(result_code, "写入失败\r\n");
        return result_code;
    }

    strcpy(result_code, "InvalidCommand\r\n");
    return result_code;
}







/* Update a selected position in stored curve data instead of rewriting the full array. */
void Write3(char* command)
{
    char* first_comma = strchr(command, ',');
    char* second_comma = first_comma ? strchr(first_comma + 1, ',') : NULL;
    if (first_comma == NULL || second_comma == NULL || strchr(second_comma + 1, ',') != NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    char* colon_pos = strchr(command, ':');
    if (colon_pos == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    *first_comma = '\0';
    *second_comma = '\0';
    char* arr1_name = colon_pos + 1;
    char* arr2_name = first_comma + 1;
    char* location_str = second_comma + 1;

    int location = atoi(location_str);
    if (location == 0 || location_str[0] == '\0' || !isdigit(location_str[0]))
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    float* arr1 = get_array_by_name(arr1_name);
    float* arr2 = get_array_by_name(arr2_name);

    if (arr1 == NULL || arr2 == NULL)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
        send_data_from_tx_fifo();
        return;
    }

    strcpy(result_code, TF_SendWrite3(arr1, arr2, location));
    fifo_s_puts(&uart2_tx_fifo, (uint8_t *)result_code, strlen(result_code));  // 非阻塞发送
    send_data_from_tx_fifo();
}





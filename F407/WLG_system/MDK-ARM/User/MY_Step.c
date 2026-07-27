#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_hal.h"              // HAL header for GPIO definitions
#include "MY_Step.h"



void ProcessD2Command(char* command)
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

	if (strcmp(prefix, "dir") == 0)
    {
        Set_Direction(command);  // 设置方向命令
    }
    else if (strcmp(prefix, "sub") == 0)
    {
        Set_Subdivision(command);  // 设置细分命令
    }
    else if (strcmp(prefix, "freq") == 0)
    {
        Set_Frequency(command);  // 设置频率命令
    }
    else if (strcmp(prefix, "stop") == 0)
    {
        Stop_StepperMotor();  // 停止命令
    }
    else if (strcmp(prefix, "start") == 0)
    {
        Start_StepperMotor();  // 启动命令
    }
    else
    {
        // 处理无效命令
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t *)result_code, strlen(result_code));
        send_data_from_tx_fifo();
    }
}



static void Subdivision(uint8_t i)
{
	if(i==8)
	{
		HAL_GPIO_WritePin(GPIOE,TIM1_MS2_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE,TIM1_MS1_Pin, GPIO_PIN_RESET);
	}
	else if(i==32)
	{
		HAL_GPIO_WritePin(GPIOE,TIM1_MS2_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE,TIM1_MS1_Pin, GPIO_PIN_SET);
	}
	else if(i==64)
	{
		HAL_GPIO_WritePin(GPIOE,TIM1_MS2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOE,TIM1_MS1_Pin, GPIO_PIN_RESET);
	}
	else if(i==16)
	{
		HAL_GPIO_WritePin(GPIOE,TIM1_MS2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOE,TIM1_MS1_Pin, GPIO_PIN_SET);
	}
}




/*
 * 功能: Set_Subdivision
 * 描述: 设置步进电机细分模式
 * 输入: 命令格式 "D2sub:X@"
 * 其中 X 为细分值 (8, 16, 32, 64)
 */
void Set_Subdivision(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (!colon_pos)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *sub_str = colon_pos + 1;

    // 检查是否为纯数字
    for (int i = 0; i < strlen(sub_str); i++)
    {
        if (!isdigit(sub_str[i]))
        {
            strcpy(result_code, "InvalidCommand.\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
            send_data_from_tx_fifo();
            return;
        }
    }

    // 转换为整数
    int sub_value = atoi(sub_str);

    // 检查是否为允许值
    if (sub_value != 8 && sub_value != 16 && sub_value != 32 && sub_value != 64)
    {
        strcpy(result_code, "Subdivision must be 8, 16, 32, or 64.\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo();
        return;
    }

    // 调用底层函数设置细分
    Subdivision((uint8_t)sub_value);

    // 反馈执行结果
    snprintf(result_code, sizeof(result_code), "Subdivision set to %d successfully.\r\n", sub_value);
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}




static void MoveDirection(uint8_t DIR_Flag)
{
	
	if(DIR_Flag)
	{
		// 顺时针
		HAL_GPIO_WritePin(GPIOE,TIM1_DIR_Pin, GPIO_PIN_SET);
	}
	else
	{
		// 逆时针
		HAL_GPIO_WritePin(GPIOE,TIM1_DIR_Pin, GPIO_PIN_RESET);
	}
	
}


/*
 * 功能: Set_Direction
 * 描述: 设置步进电机的旋转方向
 * 输入: 命令格式 "D2dir:X@"
 * 其中 X 为 0（逆时针）或 1（顺时针）
 */
void Set_Direction(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (!colon_pos)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *dir_str = colon_pos + 1;

    // 检查是否为纯数字
    for (size_t i = 0; i < strlen(dir_str); i++)
    {
        if (!isdigit((unsigned char)dir_str[i]))
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
            send_data_from_tx_fifo();
            return;
        }
    }

    int dir_value = atoi(dir_str);

    // 检查取值是否合法
    if (dir_value != 0 && dir_value != 1)
    {
        strcpy(result_code, "Direction must be 0 (CCW) or 1 (CW).\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo();
        return;
    }

    // 调用底层函数
    MoveDirection((uint8_t)dir_value);

    // 反馈执行结果
    if (dir_value == 1)
        strcpy(result_code, "Direction set to 正转\r\n");
    else
        strcpy(result_code, "Direction set to 反转\r\n");

    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}




static void Set_Timer_Frequency(int freq_value)
{
    // 设置定时器频率（ARR = 自动重装值，ARR 的值即为频率值）
    __HAL_TIM_SET_AUTORELOAD(&htim1, freq_value - 1);

    // 更新比较值（PWM 调整周期），设置为频率的一半
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, freq_value / 2);
}


/*
 * 功能: Set_Frequency
 * 描述: 设置步进电机的频率（即定时器的自动重装值）
 * 输入: 命令格式 "D2freq:X@"，
 * 其中 X 为频率值
 */
void Set_Frequency(char* command)
{
    char *colon_pos = strchr(command, ':');
    if (!colon_pos)
    {
        strcpy(result_code, "InvalidCommand\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo(); 
        return;
    }

    char *freq_str = colon_pos + 1;

    // 检查是否为纯数字
    for (size_t i = 0; i < strlen(freq_str); i++)
    {
        if (!isdigit((unsigned char)freq_str[i]))
        {
            strcpy(result_code, "InvalidCommand\r\n");
            fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
            send_data_from_tx_fifo();
            return;
        }
    }

    int freq_value = atoi(freq_str);

    // 检查频率是否合理（你可以根据实际需求设置范围）
    if (freq_value < 1 || freq_value > 10000)  // 假设频率范围是 10 至 10000
    {
        strcpy(result_code, "Frequency must be between 10 and 10000\r\n");
        fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
        send_data_from_tx_fifo();
        return;
    }

	Set_Timer_Frequency(freq_value);

    // 反馈执行结果
    snprintf(result_code, sizeof(result_code), "Frequency set to %d successfully\r\n", freq_value);
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}




/*
 * 功能: Start_StepperMotor
 * 描述: 启动步进电机
 * 输入: 命令格式 "D2start:@",
 */
void Start_StepperMotor(void)
{
	// 使能引脚拉低，启动步进电机
	HAL_GPIO_WritePin(GPIOE, TIM1_EN_Pin, GPIO_PIN_RESET);  
	// 启动PWM输出（开启通道 1）
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    strcpy(result_code, "Stepper motor started.\r\n");
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}



/*
 * 功能: Stop_StepperMotor
 * 描述: 停止步进电机
 * 输入: 命令格式 "D2stop:@",
 */
void Stop_StepperMotor(void)
{
	// 使能引脚拉高，停止步进电机
	HAL_GPIO_WritePin(GPIOE, TIM1_EN_Pin, GPIO_PIN_SET);  

    // 停止PWM输出（关闭通道 1）
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);

    // 反馈信息
    strcpy(result_code, "Stepper motor stopped.\r\n");
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}


void StepperMotor_Init(void)
{
    // 默认频率设置为 100
    int default_freq = 100;  
    // 默认细分模式设置为 8
    uint8_t default_subdivision = 8; 
    // 默认方向为正转（顺时针）
    uint8_t default_direction = 1;

    // 设置默认的细分模式
    Subdivision(default_subdivision);

    // 设置默认的运动方向
    MoveDirection(default_direction);

    // 设置默认频率
    Set_Timer_Frequency(default_freq);

    snprintf(result_code, sizeof(result_code), "Stepper motor initialized: Subdivision: %d, Direction: %s, Frequency: %d\r\n", 
            default_subdivision, (default_direction == 1) ? "正转" : "反转", default_freq);
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();
}



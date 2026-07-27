/*
 * File: uart_dma.c
 * Purpose: Central UART DMA receive and FIFO transmit support.
 * Hardware map: USART1 talks to Qt, USART2/USART3 talk to two temperature controllers, UART4 talks to the CO2 sensor.
 * Design note: DMA buffers only capture raw bytes; business modules should consume data from FIFO objects.
 */
#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_hal.h"              // HAL header for GPIO definitions
#include "stm32f4xx_hal_uart.h"
#include "uart_dma.h"
#include "fifo.h"


uint8_t uart1_dma_rx_buf[UART_DMA_BUF_SIZE];
uint8_t uart1_fifo_rx_buf[RX_FIFO_BUF_SIZE];
uint8_t uart1_fifo_tx_buf[TX_FIFO_BUF_SIZE];
// uint8_t USART1_Rx_buf[RX_BUF_SIZE];
uint8_t USART1_Tx_buf[TX_BUF_SIZE];

uint8_t uart2_dma_rx_buf[UART_DMA_BUF_SIZE];
uint8_t uart2_fifo_rx_buf[RX_FIFO_BUF_SIZE];
uint8_t uart2_fifo_tx_buf[TX_FIFO_BUF_SIZE];
// uint8_t USART2_Rx_buf[RX_BUF_SIZE];
uint8_t USART2_Tx_buf[TX_BUF_SIZE];

uint8_t uart3_dma_rx_buf[UART_DMA_BUF_SIZE];
uint8_t uart3_fifo_rx_buf[RX_FIFO_BUF_SIZE];
uint8_t uart3_fifo_tx_buf[TX_FIFO_BUF_SIZE];
// uint8_t USART3_Rx_buf[RX_BUF_SIZE];
uint8_t USART3_Tx_buf[TX_BUF_SIZE];

uint8_t uart4_dma_rx_buf[UART_DMA_BUF_SIZE];
uint8_t uart4_fifo_rx_buf[RX_FIFO_BUF_SIZE];
uint8_t uart4_fifo_tx_buf[TX_FIFO_BUF_SIZE];
// uint8_t USART4_Rx_buf[RX_BUF_SIZE];
uint8_t USART4_Tx_buf[TX_BUF_SIZE];

fifo_s uart1_rx_fifo;
fifo_s uart1_tx_fifo;
fifo_s uart2_rx_fifo;
fifo_s uart2_tx_fifo;
fifo_s uart3_rx_fifo;
fifo_s uart3_tx_fifo;
fifo_s uart4_rx_fifo;
fifo_s uart4_tx_fifo;

volatile uint8_t uart1_tx_complete_flag = 1; // 初始化为1，允许首次发送
volatile uint8_t uart2_tx_complete_flag = 1; // 初始化为1，允许首次发送
volatile uint8_t uart3_tx_complete_flag = 1; // 初始化为1，允许首次发送
volatile uint8_t uart4_tx_complete_flag = 1; // 初始化为1，允许首次发送

/* DMA receive callback. It copies only the newly received bytes from the circular DMA buffer into the matching FIFO. */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1) 
    {
        // 当前回调接收的数据在缓冲区的起点
        static uint16_t Rx1_buf_pos = 0;
        if (Size > Rx1_buf_pos) 
        {
            uint16_t length = Size - Rx1_buf_pos;
            fifo_s_puts(&uart1_rx_fifo, &uart1_dma_rx_buf[Rx1_buf_pos], length);
            Rx1_buf_pos = Size;
        } 
        else 
        {
            // 发生了绕圈
            uint16_t first_part = UART_DMA_BUF_SIZE - Rx1_buf_pos;
            uint16_t second_part = Size;

            // 先放入缓冲区末尾的部分
            fifo_s_puts(&uart1_rx_fifo, &uart1_dma_rx_buf[Rx1_buf_pos], first_part);

            // 再放入缓冲区开头的部分
            fifo_s_puts(&uart1_rx_fifo, uart1_dma_rx_buf, second_part);

            Rx1_buf_pos = Size; // 更新位置
        }
    }
    else if (huart->Instance == USART2) 
    {
        // 当前回调接收的数据在缓冲区的起点
        static uint16_t Rx2_buf_pos = 0;
        if (Size > Rx2_buf_pos) 
        {
            uint16_t length = Size - Rx2_buf_pos;
            fifo_s_puts(&uart2_rx_fifo, &uart2_dma_rx_buf[Rx2_buf_pos], length);
            Rx2_buf_pos = Size;
        } else 
        {
            // 发生了绕圈
            uint16_t first_part = UART_DMA_BUF_SIZE - Rx2_buf_pos;
            uint16_t second_part = Size;

            // 先放入缓冲区末尾的部分
            fifo_s_puts(&uart2_rx_fifo, &uart2_dma_rx_buf[Rx2_buf_pos], first_part);

            // 再放入缓冲区开头的部分
            fifo_s_puts(&uart2_rx_fifo, uart2_dma_rx_buf, second_part);

            Rx2_buf_pos = Size; // 更新位置
        }
    }
    else if (huart->Instance == USART3) 
    {
        // 当前回调接收的数据在缓冲区的起点
        static uint16_t Rx3_buf_pos = 0;
        if (Size > Rx3_buf_pos) 
        {
            uint16_t length = Size - Rx3_buf_pos;
            fifo_s_puts(&uart3_rx_fifo, &uart3_dma_rx_buf[Rx3_buf_pos], length);
            Rx3_buf_pos = Size;
        } else 
        {
            // 发生了绕圈
            uint16_t first_part = UART_DMA_BUF_SIZE - Rx3_buf_pos;
            uint16_t second_part = Size;

            // 先放入缓冲区末尾的部分
            fifo_s_puts(&uart3_rx_fifo, &uart3_dma_rx_buf[Rx3_buf_pos], first_part);

            // 再放入缓冲区开头的部分
            fifo_s_puts(&uart3_rx_fifo, uart3_dma_rx_buf, second_part);

            Rx3_buf_pos = Size; // 更新位置
        }
    }
    else if (huart->Instance == UART4) 
    {
        // 当前回调接收的数据在缓冲区的起点
        static uint16_t Rx4_buf_pos = 0;
        if (Size > Rx4_buf_pos) 
        {
            uint16_t length = Size - Rx4_buf_pos;
            fifo_s_puts(&uart4_rx_fifo, &uart4_dma_rx_buf[Rx4_buf_pos], length);
            Rx4_buf_pos = Size;
        } else 
        {
            // 发生了绕圈
            uint16_t first_part = UART_DMA_BUF_SIZE - Rx4_buf_pos;
            uint16_t second_part = Size;

            // 先放入缓冲区末尾的部分
            fifo_s_puts(&uart4_rx_fifo, &uart4_dma_rx_buf[Rx4_buf_pos], first_part);

            // 再放入缓冲区开头的部分
            fifo_s_puts(&uart4_rx_fifo, uart4_dma_rx_buf, second_part);

            Rx4_buf_pos = Size; // 更新位置
        }
    }
}




/* UART transmit-complete callback. The flag allows the next pending FIFO packet to be sent. */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) 
    {
        // 发送完成，设置标记，允许继续发送
        uart1_tx_complete_flag = 1;
    }
    else if (huart->Instance == USART2) 
    {
        // 发送完成，设置标记，允许继续发送
        uart2_tx_complete_flag = 1;
    }
    else if (huart->Instance == USART3) 
    {
        // 发送完成，设置标记，允许继续发送
        uart3_tx_complete_flag = 1;
    }
    else if (huart->Instance == UART4) 
    {
        // 发送完成，设置标记，允许继续发送
        uart4_tx_complete_flag = 1;
    }
}


/** 
 * @brief       串口错误回调函数
 * @param       huart：串口处理结构体指针
 * @note        本函数清除串口溢出标志，重新开启串口接收中断
 * @retval      无
 */
/* UART error callback. Restart DMA reception so a framing/noise error does not permanently stop communication. */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if(huart->ErrorCode & HAL_UART_ERROR_ORE)	//串口溢出错误
	{
		//串口溢出错误标志经过先读取SR再读取DR清除
		uint32_t temp = huart->Instance->SR;
		temp = huart->Instance->DR;
		//开启接收中断
        if(huart->Instance == USART1)
            HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart1_dma_rx_buf, UART_DMA_BUF_SIZE);
        else if(huart->Instance == USART2)
            HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart2_dma_rx_buf, UART_DMA_BUF_SIZE);
        else if(huart->Instance == USART3)
            HAL_UARTEx_ReceiveToIdle_DMA(&huart3, uart3_dma_rx_buf, UART_DMA_BUF_SIZE);
        else if(huart->Instance == UART4)
            HAL_UARTEx_ReceiveToIdle_DMA(&huart4, uart4_dma_rx_buf, UART_DMA_BUF_SIZE);
	}
}


/* Send queued USART1 text back to the Qt upper computer. Most D0/D1/D2/D3 replies leave through this function. */
void send_data_from_tx_fifo(void) 
{
    if (uart1_tx_complete_flag) 
    {
        uint16_t tx_len = fifo_s_count(&uart1_tx_fifo);
        if (tx_len > 0) 
        {
            uart1_tx_complete_flag = 0; 
            // 从发送 FIFO 读取数据
            uint8_t tx_read_status = fifo_s_gets(&uart1_tx_fifo, USART1_Tx_buf, tx_len);
            if (tx_read_status == FIFO_OK) 
            {
                if (huart1.hdmatx->State == HAL_DMA_STATE_READY) 
                {
                    HAL_UART_Transmit_DMA(&huart1, USART1_Tx_buf, tx_len);
                }
            }
        }
    }
    
    if (uart2_tx_complete_flag) 
    {
        uint16_t tx_len = fifo_s_count(&uart2_tx_fifo);
        if (tx_len > 0) 
        {
            uart2_tx_complete_flag = 0; 
            // 从发送 FIFO 读取数据
            uint8_t tx_read_status = fifo_s_gets(&uart2_tx_fifo, USART2_Tx_buf, tx_len);
            if (tx_read_status == FIFO_OK) 
            {
                if (huart2.hdmatx->State == HAL_DMA_STATE_READY) 
                {
                    HAL_UART_Transmit_DMA(&huart2, USART2_Tx_buf, tx_len);
                }
            }
        }
    }
    
    if(uart3_tx_complete_flag) 
    {
        uint16_t tx_len = fifo_s_count(&uart3_tx_fifo);
        if (tx_len > 0) 
        {
            uart3_tx_complete_flag = 0; 
            // 从发送 FIFO 读取数据
            uint8_t tx_read_status = fifo_s_gets(&uart3_tx_fifo, USART3_Tx_buf, tx_len);
            if (tx_read_status == FIFO_OK) 
            {
                if (huart3.hdmatx->State == HAL_DMA_STATE_READY) 
                {
                    HAL_UART_Transmit_DMA(&huart3, USART3_Tx_buf, tx_len);
                }
            }
        }
    }

    if(uart4_tx_complete_flag) 
    {
        uint16_t tx_len = fifo_s_count(&uart4_tx_fifo);
        if (tx_len > 0) 
        {
            uart4_tx_complete_flag = 0; 
            // 从发送 FIFO 读取数据
            uint8_t tx_read_status = fifo_s_gets(&uart4_tx_fifo, USART4_Tx_buf, tx_len);
            if (tx_read_status == FIFO_OK) 
            {
                if (huart4.hdmatx->State == HAL_DMA_STATE_READY) 
                {
                    HAL_UART_Transmit_DMA(&huart4, USART4_Tx_buf, tx_len);
                }
            }
        }
    }
}




/*
 * 函数名: UART_System_Init
 * 功能: 初始化所有与 UART 相关的 DMA、FIFO、状态标志
 * 说明: 在 main() 函数中调用一次即可
 */
/* Initialize all UART FIFOs and start DMA reception before protocol modules begin parsing commands. */
void UART_System_Init(void)
{
    /* 初始化 FIFO 缓冲区 */
    fifo_s_init(&uart1_rx_fifo, uart1_fifo_rx_buf, RX_FIFO_BUF_SIZE);
    fifo_s_init(&uart1_tx_fifo, uart1_fifo_tx_buf, TX_FIFO_BUF_SIZE);

    fifo_s_init(&uart2_rx_fifo, uart2_fifo_rx_buf, RX_FIFO_BUF_SIZE);
    fifo_s_init(&uart2_tx_fifo, uart2_fifo_tx_buf, TX_FIFO_BUF_SIZE);

    fifo_s_init(&uart3_rx_fifo, uart3_fifo_rx_buf, RX_FIFO_BUF_SIZE);
    fifo_s_init(&uart3_tx_fifo, uart3_fifo_tx_buf, TX_FIFO_BUF_SIZE);

    fifo_s_init(&uart4_rx_fifo, uart4_fifo_rx_buf, RX_FIFO_BUF_SIZE);
    fifo_s_init(&uart4_tx_fifo, uart4_fifo_tx_buf, TX_FIFO_BUF_SIZE);

    /* 初始化 DMA 接收（空闲中断方式） */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart1_dma_rx_buf, UART_DMA_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);  // 禁止半传输中断sw
    
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart2_dma_rx_buf, UART_DMA_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);

    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, uart3_dma_rx_buf, UART_DMA_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);

    HAL_UARTEx_ReceiveToIdle_DMA(&huart4, uart4_dma_rx_buf, UART_DMA_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart4.hdmarx, DMA_IT_HT);

    /* 清空标志位 */
    uart1_tx_complete_flag = 1;
    uart2_tx_complete_flag = 1;
    uart3_tx_complete_flag = 1;
    uart4_tx_complete_flag = 1;

    snprintf(result_code, sizeof(result_code), "fifo initialized successfully\r\n");
    fifo_s_puts(&uart1_tx_fifo, (uint8_t*)result_code, strlen(result_code));
    send_data_from_tx_fifo();

}






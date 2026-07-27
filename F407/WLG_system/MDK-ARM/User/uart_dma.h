#ifndef __UART_DMA_H__
#define __UART_DMA_H__

#include "usart.h"
#include "fifo.h"
#include <stdint.h>
#include <string.h>


#define UART_DMA_BUF_SIZE 256
#define RX_BUF_SIZE 256
#define TX_BUF_SIZE 256
#define RX_FIFO_BUF_SIZE 128
#define TX_FIFO_BUF_SIZE 128

extern uint8_t uart1_dma_rx_buf[UART_DMA_BUF_SIZE];
extern uint8_t uart1_fifo_rx_buf[RX_FIFO_BUF_SIZE];
extern uint8_t uart1_fifo_tx_buf[TX_FIFO_BUF_SIZE];
extern uint8_t USART1_Rx_buf[RX_BUF_SIZE];
extern uint8_t USART1_Tx_buf[TX_BUF_SIZE];

extern uint8_t uart2_dma_rx_buf[UART_DMA_BUF_SIZE];
extern uint8_t uart2_fifo_rx_buf[RX_FIFO_BUF_SIZE];
extern uint8_t uart2_fifo_tx_buf[TX_FIFO_BUF_SIZE];
extern uint8_t USART2_Rx_buf[RX_BUF_SIZE];
extern uint8_t USART2_Tx_buf[TX_BUF_SIZE];

extern uint8_t uart3_dma_rx_buf[UART_DMA_BUF_SIZE];
extern uint8_t uart3_fifo_rx_buf[RX_FIFO_BUF_SIZE];
extern uint8_t uart3_fifo_tx_buf[TX_FIFO_BUF_SIZE];
extern uint8_t USART3_Rx_buf[RX_BUF_SIZE];
extern uint8_t USART3_Tx_buf[TX_BUF_SIZE];

extern uint8_t uart4_dma_rx_buf[UART_DMA_BUF_SIZE];
extern uint8_t uart4_fifo_rx_buf[RX_FIFO_BUF_SIZE];
extern uint8_t uart4_fifo_tx_buf[TX_FIFO_BUF_SIZE];
extern uint8_t USART4_Rx_buf[RX_BUF_SIZE];
extern uint8_t USART4_Tx_buf[TX_BUF_SIZE];

extern fifo_s uart1_rx_fifo;
extern fifo_s uart1_tx_fifo;
extern fifo_s uart2_rx_fifo;
extern fifo_s uart2_tx_fifo;
extern fifo_s uart3_rx_fifo;
extern fifo_s uart3_tx_fifo;
extern fifo_s uart4_rx_fifo;
extern fifo_s uart4_tx_fifo;

extern volatile uint8_t uart1_tx_complete_flag; // 发送完成标记
extern volatile uint8_t uart2_tx_complete_flag; // 发送完成标记
extern volatile uint8_t uart3_tx_complete_flag; // 发送完成标记
extern volatile uint8_t uart4_tx_complete_flag; // 发送完成标记

void send_data_from_tx_fifo(void);
void UART_System_Init(void);

#endif

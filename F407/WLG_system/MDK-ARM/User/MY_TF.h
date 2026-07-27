#ifndef __My_TF_H
#define __My_TF_H

#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "string.h"
#include "stdio.h"
#include  "ctype.h"
#include "stdlib.h"
#include <stdint.h>
#include "uart_dma.h"

#define CMD_BUFFER_SIZE  218       
#define MODULENAME_SIZE  32        
#define PARAMNAME_SIZE   32       
#define PARAMVALUE_SIZE  32        

extern char rx_buffer[CMD_BUFFER_SIZE];                 // UART接收缓冲区
extern char command_buffer[CMD_BUFFER_SIZE];             // 命令解析缓冲区
extern uint32_t ReceiveLen;                               // 当前接收到的字节数
extern uint8_t command_received;                          // 标记命令是否接收完成
extern char result_code[CMD_BUFFER_SIZE];                // 处理结果返回的字符串
extern char module_name[MODULENAME_SIZE];                 // 模块名称
extern char param_name[PARAMNAME_SIZE];                   // 参数名称
extern char param_value[PARAMVALUE_SIZE];                 // 参数值


float* get_array_by_name(char* array_name);  // 根据数组名称获取数组指针
int add_array_to_map(const char* name, float* array, size_t size);  // 将数组添加到映射中

void ProcessD1Command(char* command);  // 处理D1命令
char* TF_SendCreateFile(uint8_t* data);  // 发送创建文件命令
void CreateFile(char* command);  // 创建文件命令
char* TF_SendCreateDocument(uint8_t* data1, uint8_t* data2);  // 发送创建文档命令
void CreateDocument(char* command);  // 创建文档命令
char* TF_SendOpenFile(uint8_t* data);  // 发送打开文件命令
void OpenFile(char* command);  // 打开文件命令
char* TF_SendOpenDocument(uint8_t* data1, uint8_t* data2);  // 发送打开文档命令
void OpenDocument(char* command);  // 打开文档命令
char* TF_SendDeleteFile(unsigned char* data);  // 发送删除文件命令
void DeleteFile(char* command);  // 删除文件命令
char* TF_SendDeleteDocument(unsigned char* data1, unsigned char* data2);  // 发送删除文档命令
void DeleteDocument(char* command);  // 删除文档命令
char* TF_SendRead(unsigned char* data2, int read_len, unsigned char* response);  // 发送读取命令
void Read(char* command);  // 读取命令
char* TF_SendWrite1(uint8_t* data, int32_t len);  // 发送写入1命令
void Write1(char* command);  // 写入1命令
char* TF_SendWrite2(float* arr1, float* arr2, int len);  // 发送写入2命令
void Write2(char* command);  // 写入2命令
char* TF_SendWrite3(float* arr1, float* arr2, int location);  // 发送写入3命令
void Write3(char* command);  // 写入3命令

#endif



#ifndef __FIFO_H__
#define __FIFO_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define FIFO_OK      0
#define FIFO_FULL    1
#define FIFO_EMPTY   2

#pragma pack(push, 1)
// 环形缓冲区结构体
typedef struct {
    uint8_t *buf;      // 指向缓冲区内存的指针
    uint16_t size;     // 缓冲区总大小（元素个数）
    uint16_t head;     // 写入数据的位置索引
    uint16_t tail;     // 读取数据的位置索引
    uint16_t count;    // 当前缓冲区中有效数据个数
} fifo_s;
#pragma pack(pop)

void        fifo_s_init(fifo_s *fifo, uint8_t *buf, uint16_t size);
uint8_t     fifo_s_put(fifo_s *fifo, uint8_t data);
uint8_t     fifo_s_get(fifo_s *fifo, uint8_t *data);
uint8_t     fifo_s_is_empty(fifo_s *fifo);
uint8_t     fifo_s_is_full(fifo_s *fifo);
uint16_t    fifo_s_count(fifo_s *fifo);
uint8_t     fifo_s_puts(fifo_s *fifo, const uint8_t *data, uint16_t len);
uint8_t     fifo_s_gets(fifo_s *fifo, uint8_t *data, uint16_t len);
uint16_t    fifo_s_free(fifo_s *fifo);

#endif

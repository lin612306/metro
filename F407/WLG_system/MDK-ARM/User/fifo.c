#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_hal.h"              // HAL header for GPIO definitions
#include "fifo.h"

void fifo_s_init(fifo_s *fifo, uint8_t *buf, uint16_t size)
{
    fifo->buf = buf;
    fifo->size = size;
    fifo->head = 0;
    fifo->tail = 0;
    fifo->count = 0;
}

uint8_t fifo_s_put(fifo_s *fifo, uint8_t data)
{
    if (fifo->count == fifo->size) return FIFO_FULL;
    fifo->buf[fifo->head] = data;
    fifo->head = (fifo->head + 1) % fifo->size;
    fifo->count++;
    return FIFO_OK;
}

uint8_t fifo_s_get(fifo_s *fifo, uint8_t *data)
{
    if (fifo->count == 0) return FIFO_EMPTY;
    *data = fifo->buf[fifo->tail];
    fifo->tail = (fifo->tail + 1) % fifo->size;
    fifo->count--;
    return FIFO_OK;
}

uint8_t fifo_s_is_empty(fifo_s *fifo)
{
    return (fifo->count == 0);
}

uint8_t fifo_s_is_full(fifo_s *fifo)
{
    return (fifo->count == fifo->size);
}

uint16_t fifo_s_count(fifo_s *fifo)
{
    return fifo->count;
}

uint8_t fifo_s_puts(fifo_s *fifo, const uint8_t *data, uint16_t len)
{
    if (fifo->size - fifo->count < len) {
        return FIFO_FULL;
    }

    uint16_t space_to_end = fifo->size - fifo->head;
    if (space_to_end >= len) {
        memcpy(&fifo->buf[fifo->head], data, len);
        fifo->head = (fifo->head + len) % fifo->size;
    } else {
        memcpy(&fifo->buf[fifo->head], data, space_to_end);
        memcpy(fifo->buf, data + space_to_end, len - space_to_end);
        fifo->head = len - space_to_end;
    }
    fifo->count += len;
    return FIFO_OK;
}

uint8_t fifo_s_gets(fifo_s *fifo, uint8_t *data, uint16_t len)
{
    if (fifo->count < len) {
        return FIFO_EMPTY;
    }

    uint16_t space_to_end = fifo->size - fifo->tail;
    if (space_to_end >= len) {
        memcpy(data, &fifo->buf[fifo->tail], len);
        fifo->tail = (fifo->tail + len) % fifo->size;
    } else {
        memcpy(data, &fifo->buf[fifo->tail], space_to_end);
        memcpy(data + space_to_end, fifo->buf, len - space_to_end);
        fifo->tail = len - space_to_end;
    }
    fifo->count -= len;
    return FIFO_OK;
}

uint16_t fifo_s_free(fifo_s *fifo)
{
    // 用 FIFO 的总大小减去当前已使用的元素数量，得到空闲空间
    return fifo->size - fifo->count;
}

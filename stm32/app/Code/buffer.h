#ifndef __BUFFER_H
#define __BUFFER_H

#include "main.h"

// 环形缓冲区最基本结构：能够实现从后面跳到前面
// 这只是一个基本结构体，而不是环形缓冲区接收的全部，DMA传输的时候可不会管你什么从后面跳到前面，人家只会自增存储
typedef struct s circle_buffer_t;

typedef struct s{
    void* buf;      // 这样定义的话，会因为不知道步进而不能使用[]来检索
    uint16_t buf_size;
    uint16_t head;
    uint16_t tail;
    circle_buffer_t* son;
}circle_buffer_t;

typedef struct{
    uint16_t head;
    uint16_t tail;
}recv_packet_info_t;

extern circle_buffer_t recv_circle_buf;
extern circle_buffer_t packet_circle_buf;

// void circle_buffer_init(void);
void circle_buf_packet_read(circle_buffer_t* packet_circle_buf, uint8_t* buf, uint16_t size);
void circle_buf_clear(circle_buffer_t* packet_circle_buf);
uint8_t* circle_buf_adjust(circle_buffer_t* packet_circle_buf, uint16_t recv_len);

#endif

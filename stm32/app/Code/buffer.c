#include "buffer.h"
#include <stdio.h>

// 对于 packet_circle_buf ，我需要存储每个包对应的起始和终点
// 为什么要另外再定义数据包的环形缓冲区呢？我直接数据的环形缓冲区在每次idle中断做出对应处理，每次外部程序再直接读取不就好了吗？
// 超子当时定义这个的使用好像有：
// 1. 根据数据包缓冲区的读写位置判断是否有数据可以读 -> 完全可以由数据缓冲区的读写位置来判断
// 2. 如果不想两个数据包之间粘合在一起，那确实是很有必要

void circle_buf_packet_read(circle_buffer_t* packet_circle_buf, uint8_t* buf, uint16_t size){
    if(packet_circle_buf->tail == packet_circle_buf->head){
        printf("No data to read\r\n");
        return ;    // 无数据可读
    }

    // 程序机制使得 _start 一定小于 _end 
    uint16_t _start = ((recv_packet_info_t*)packet_circle_buf->buf + packet_circle_buf->tail)->tail;  // 从包缓冲区中读取包起始位置
    uint16_t _end = ((recv_packet_info_t*)packet_circle_buf->buf + packet_circle_buf->tail)->head;
    printf("[circle_buf_packet_read] : _start is %d, _end is %d\r\n", _start, _end);

    if(buf != NULL){
        for(uint16_t i=0; i<_end-_start; i++){    // 由于空余空间不够大时，数据缓冲区直接调过这空余空间，所以不存在tail>head的情况
            if(i >= size){
                printf("ERROR: i >= size\r\n");
                break;
            }
            buf[i] = *((uint8_t*)packet_circle_buf->son->buf + _start + i);
        }
        printf("[circle_buf_packet_read] : three datum ahead of buf are 0x%x 0x%x 0x%x\r\n", buf[0], buf[1], buf[2]);
    }

    // 挪动
    packet_circle_buf->tail++;
    if(packet_circle_buf->tail >= packet_circle_buf->buf_size){
        packet_circle_buf->tail = 0;
    }

    printf("\r\n");
}

void circle_buf_all_read(circle_buffer_t* packet_circle_buf, uint8_t* buf, uint16_t size){

}

// TODO:有些情况是对方一下子响应几个消息，中间存在间隔，这就相当于多次空闲接收了
// 清理掉环形缓冲区
void circle_buf_clear(circle_buffer_t* packet_circle_buf){
    while(packet_circle_buf->tail != packet_circle_buf->head){
        circle_buf_packet_read(packet_circle_buf, NULL, 0);
    }
}

// 在接收结束后调用，返回下次接收的起始地址
uint8_t* circle_buf_adjust(circle_buffer_t* packet_circle_buf, uint16_t recv_len){
    if(packet_circle_buf->son == NULL){
        printf("[circle_buf_adjust] : ERROR\r\n");
        return NULL;
    }

    // 记录
    uint16_t old_recv_head = 0;    
    static uint8_t start_flag = 0;
    if(0 == start_flag){    // 首次进入
        printf("[circle_buf_adjust] : if(0 == start_flag) passed\r\n");
        if(recv_len > 0){
            start_flag = 1;
        }
    }
    else if(0 == packet_circle_buf->head){    // 当前的packet_circle_buf.head指向的成员还没写入信息
        printf("[circle_buf_adjust] : if(0 == packet_circle_buf.head) passed\r\n");
        old_recv_head = ((recv_packet_info_t*)packet_circle_buf->buf + packet_circle_buf->buf_size - 1)->head;     //转移到最后一个成员找对应数据包的头
    }
    else{
        old_recv_head = ((recv_packet_info_t*)packet_circle_buf->buf + packet_circle_buf->head - 1)->head;    //! 注意-1才是上一个数据包
    }

    if(packet_circle_buf->son->buf_size - old_recv_head < 133+1){   // 如果是133，新头可能会爆出去
        old_recv_head = 0;   // 从0开始
    }

    printf("[circle_buf_adjust] : old_recv_head = %d\r\n", old_recv_head);

    // 设置本次数据包头尾信息
    uint16_t this_recv_head = old_recv_head + recv_len;  //新头
    uint16_t this_recv_tail = old_recv_head;     // 旧头作尾
    ((recv_packet_info_t*)packet_circle_buf->buf + packet_circle_buf->head)->head = this_recv_head;
    ((recv_packet_info_t*)packet_circle_buf->buf + packet_circle_buf->head)->tail = this_recv_tail;
    printf("[circle_buf_adjust] : this_recv_head = %d\r\n", this_recv_head);
    printf("[circle_buf_adjust] : this_recv_tail = %d\r\n", this_recv_tail);

    // 挪动
    packet_circle_buf->head++;
    if(packet_circle_buf->head >= packet_circle_buf->buf_size){
        printf("[circle_buf_adjust] : Packet head return to 0\r\n");
        packet_circle_buf->head = 0;
    }

    //! 注意新的发送地址也要归零，这里不归零导致0起始位置数据一直没有被修改
    if(packet_circle_buf->son->buf_size - this_recv_head < 133+1){   // 如果是133，新头可能会爆出去
        this_recv_head = 0;   // 从0开始
    }
    return (uint8_t*)packet_circle_buf->son->buf+this_recv_head;

}

void circle_buf_test(void){

}

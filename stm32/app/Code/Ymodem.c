#include "Ymodem.h"
#include "usart.h"
#include "flash.h"
#include "bootloader.h"
#include "buffer.h"
#include <stdio.h>

// 发送函数
void send_cmd(uint8_t cmd){
    HAL_UART_Transmit(&huart6, &cmd, 1, HAL_MAX_DELAY);
}

// CRC16校验（对数据做运算得到校验值）
// 形参 crc 是初始 CRC 值（种子），用于支持增量/串流计算或非零初始值
#define POLY    0x1021

uint16_t crc16_calc(uint8_t* addr, uint16_t num){
    int i;  
    uint16_t crc = 0;
    for (; num > 0; num--)					/* Step through bytes in memory */  
    {  
        crc = crc ^ (*addr++ << 8);			/* Fetch byte from memory, XOR into CRC top byte*/  
        for (i = 0; i < 8; i++)				/* Prepare to rotate 8 bits */  
        {
            if (crc & 0x8000)				/* b15 is set... */  
                crc = (crc << 1) ^ POLY;  	/* rotate and XOR with polynomic */  
            else                          	/* b15 is clear... */  
                crc <<= 1;					/* just rotate */  
        }									/* Loop for 8 bits */  
        crc &= 0xFFFF;						/* Ensure CRC remains 16-bit value */  
    }										/* Loop until num=0 */  
    return(crc);
}

// y modem执行函数
void ymodem_execute(void){
    static uint8_t process_flag = 0;    //0：未接受 1：开始接收 2：结束接收
    static uint32_t data_sequence = 0x00;      // 序列
    uint16_t crc = 0;

    if(process_flag == 0){
        send_cmd(CRC16);
        HAL_Delay(3000);
        send_cmd(CRC16);
    }

    if(rx_flag == 1){
		rx_flag = 0;
		printf("Buffer length is %d\r\n", rx_len);
        #if USE_CIRCLE_BUFFER
        circle_buf_packet_read(&uart6_packet_circle, recv_buf, 135);
        #endif
        switch(recv_buf[0]){
            case SOH:
                printf("Receive SOH \r\n");
                crc = crc16_calc(recv_buf+3, 128);
                if(crc != (recv_buf[131]<<8|recv_buf[132])){     // 不通过校验
					printf("CRC16 Fail \r\n");
                    send_cmd(NAK);
                    return ;
                }
                // 开始包
                if(process_flag==0 && recv_buf[1]==0x00 && recv_buf[2]==(uint8_t)(~recv_buf[1])){   // 在 C 里 uint8_t 会先做整型提升到 int，然后 ~ 运算符作用在 int 上。当 recv_buf[1] == 0x00 时，~recv_buf[1] 的结果是 -1（0xFFFFFFFF），而 recv_buf[2] 被提升为正数 255（0x000000FF），所以比较失败。
					printf("Start-------------- \r\n");
					process_flag = 1;
                    data_sequence = 0x00;
                    flash_erase(APP2_ADDR, 1);
                    send_cmd(ACK);
                }
                // 结束包
                else if(process_flag==2 && recv_buf[1]==0x00 && recv_buf[2]==(uint8_t)(~recv_buf[1])){
					printf("End----------------- \r\n");
                    send_cmd(ACK);
                    process_flag = 0;
                    uint32_t temp = 0x1122AABB;
                    flash_write(APP2_ADDR + APP_SIZE - 4, &temp, 1);
                    HAL_NVIC_SystemReset();
                }
                // 数据包
                else if(process_flag==1 && recv_buf[2]==(uint8_t)(~recv_buf[1])){
                    if(data_sequence >= recv_buf[1]){
                        // 说明之前返回的ACK被忽略
                        printf("Data Ignore, Return ACK Again \r\n");
                        send_cmd(ACK);
                        return ;
                    }
                    data_sequence++;
					printf("> Receive data bag:%d byte\r\n",data_sequence * 128);
                    flash_write(APP2_ADDR + (data_sequence-1)*128, (uint32_t*)(&recv_buf[3]), 32);      //? 这该怎么解耦呢？
					send_cmd(ACK);
                }
				else{
					printf("NAK \r\n");
					send_cmd(NAK);
				}
                break;

            case STX:       // 协议传输数据内容时不会同时使用SOH和STX
                printf("Receive STX \r\n");
                crc = crc16_calc(recv_buf+3, 1024);
                if(crc != (recv_buf[1027]<<8|recv_buf[1028])){
					printf("CRC16 Fail \r\n");
                    send_cmd(NAK);
                    return ;
                }
                if(recv_buf[2]==(uint8_t)(~recv_buf[1])){
                    if(data_sequence >= recv_buf[1]){
                        // 说明之前返回的ACK被忽略
                        printf("Data Ignore, Return ACK Again \r\n");
                        send_cmd(ACK);
                        return ;
                    }
                    data_sequence++;
                    flash_write(APP2_ADDR + (data_sequence-1)*1024, (uint32_t*)(&recv_buf[3]), 256);	// 即使在数值上整合成32位，但是写入到内存是一样的
					send_cmd(ACK);
                }
				else{
					send_cmd(NAK);
				}
                break;

            case EOT:
				if(process_flag == 1){
                    printf("Receive EOT1 \r\n");
                    process_flag = 2;
                    send_cmd(NAK);
                }
                else if(process_flag == 2){
                    printf("Receive EOT2 \r\n");
                    send_cmd(ACK);
                    send_cmd('C');
                }
                break;
        }
    }
}
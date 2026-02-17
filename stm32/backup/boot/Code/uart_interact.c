#include "uart_interact.h"
#include <stdio.h>
#include <string.h>
#include "bootloader.h"
#include "ota_protocol.h"

extern uint8_t uart1_recv_buf[];
extern volatile uint8_t uart1_rx_flag;

#define _uart_recv_buf		uart1_recv_buf
#define _uart_rx_flag		uart1_rx_flag

static void check_update(void);

static void uart_interact_main(void){
	printf("=================UART INTERACT=================\r\n");
	printf("[A] Check update\r\n");
	printf("[rrr] Exit\r\n");
	printf("===============================================\r\n");
	while(1){
		if(!_uart_rx_flag){
			continue;
		}
		_uart_rx_flag = 0;
		char* temp = (char*)_uart_recv_buf;
		printf("temp is %s\r\n", (char*)_uart_recv_buf);
		
		if(strcmp(temp, "A")==0){
			check_update();
		}
		else if(strcmp(temp, "rrr")==0){
			return ;
		}
	}
}

static void check_update(void){
	printf("Checking the update...Enter rrr to shutdown\r\n");
	while(1){
		if(_uart_rx_flag==1){
			_uart_rx_flag = 0;
			char* temp = (char*)_uart_recv_buf;
			printf("temp is %s\r\n", (char*)_uart_recv_buf);
			if(strcmp(temp, "rrr")==0){
				return ;
			}
		}
//		ota_protocol_process();
	}
}

void check_wakeup_singal(void){
	int cnt = 0;
	while(!_uart_rx_flag){
		HAL_Delay(10);
		cnt++;
		if(cnt > 1000){
			return ;
		}
	}
	_uart_rx_flag = 0;
	char* temp = (char*)_uart_recv_buf;
	printf("%s\r\n", temp);
	if(strcmp(temp, "uuu") == 0){
		uart_interact_main();
	}
}
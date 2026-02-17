#include "ota_protocol.h"
#include <stdio.h>
#include "bootloader.h"

//============== usart interface ========================
extern UART_HandleTypeDef huart2;
#define uart_tans_func(pdata, size)         HAL_UART_Transmit(&huart2, pdata, size, 1000)
//============== flash interface ========================
#include "flash.h"

#define get_download_addr()                 bootloader_get_backup_addr()
#define erase_flash_sector(addr)                flash_erase(addr, 1)
#define write_to_flash(addr, pdata, size)         flash_write_generic(addr, pdata, size)
//=======================================================

#define ACK     0x06
#define NACK    0x07

typedef struct{
    uint8_t head[2];    // 0x55 0xAA
    uint8_t len;
    uint8_t cmd;
    uint8_t payload[PAYLOAD_LEN];
    uint16_t crc;
}frame_t;

// cmd types
enum{
    CMD_START = 0x00,
    CMD_COMPLETE,
    CMD_WRITE,
    CMD_COPY,
};

frame_t frame_info;
uint8_t ota_protocol_buffer[PROTOCOL_LEN];
volatile uint8_t ota_protocol_recv_flag = 0;

/**
 * @brief CRC Verification
 */
uint16_t cal_crc16(uint8_t* data, uint8_t len){
    uint16_t crc = 0xFFFF;
    for(int i=0; i<len; i++){
        crc ^= data[i];
        for(int j=0; j<8; j++){
            if((crc&1) != 0)    crc = (crc>>1)^0xA001;
            else                crc >>= 1;
        }
    }
    return crc;
}

/**
 * @brief the procession of ota protocol
 */
void ota_protocol_process(void){
	if(ota_protocol_recv_flag==0)   return ;
    ota_protocol_recv_flag = 0;

    static uint32_t download_addr;
    if(ota_protocol_buffer[0]!=0x55 || ota_protocol_buffer[1]!=0xAA){
        return ;
    }

    frame_info.len = ota_protocol_buffer[2];

    // crc verification
    frame_info.crc = cal_crc16(&ota_protocol_buffer[2], frame_info.len+2);   // include the len and cmd
    int crc_index = 3+frame_info.len+1;
    if(frame_info.crc != (uint16_t)(ota_protocol_buffer[crc_index] | (ota_protocol_buffer[crc_index+1]<<8))){
        uint8_t nack = NACK;
        uart_tans_func(&nack, 1);
        return  ;
    }

    frame_info.cmd = ota_protocol_buffer[3];
    if(frame_info.cmd == CMD_START){
		printf("CMD_START\r\n");
        download_addr = get_download_addr();
        erase_flash_sector(download_addr);
        uint8_t ack = ACK;
        uart_tans_func(&ack, 1);
    }
    else if(frame_info.cmd == CMD_COMPLETE){
        uint8_t ack = ACK;
        uart_tans_func(&ack, 1);
        bootloader_set_updateflag(1);
		HAL_NVIC_SystemReset();
    }
    else if(frame_info.cmd == CMD_COPY){

    }
    else if(frame_info.cmd == CMD_WRITE){
        write_to_flash(download_addr, &ota_protocol_buffer[4], frame_info.len);
        download_addr += frame_info.len;
        uint8_t ack = ACK;
        uart_tans_func(&ack, 1);
    }
}

#if 0

// fsm state types
typedef enum{
    STATE_IDLE = 0,
    STATE_HEAD2,
    STATE_LEN,
    STATE_CMD,
    STATE_PAYLOAD,
    STATE_CRC1,
    STATE_CRC2,
}state_t;

 /**
  * @brief  FSM to manage protocol
  * @return the validity of protocol data(0/1)
  */
uint8_t ota_protocol_fsm(uint8_t byte){
    static state_t state = 0;
    switch(state){
        case STATE_IDLE:
            if(byte == 0x55){
                state = STATE_HEAD2;
            }
            else{
                return 0;
            }
            break;
        case STATE_HEAD2:
            if(byte == 0xAA){
                state = STATE_LEN;
            }
            else{
                state = STATE_IDLE;
                return 0;
            }
            break;
        case STATE_LEN:
            frame_info.len = byte;
            state = STATE_CMD;
            break;
        case STATE_CMD:
            frame_info.cmd = byte;
            if(frame_info.cmd==CMD_WRITE || frame_info.cmd==CMD_COPY)   state = STATE_PAYLOAD;
            else if(frame_info.cmd==CMD_COMPLETE)                       state = STATE_IDLE;
            break;
        case STATE_PAYLOAD:
            for(int i=0; i<frame_info.len; i++){
                frame_info.payload[i] = ota_protocol_buffer[i+4];
            }
            state = STATE_CRC1;
            break;
        case STATE_CRC1:
            // crc verification
            frame_info.crc = cal_crc16(frame_info.payload, frame_info.len);
            if((uint8_t)(frame_info.crc&0xFF) == byte){
                state = STATE_CRC2;
            }
            else{
                state = STATE_IDLE;
                return 0;
            }
            break;
        case STATE_CRC2:
            if((uint8_t)(frame_info.crc>>8) == byte){
                state = STATE_IDLE;
            }
            else{
                state = STATE_IDLE;
                return 0;
            }
            break;
    }
    return 1;
}

/**
 * @brief the procession of ota protocol
 */
void ota_protocol_process(void){
	if(ota_protocol_recv_flag==0)   return ;
    ota_protocol_recv_flag = 0;
    uint8_t res = 0;

    res = ota_protocol_fsm(ota_protocol_buffer[0]);
    if(res==0)      return ;
    res = ota_protocol_fsm(ota_protocol_buffer[1]);
    if(res==0)      return ;
    ota_protocol_fsm(ota_protocol_buffer[2]);
    ota_protocol_fsm(ota_protocol_buffer[3]);
    // set complete flag
    if(frame_info.cmd == CMD_COMPLETE){
        uart_tans_func((uint8_t*)"ACK", 4);
        bootloader_set_updateflag(1);
		HAL_NVIC_SystemReset();
    }
    else if(frame_info.cmd == CMD_COPY){

    }
    else if(frame_info.cmd == CMD_WRITE){
        ota_protocol_fsm(0x00);
        res = ota_protocol_fsm(ota_protocol_buffer[PROTOCOL_LEN-2]);
        if(res==0){
            uart_tans_func((uint8_t*)"NACK", 5);
            return ;
        }
        res = ota_protocol_fsm(ota_protocol_buffer[PROTOCOL_LEN-1]);
        if(res==0){
            uart_tans_func((uint8_t*)"NACK", 5);
            return ;
        }
        // write to flash
        write_to_flash(frame_info.payload, frame_info.len);

        uart_tans_func((uint8_t*)"ACK", 4);
    }
}

#endif


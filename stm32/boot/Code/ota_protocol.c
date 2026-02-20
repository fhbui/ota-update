#include "ota_protocol.h"
#include <stdio.h>
#include "bootloader.h"
#include "log.h"

//============== usart interface ========================
extern UART_HandleTypeDef huart6;
#define uart_tans_func(pdata, size)         HAL_UART_Transmit(&huart6, pdata, size, 1000)
//============== flash interface ========================
#include "flash.h"

//#define get_download_addr()                 bootloader_get_backup_addr()
//#define get_app_addr()						bootloader_get_active_addr()
//#define erase_flash_sector(addr)                flash_erase(addr, 1)
#define read_from_flash(addr, pdata, size)			flash_read_generic(addr, pdata, size)
//#define write_to_flash(addr, pdata, size)         flash_write_generic(addr, pdata, size)
//============== w25qxx interface =======================
#include "w25qxx.h"

#define erase_flash_sector(addr)                w25qxx_erase_block(addr, 1)
#define write_to_flash(addr, pdata, size)         w25qxx_buffer_write(addr, pdata, size)
// ======================================================
#define get_download_addr()			BACKUP_ADDR
#define get_app_addr()				APP_ADDR

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

static const char* TAG = "ota_protocol";

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

    static uint32_t download_addr, app_addr, total_size;
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
        LOG_ERROR(TAG, "CRC Verification error! calculated is %d, recevied is %d", frame_info.crc, (uint16_t)(ota_protocol_buffer[crc_index] | (ota_protocol_buffer[crc_index+1]<<8)));
        return  ;
    }

    frame_info.cmd = ota_protocol_buffer[3];
    if(frame_info.cmd == CMD_START){
		LOG_INFO(TAG, "CMD_START");
		total_size = 0;
		app_addr = get_app_addr();
        download_addr = get_download_addr()+4;	// front addr used to set size
        erase_flash_sector(download_addr);
        uint8_t ack = ACK;
        uart_tans_func(&ack, 1);
    }
    else if(frame_info.cmd == CMD_COMPLETE){
        LOG_INFO(TAG, "CMD_COMPLETE");
        uint8_t ack = ACK;
        uart_tans_func(&ack, 1);
		write_to_flash(get_download_addr(), (uint8_t*)&total_size, 4);
        bootloader_set_updateflag(1);
		HAL_NVIC_SystemReset();
    }
    else if(frame_info.cmd == CMD_COPY){
		read_from_flash(app_addr, frame_info.payload, frame_info.len);
		write_to_flash(download_addr, frame_info.payload, frame_info.len);
		LOG_DEBUG(TAG, "Copy to 0x%08x", download_addr);
        download_addr += frame_info.len;	// COPY_CMD should full of 128 btyes payload so that the len is 128
		app_addr += frame_info.len;		// old frame size is same as new unless near the end
		total_size += frame_info.len;
        uint8_t ack = ACK;
        uart_tans_func(&ack, 1);
    }
    else if(frame_info.cmd == CMD_WRITE){
        write_to_flash(download_addr, &ota_protocol_buffer[4], frame_info.len);
		LOG_DEBUG(TAG, "Write to 0x%08x", download_addr);
        download_addr += frame_info.len;	// COPY_CMD should full of 128 btyes payload so that the len is 128
		app_addr += frame_info.len;		// old frame size is same as new unless near the end
		total_size += frame_info.len;
        uint8_t ack = ACK;
        uart_tans_func(&ack, 1);
    }
}


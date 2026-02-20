#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "main.h"

// 定义分区情况
#define SHARED_ADDR		0x08008000
#define APP1_ADDR   	0x08020000
#define APP2_ADDR   	0x08040000
#define APP_SIZE    	0xA000

#pragma pack(push, 1)  // 将当前的内存对齐规则保存（压栈） 起来，强制4字节对齐
typedef struct _shared_info{
	uint32_t active_app_addr;
	uint8_t update_flag;
	uint8_t error_flag;
	uint8_t status;
}shared_info_t;
#pragma pack(pop)

#define ACTIVE_APP_ADDR 	shared_info.active_app_addr
#define UPDATE_FLAG			shared_info.update_flag
#define ERROR_FLAG			shared_info.error_flag

#define flash_write_shared_info()		flash_erase(SHARED_ADDR, 1);			\
										flash_write_generic(SHARED_ADDR, &shared_info, 1*sizeof(shared_info_t))
#define flash_read_shared_info()		flash_read_generic(SHARED_ADDR, &shared_info, 1*sizeof(shared_info_t))

extern shared_info_t shared_info;

int bootloader_checkupdate(void);
void bootloader_executeapp(void);
void bootloader_updateapp(void);

#endif

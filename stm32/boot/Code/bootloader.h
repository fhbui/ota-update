#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "main.h"

// 定义分区情况
#define SHARED_ADDR		0x08008000
#define APP1_ADDR   	0x08020000
#define APP2_ADDR   	0x08040000
#define APP_SIZE    	0xA000

// 固件包头文件
#pragma pack(push, 1)  // 强制1字节对齐，无填充
typedef struct {
    uint32_t magic;          // 偏移0-3
    uint32_t header_crc;     // 偏移4-7
    uint8_t  version_major;  // 偏移8
    uint8_t  version_minor;  // 偏移9
    uint16_t version_build;  // 偏移10-11
    uint16_t hw_id;          // 偏移12-13
    uint16_t hw_variant;     // 偏移14-15
    uint32_t image_size;     // 偏移16-19
    uint32_t image_crc;      // 偏移20-23
    uint32_t signature[2];   // 偏移24-31
    uint32_t reserved;       // 偏移32-35
} fw_header_t;        // 总大小：36字节
#pragma pack(pop)

#define fw_header_size		36	// 字节

int bootloader_checkupdate(void);
uint32_t bootloader_get_active_addr(void);
uint32_t bootloader_get_backup_addr(void);
void bootloader_set_updateflag(uint8_t val);
void bootloader_set_errorflag(uint8_t val);

void bootloader_executeapp(void);
void bootloader_updateapp(void);
void bootloader_init(void);

#endif

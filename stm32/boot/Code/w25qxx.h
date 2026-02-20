#ifndef __W25QXX_H
#define __W25QXX_H

#include "main.h"

#define W25QXX_PageSize              256
#define W25QXX_PerWritePageSize      256

#define W25QXX_WriteEnable		      0x06 
#define W25QXX_WriteDisable		      0x04 
#define W25QXX_ReadStatusReg		    0x05 
#define W25QXX_WriteStatusReg		  0x01 
#define W25QXX_ReadData			        0x03 
#define W25QXX_FastReadData		      0x0B 
#define W25QXX_FastReadDual		      0x3B 
#define W25QXX_PageProgram		      0x02 
#define W25QXX_BlockErase			      0xD8 
#define W25QXX_SectorErase		      0x20 
#define W25QXX_ChipErase			      0xC7 
#define W25QXX_PowerDown			      0xB9 
#define W25QXX_ReleasePowerDown	  0xAB 
#define W25QXX_DeviceID			        0xAB 
#define W25QXX_ManufactDeviceID   	0x90 
#define W25QXX_JedecDeviceID		    0x9F 

#define WIP_Flag                  0x01  /* Write In Progress (WIP) flag */
#define Dummy_Byte                0xFF

#define W25QXX_TIMEOUT         ((uint32_t)0x1000)

uint8_t w25qxx_erase_sector(uint32_t sector_addr, uint32_t num);
uint8_t w25qxx_erase_block(uint32_t block_addr, uint32_t num);
void w25qxx_erase_chip(void);
void w25qxx_buffer_read(uint32_t read_addr, uint8_t* pbuf, uint32_t numbyte);
void w25qxx_page_write(uint32_t write_addr, uint8_t* pbuf, uint32_t numbyte);
void w25qxx_buffer_write(uint32_t write_addr, uint8_t* pbuf, uint32_t numbyte);
void w25qxx_test(void);
#endif

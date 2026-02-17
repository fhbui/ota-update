#ifndef __FLASH_H
#define __FLASH_H

#include "main.h"

void flash_erase(uint32_t addr, uint32_t num);
void flash_read(uint32_t addr, uint32_t* buf, uint32_t size);
void flash_write(uint32_t addr, uint32_t* buf, uint32_t size);
void flash_read_generic(uint32_t addr, void* buf, uint32_t size);
void flash_write_generic(uint32_t addr, void* buf, uint32_t size);

void flash_test(void);
void flash_compare(uint32_t addr1, uint32_t addr2, uint16_t size);
void flash_compare_to_buf(uint32_t addr, uint32_t* buf, uint16_t size);
void flash_check_empty(uint32_t addr, uint16_t size);

#endif

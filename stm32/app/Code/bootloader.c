#include "bootloader.h"
#include "flash.h"
#include <stdio.h>

shared_info_t shared_info;

// 检查更新标志位（APP2区最后数据）
int bootloader_checkupdate(void){
  if(*(uint32_t*)(APP2_ADDR + APP_SIZE - 4) != 0x1122AABB ){
    return 0;   // 无需更新
  }
  else{
    return 1;   // 需要更新
  }
}

// 修改MSP寄存器
__asm void MSR_MSP(uint32_t addr){
  MSR MSP, r0
  BX  r14
}

typedef void (*pc_adjust)(void);

// 跳转APP1区
void bootloader_executeapp(void){
  // TODO : 此前可以先保存当前程序版本号到新的扇区

  pc_adjust pc;
  // 检查栈顶地址正确性
  if(( (*(uint32_t *) APP1_ADDR) & 0x2FFE0000 ) == 0x20000000){
    MSR_MSP(*(uint32_t *) APP1_ADDR);
    pc = (pc_adjust)(*(uint32_t*)(APP1_ADDR+4));
    pc();
  }
}

// 更新APP1区
void bootloader_updateapp(void){
  // TODO : 动态地按照程序的大小进行更新
  uint32_t buf_temp[256] = {0};

  flash_erase(APP1_ADDR, 3);
  flash_check_empty(APP1_ADDR, APP_SIZE);
  for(int i=0; i<APP_SIZE/256/4; i++){
    flash_read(APP2_ADDR+i*256*4, buf_temp, 256);
    flash_write(APP1_ADDR+i*256*4, buf_temp, 256);
  }
// flash_compare(APP1_ADDR, APP2_ADDR, APP_SIZE/4);
  flash_erase(APP2_ADDR, 1);
}



#include "bootloader.h"
#include "flash.h"
#include <stdio.h>
#include "log.h"

#pragma pack(push, 1)  // ����ǰ���ڴ������򱣴棨ѹջ�� ������ǿ��4�ֽڶ���
typedef struct _shared_info{
	uint32_t active_app_addr;
	uint8_t update_flag;
	uint8_t error_flag;
}shared_info_t;
#pragma pack(pop)

static shared_info_t shared_info;

#define ACTIVE_APP_ADDR 	shared_info.active_app_addr
#define UPDATE_FLAG			shared_info.update_flag
#define ERROR_FLAG			shared_info.error_flag

#define flash_write_shared_info()		flash_erase(SHARED_ADDR, 1);			\
										flash_write_generic(SHARED_ADDR, &shared_info, 1*sizeof(shared_info_t))
#define flash_read_shared_info()		flash_read_generic(SHARED_ADDR, &shared_info, 1*sizeof(shared_info_t))
	
static const char* TAG = "bootloader";
										
// �����±�־λ��APP2��������ݣ�
int bootloader_checkupdate(void){
	return shared_info.update_flag;
}

void bootloader_set_updateflag(uint8_t val){
	flash_read_shared_info();
	shared_info.update_flag = val;
	flash_write_shared_info();
}

void bootloader_set_errorflag(uint8_t val){
	flash_read_shared_info();
	shared_info.error_flag = val;
	flash_write_shared_info();	
}

uint32_t bootloader_get_backup_addr(void){
	if(shared_info.active_app_addr == APP1_ADDR){
		return APP2_ADDR;
	}
	else{
		return APP1_ADDR;
	}
}

// �޸�MSP�Ĵ���
__asm void MSR_MSP(uint32_t addr){
	MSR MSP, r0
	BX  r14
}

typedef void (*pc_adjust)(void);

// ��ת����ķ���
void bootloader_executeapp(void){
	pc_adjust pc;
	// ���ջ����ַ��ȷ��
	if(( (*(uint32_t *) ACTIVE_APP_ADDR) & 0x2FFE0000 ) == 0x20000000){
		MSR_MSP(*(uint32_t *) ACTIVE_APP_ADDR);
		pc = (pc_adjust)(*(uint32_t*)(ACTIVE_APP_ADDR+4));
		pc();
	}
}

// ����APP1��
void bootloader_updateapp(void){
	if(ACTIVE_APP_ADDR == APP1_ADDR){
		ACTIVE_APP_ADDR = APP2_ADDR;
	}
	else{
		ACTIVE_APP_ADDR = APP1_ADDR;
	}
	LOG_DEBUG(TAG, "new partition is 0x%08x", ACTIVE_APP_ADDR);
	
	// �洢��Ϣ��������
	shared_info.update_flag = 0;
	shared_info.error_flag = 1;		// ��APP������λ
	flash_write_shared_info();

}

void bootloader_init(void){
//	shared_info.active_app_addr = APP1_ADDR;
//	flash_write_shared_info();
	flash_read_shared_info();
	LOG_DEBUG(TAG, "active addr is 0x%08x", shared_info.active_app_addr);
	
	if(shared_info.active_app_addr != APP1_ADDR && shared_info.active_app_addr != APP2_ADDR){
		// ��Ե���������ַ
		LOG_ERROR(TAG, "Addr error! adjust parition");
		shared_info.active_app_addr = APP1_ADDR;
	}
	
	if(shared_info.error_flag){
		// ���˵��ɰ汾
		shared_info.error_flag = 0;
		shared_info.active_app_addr = bootloader_get_backup_addr();
		flash_write_shared_info();
		LOG_ERROR(TAG, "Return to old version!");
	}
}

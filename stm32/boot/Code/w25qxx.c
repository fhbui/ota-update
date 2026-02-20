#include "w25qxx.h"

// ===================================
#include "spi.h"

#define W25Q_CS_SET(sta)	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, sta)
#define SPI_TANS_RECV(p_txdata, p_rxdata, num)    HAL_SPI_TransmitReceive(&hspi2, p_txdata, p_rxdata, num, W25QXX_TIMEOUT)
#define GET_SPI_BUSY_FALG()     __HAL_SPI_GET_FLAG(&hspi2, SPI_FLAG_BSY)
// ===================================

uint8_t w25qxx_sendbyte(uint8_t cmd){
	uint8_t rx_data;
	uint8_t tx_data = cmd;
	SPI_TANS_RECV(&tx_data, &rx_data, 1);
	return rx_data;
}

uint8_t w25qxx_recvbyte(void){
    return (w25qxx_sendbyte(Dummy_Byte));
}

uint8_t w25qxx_read_SR(void){
	uint8_t byte = 0;
	W25Q_CS_SET(0);
	w25qxx_sendbyte(W25QXX_ReadStatusReg);
	byte = w25qxx_recvbyte();
	W25Q_CS_SET(1);
	return byte;
}

void w25qxx_write_enable(void){
	W25Q_CS_SET(0);
	w25qxx_sendbyte(W25QXX_WriteEnable);
	
	uint32_t time_out = W25QXX_TIMEOUT;
	while(GET_SPI_BUSY_FALG() && time_out>0){
		time_out--;
	}
	W25Q_CS_SET(1);
}

void w25qxx_write_disable(void){
	W25Q_CS_SET(0);
	w25qxx_sendbyte(W25QXX_WriteDisable);
	
	uint32_t time_out = W25QXX_TIMEOUT;
	while(GET_SPI_BUSY_FALG() && time_out>0){
		time_out--;
	}
	W25Q_CS_SET(1);
}

void w25qxx_wait_busy(void){
	uint8_t ret_flag = 0;
	do {
        ret_flag = w25qxx_read_SR();
    } while((ret_flag & 0x01) == 0x01);    
}
// =======================================

uint8_t w25qxx_erase_sector(uint32_t sector_addr, uint32_t num){
	// The sctor addr will be tansferred by w25qxx to sector num(through ignore some least bits)
	w25qxx_wait_busy();
	w25qxx_write_enable();
	W25Q_CS_SET(0);
	
	uint32_t time_out = 0;
	for(int i=0; i<num; i++){
		w25qxx_sendbyte(W25QXX_SectorErase);
		w25qxx_sendbyte((sector_addr >> 16)&0xFF);
		w25qxx_sendbyte((sector_addr >> 8)&0xFF);
		w25qxx_sendbyte(sector_addr & 0xFF);
		sector_addr += 0x1000;
		
		time_out = W25QXX_TIMEOUT;
		while(GET_SPI_BUSY_FALG() && time_out>0){
			time_out--;
		}
		if(time_out==0){
			W25Q_CS_SET(1);
			w25qxx_wait_busy();
			w25qxx_write_disable();
			return 0;
		}
	}
	
	W25Q_CS_SET(1);
	w25qxx_wait_busy();
	w25qxx_write_disable();
	return 1;
}

uint8_t w25qxx_erase_block(uint32_t block_addr, uint32_t num){
	w25qxx_wait_busy();
	w25qxx_write_enable();
	W25Q_CS_SET(0);
	
	uint32_t time_out = 0;
	for(int i=0; i<num; i++){
		w25qxx_sendbyte(W25QXX_BlockErase);
		w25qxx_sendbyte((block_addr >> 16)&0xFF);
		w25qxx_sendbyte((block_addr >> 8)&0xFF);
		w25qxx_sendbyte(block_addr & 0xFF);
		block_addr += 0x10000;
		
		time_out = W25QXX_TIMEOUT;
		while(GET_SPI_BUSY_FALG() && time_out>0){
			time_out--;
		}
		if(time_out==0){
			W25Q_CS_SET(1);
			w25qxx_wait_busy();
			w25qxx_write_disable();
			return 0;
		}
	}
	
	W25Q_CS_SET(1);
	w25qxx_wait_busy();
	w25qxx_write_disable();
	return 1;
}

void w25qxx_erase_chip(void){
	w25qxx_wait_busy();
	w25qxx_write_enable();
	
	W25Q_CS_SET(0);
	w25qxx_sendbyte(W25QXX_ChipErase);
	W25Q_CS_SET(1);
	
	w25qxx_wait_busy();
	w25qxx_write_disable();
}

void w25qxx_buffer_read(uint32_t read_addr, uint8_t* pbuf, uint32_t numbyte){
	w25qxx_wait_busy();
	
	W25Q_CS_SET(0);
	w25qxx_sendbyte(W25QXX_ReadData);
	w25qxx_sendbyte((read_addr >> 16)&0xFF);
	w25qxx_sendbyte((read_addr >> 8)&0xFF);
	w25qxx_sendbyte(read_addr & 0xFF);
	
	for(int i=0; i<numbyte; i++){
		*pbuf = w25qxx_recvbyte();
		pbuf++;
	}
	W25Q_CS_SET(1);
}

void w25qxx_page_write(uint32_t write_addr, uint8_t* pbuf, uint32_t numbyte){
	w25qxx_wait_busy();
	w25qxx_write_enable();
	if(numbyte > W25QXX_PerWritePageSize)	
		numbyte = W25QXX_PerWritePageSize;

	W25Q_CS_SET(0);	
	w25qxx_sendbyte(W25QXX_PageProgram);
	w25qxx_sendbyte((write_addr >> 16)&0xFF);
	w25qxx_sendbyte((write_addr >> 8)&0xFF);
	w25qxx_sendbyte(write_addr & 0xFF);
	
	for(int i=0; i<numbyte; i++){
		w25qxx_sendbyte(*pbuf);
		pbuf++;
	}
	
	W25Q_CS_SET(1);	
	w25qxx_wait_busy();
	w25qxx_write_disable();
}

void w25qxx_buffer_write(uint32_t write_addr, uint8_t* pbuf, uint32_t numbyte){
    uint32_t bytes_remaining = numbyte;
    uint32_t current_addr = write_addr;
    uint8_t* current_buf = pbuf;

    uint32_t page_offset = current_addr % W25QXX_PageSize;
    if (page_offset != 0) {
        uint32_t bytes_in_first_page = W25QXX_PageSize - page_offset;
        if (bytes_in_first_page > bytes_remaining) {
            bytes_in_first_page = bytes_remaining;
        }
        
        w25qxx_page_write(current_addr, current_buf, bytes_in_first_page);
        
        current_buf += bytes_in_first_page;
        current_addr += bytes_in_first_page;
        bytes_remaining -= bytes_in_first_page;
    }

    while (bytes_remaining >= W25QXX_PageSize) {
        w25qxx_page_write(current_addr, current_buf, W25QXX_PageSize);
        
        current_buf += W25QXX_PageSize;
        current_addr += W25QXX_PageSize;
        bytes_remaining -= W25QXX_PageSize;
    }

    if (bytes_remaining > 0) {
        w25qxx_page_write(current_addr, current_buf, bytes_remaining);
    }
}

uint32_t w25qxx_read_id(void){
	W25Q_CS_SET(0);
	w25qxx_sendbyte(0x90);
	w25qxx_sendbyte(0x00);
	w25qxx_sendbyte(0x00);
	w25qxx_sendbyte(0x00);
	uint32_t temp1 = (uint32_t)w25qxx_recvbyte();
	uint32_t temp2 = (uint32_t)w25qxx_recvbyte();
	W25Q_CS_SET(1);
	
	return (temp1<<8)|(temp2);
}


#include <stdio.h>
void w25qxx_test(void){
	w25qxx_erase_sector(0x020000, 1);
	uint8_t data[2] = {1,2};
	w25qxx_buffer_write(0x020000, data, 2);
	uint8_t temp[2] = {0};
	w25qxx_buffer_read(0x020000, data, 2);
	
	printf("temp is %d %d\r\n", data[0], data[1]);
}

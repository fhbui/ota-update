#ifndef __OTA_PROTOCOL_H
#define __OTA_PROTOCOL_H

#include "main.h"

#define PAYLOAD_LEN         128
#define PROTOCOL_LEN		134
extern uint8_t ota_protocol_buffer[];
extern volatile uint8_t ota_protocol_recv_flag;

void ota_protocol_process(void);

#endif

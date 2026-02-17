#ifndef __YMODEM_H
#define __YMODEM_H

#include "main.h"

#define SOH     0x01    // modem 128字节头标志
#define STX     0x02    // modem 1024字节头标志
#define EOT     0x04    // 发送结束标志
#define ACK     0x06    // 应答标志
#define NAK     0x15    // 非应答标志
#define CAN     0x18    // 取消发送标志
#define CRC16   0x43    // 0x43正好对应'C'

void ymodem_execute(void);

#endif

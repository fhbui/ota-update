#ifndef __ONENET_H
#define __ONENET_H

#include "main.h"

// OneNet云连接相关信息
#define ONENET_HOST "mqtts.heclouds.com"    // 可在文档查看
#define ONENET_PORT 1883

#define PROID       "03jx21ALJH"// 产品ID
#define DEVICE_NAME "stm32"  // 设备名称
#define ACCESS_KEY  "TG0yblJxV2dZU293cEpXUXROODFBZWJ1NWVKNzRYTUc="  // 设备密钥，注意不是产品密钥（这个也是填写在TOKEN生成工具中的key）
#define TOKEN       "version=2018-10-31&res=products%2F03jx21ALJH%2Fdevices%2Fstm32&et=1956499200&method=sha1&sign=NIog2CtFXFKY%2Flw5Jrhvr1MF20g%3D"

// onenet相关函数
uint8_t onenet_connect(void);

void onenet_mqtt_test(void);
void onenet_http_test(void);

#endif

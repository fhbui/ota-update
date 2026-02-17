#include "onenet.h"
#include "wifi.h"
#include <stdio.h>
#include <string.h>
#include "flash.h"
#include "bootloader.h"

uint8_t onenet_connect(void){
    wifi_mqtt_usr_config(DEVICE_NAME, PROID, TOKEN);
    wifi_mqtt_connect(ONENET_HOST, ONENET_PORT, 1);
}

//! OTA的服务器（http）和MQTT的服务器不同
uint8_t onenet_ota_subscribe(void){
    char topic[128];
    snprintf(topic, sizeof(topic), "$sys/%s/%s/ota/inform", PROID, DEVICE_NAME);
    wifi_mqtt_subscribe(topic, 1);
}

uint8_t onenet_ota_check(void){
    // 上报设备当前版本

}

uint8_t onenet_ota_update(void){

}

void onenet_mqtt_test(void){
    uint8_t res = 0;
    res = onenet_connect();
    if(res == 1){
        char topic[128];
        snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);
        wifi_mqtt_subscribe(topic, 1);
        snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post/reply", PROID, DEVICE_NAME);
        wifi_mqtt_subscribe(topic, 1);

        // 发送消息
        snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", PROID, DEVICE_NAME);
        char data[256];
        snprintf(data, sizeof(data), "{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"test_point\\\":{\\\"value\\\":%d}}}", 1);   //! 和在printf中的字符串内包含""一样，发给ESP8266的字符串内含""时也应该通过\转义，而这个\在printf则需要\\转义
        wifi_mqtt_publish(topic, data, 0, 0);
    }
}

void onenet_http_test(void){
    char temp[512];
    uint8_t res = 0;
    int tid = 0, size = 0;
    char target[10];

    // 建立OTA服务器的连接
    snprintf(temp, sizeof(temp), "iot-api.heclouds.com");
    res = wifi_connect_tcp(temp, 80);
    if(res == 1){
        // 发送版本号
        snprintf(temp, sizeof(temp), "POST /fuse-ota/%s/%s/version HTTP/1.1\r\nContent-Type: application/json\r\nAuthorization:%s\r\nhost:iot-api.heclouds.com\r\nContent-Length:41\r\n\r\n{\"s_version\":\"V1.3\", \"f_version\": \"V2.0\"}\r\n\r\n",
										PROID, DEVICE_NAME, TOKEN);
        uint16_t len = strlen(temp);
        wifi_tcp_send(len, temp, "\"msg\":\"succ\"");

        // 查询OTA任务
        snprintf(temp, sizeof(temp), "GET /fuse-ota/%s/%s/check?type=2&version=V1.3 HTTP/1.1\r\nContent-Type: application/json\r\nAuthorization:%s\r\nhost:iot-api.heclouds.com\r\n\r\n",
										PROID, DEVICE_NAME, TOKEN);
        len = strlen(temp);
        res = wifi_tcp_send(len, temp, "\"msg\":\"succ\"");
        
        char* ptr = strstr((char*)wifi_recv_buf, "{\"code\":0");   //! 返回匹配的起始地址
        if(ptr != NULL){
            sscanf(ptr, "{\"code\":0,\"msg\":\"succ\",\"data\":{\"target\":\"%3s\",\"tid\":%d,\"size\":%d,\"md5\":\"%*32s\",\"status\":1,\"type\":1},\"request_id\":\"%*32s\"}", target, &tid, &size);
            //! %3s前面的内容需要和原文内容完全匹配，后续才会正确。
        }
        
        printf(" target is %s, tid is %d, size is %d\r\n", target, tid, size);
        if(res == 1 && tid != 0 && size != 0){
            uint16_t _start = 0, _end = 0;
            uint16_t count = (size%256==0)? size/256 : size/256+1;  // 需接收的次数
            uint32_t _addr = APP2_ADDR;

            // 分片下载程序包（一次接收的字节数可能会有600+）
            flash_erase(APP2_ADDR, 1);      // 清除扇区
            for(int i=0; i<count; i++){
                if(i == count-1){
                    _end = size-1;
                }
                else{
                    _end = _start+255;
                }
                
                snprintf(temp, sizeof(temp), "GET /fuse-ota/%s/%s/%d/download HTTP/1.1\r\nAuthorization:%s\r\nhost:iot-api.heclouds.com\r\nRange:bytes=%d-%d\r\n\r\n",
												PROID, DEVICE_NAME, tid, TOKEN, _start, _end);
                len = strlen(temp);
                res = wifi_tcp_send(len, temp, "206 Partial Content");
                printf("[Download] : %d ~ %d\r\n", _start, _end);

                if(res == 1){
                    // 写入flash
                    ptr = strstr((char*)wifi_recv_buf, "\r\n\r\n");
                    ptr += 4;

                    uint16_t byte_num = _end-_start+1;
                    if(byte_num%4 == 0){
                        flash_write(_addr, (uint32_t*)ptr, byte_num/4);
                        _addr += byte_num;     //! 一个地址存储一个字节
                    }
                    else{
                        // 最后一次
                        flash_write(_addr, (uint32_t*)ptr, byte_num/4+1);
                        _addr += 4*(byte_num/4+1);
                    }

                    _start += 256;
                }
                else{
                    i--;    // 与后续的i++相抵消
                }
            }

            // 写标志位、复位
            // uint32_t temp = 0x1122AABB;
            // flash_write(APP2_ADDR + APP_SIZE - 4, &temp, 1);
            
            flash_read_shared_info();
			shared_info.update_flag = 1;
			flash_write_shared_info();
			
            HAL_NVIC_SystemReset();
        }
    }
}

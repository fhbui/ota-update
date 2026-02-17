#ifndef __WIFI_H
#define __WIFI_H

#include "main.h"

#define SSID    "realme 11"// 路由器SSID名称（连接的热点）
#define PASS    "abcd4567"// 路由器密码（连接的热点）

// 外部变量
extern uint8_t wifi_recv_buf[];
extern uint8_t wifi_recv_use_circlebuf;

// wifi底层函数
#define WIFI_RECV_BUF_SIZE      768
uint8_t wifi_init(void);
uint8_t wifi_net_mode_set(uint8_t mode);
uint8_t wifi_set_hdcp(uint8_t operate, uint8_t mode);
uint8_t wifi_join_ap(char* ssid, char* password);

uint8_t wifi_connect_tcp(char* host, uint16_t port);
uint8_t wifi_tcp_send(uint16_t len, char* msg, char* target);

// http相关at指令函数
uint8_t wifi_http_test(void);

// mqtt相关at指令函数
uint8_t wifi_mqtt_usr_config(char* client_id, char* username, char* password);
uint8_t wifi_mqtt_connect(char* host, uint16_t port, uint8_t reconnect);
uint8_t wifi_mqtt_subscribe(char* topic, uint8_t qos);
uint8_t wifi_mqtt_publish(char* topic, char* data, uint8_t qos, uint8_t retain);
uint8_t wifi_mqtt_publish_raw(char* topic, uint16_t length, uint8_t qos, uint8_t retain);

#endif

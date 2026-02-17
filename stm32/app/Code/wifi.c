/**
 * @date 2025_10_30 : 暂时不使用环形缓冲区，以普通不定长方式接收WIFI模块响应
 */

#include "wifi.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "buffer.h"

#define WIFI_RECV_FLAG uart3_rx_flag

// TODO: 可能要根据情况看是否要设置状态，处理比如接收的是字符串，手动添加\0

uint8_t wifi_recv_use_circlebuf = 0;

uint8_t wifi_recv_buf[WIFI_RECV_BUF_SIZE];
uint8_t wifi_recv_times = 0;

static void wifi_send_AT(char* cmd){
    uint16_t size = strlen(cmd);
    HAL_StatusTypeDef res = HAL_UART_Transmit(&huart3, (uint8_t*)cmd, size, 500);
    if(res != HAL_OK){
        printf("[wifi_send_AT] : Send AT ERROR\r\n");
    }
}

// void wifi_get_res(char* res, uint16_t size){
//     if(WIFI_RECV_FLAG){
//         WIFI_RECV_FLAG = 0;
//         circle_buf_packet_read(&uart3_packet_circle, res, size);
//         printf("%s\r\n", res);
//     }
// }

static void wifi_print_recv(void){
	printf("[wifi_print_recv] : -------------\r\n");
	printf("%s\r\n", wifi_recv_buf);
}

static uint8_t wifi_wait_for_res(char* target, uint16_t timeout){
    // char temp[128];     //TODO:还是不要这么写
    while(timeout-- > 0){
        if(WIFI_RECV_FLAG){
            WIFI_RECV_FLAG = 0;
            if (strstr((char*)wifi_recv_buf, target) != NULL) {
                return 1;
            }
        }
        HAL_Delay(1);
    }
    return 0;
}

// wifi模块初始化
uint8_t wifi_init(void){
    // while(uart3_packet_circle.tail != uart3_packet_circle.head){
    //     uint8_t recv_temp[512];
    //     circle_buf_packet_read(&uart3_packet_circle, recv_temp, 512);
    //     printf("[wifi_init] : %s\r\n", recv_temp);
    // }
    wifi_print_recv();

    MX_USART3_UART_Init();

    wifi_send_AT("AT+RST\r\n");     // 一般必须要有回车换行
    HAL_Delay(1000);
    wifi_print_recv();

    // 关闭回显
//    wifi_send_AT("ATE0");
//    if(wifi_wait_for_res("OK", 1000)){
//        printf("[wifi_init] : Close reback success\r\n");
//    }
    if(wifi_net_mode_set(1) == 0){
        return 0;
    }
    if(wifi_set_hdcp(1, 1) == 0){
        return 0;
    }
    if(wifi_join_ap(SSID, PASS) == 0){
        return 0;
    }
    return 1;
}

// 0: 无 Wi-Fi 模式，并且关闭 Wi-Fi RF
// 1: Station 模式
// 2: SoftAP 模式
// 3: SoftAP+Station 模式
uint8_t wifi_net_mode_set(uint8_t mode){
    switch (mode)
    {
    case 0:
        wifi_send_AT("AT+CWMODE=0\r\n");
        break;
    case 1:
        wifi_send_AT("AT+CWMODE=1\r\n");
        break;
    case 2:
        wifi_send_AT("AT+CWMODE=2\r\n");
        break;
    case 3:
        wifi_send_AT("AT+CWMODE=3\r\n");
        break;
    
    default:
        break;
    }

    if(1 == wifi_wait_for_res("OK", 1000)){
		wifi_print_recv();
        printf("[wifi_net_mode_set] : Set success\r\n");
        return 1;       
    }
    printf("[wifi_net_mode_set] : Set failed\r\n");
    return 0;
}

/**
 * @param operate : 0: 禁用: 1: 启用
 * @param mode : 0: Station 的 DHCP； 1: SoftAP 的 DHCP
 */
uint8_t wifi_set_hdcp(uint8_t operate, uint8_t mode){
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CWDHCP=%d,%d\r\n", operate, mode);
    printf("[wifi_set_hdcp] : The cmd is %s\r\n", cmd);
    wifi_send_AT(cmd);

    if(1 == wifi_wait_for_res("OK", 1000)){
		wifi_print_recv();
        printf("[wifi_set_hdcp] : Set success\r\n");
        return 1;
    }
    printf("[wifi_set_hdcp] : Set failed\r\n");
    return 0;
}

uint8_t wifi_join_ap(char* ssid, char* password){
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    printf("[wifi_set_hdcp] : The cmd is %s\r\n", cmd);
    wifi_send_AT(cmd);

    if(1 == wifi_wait_for_res("OK", 15000)){
		wifi_print_recv();
        printf("[wifi_join_ap] : Set success\r\n");
        return 1;       
    }
    printf("[wifi_join_ap] : Set failed\r\n");
    return 0;
}

uint8_t wifi_connect_tcp(char* host, uint16_t port){
	char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", host, port);
    wifi_send_AT(cmd);

    if(1 == wifi_wait_for_res("OK", 3000)){
        wifi_print_recv();
        printf("[wifi_connect_tcp] : Connect success\r\n");      
        return 1;
    }
    wifi_print_recv();
    printf("[wifi_connect_tcp] : Connect failed\r\n");
    return 0;
}

uint8_t wifi_tcp_send(uint16_t len, char* msg, char* target){
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", len);
    wifi_send_AT(cmd);
    if(1 == wifi_wait_for_res("OK", 3000)){
        wifi_print_recv();
        printf("[wifi_tcp_send] : First success\r\n");    
        wifi_send_AT(msg);  
        if(1 == wifi_wait_for_res(target, 3000)){
            wifi_print_recv();
            printf("[wifi_tcp_send] : Second success\r\n");  
            return 1;
        }
    }    
    wifi_print_recv();
    printf("[wifi_tcp_send] : Failed\r\n");
	return 0;
}


/* HTTP相关AT指令 */

uint8_t wifi_http_client(){

}

uint8_t wifi_http_test(void){
    // wifi_send_AT("AT+HTTPCLIENT=1,0,\\\"http://httpbin.org/get\\\",\\\"httpbin.org\\\",\\\"/get\\\",1");
    // if(1 == wifi_wait_for_res("OK", 3000)){
	// 	wifi_print_recv();
    //     printf("[wifi_http_test] : Set success\r\n");
    //     return 1;       
    // }
    // printf("[wifi_http_test] : Set failed\r\n");
    // return 0;    
}

/* MQTT相关AT指令 */

/**
 * @param client_id : MQTT 客户端 ID（即设备名称），最大长度：256 字节。
 * @param username : 用户名（即产品名称），用于登陆 MQTT broker，最大长度：64 字节。
 * @param password : 密码（TOKEN），用于登陆 MQTT broker，最大长度：64 字节。
 */
uint8_t wifi_mqtt_usr_config(char* client_id, char* username, char* password){
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", client_id, username, password);

    wifi_send_AT(cmd);

    if(1 == wifi_wait_for_res("OK", 3000)){
		wifi_print_recv();
        printf("[wifi_mqtt_usr_config] : Set success\r\n");
        return 1;       
    }
    printf("[wifi_mqtt_usr_config] : Set failed\r\n");
    return 0;
}

/**
 * @param host : MQTT broker 域名，最大长度：128 字节。（mqtts.heclouds.com）
 * @param port : MQTT broker 端口，最大端口：65535。（1883）
 * @param  reconnect : 0: MQTT 不自动重连；1: MQTT 自动重连，会消耗较多的内存资源。
 */
uint8_t wifi_mqtt_connect(char* host, uint16_t port, uint8_t reconnect){
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%d,%d\r\n", host, port, reconnect);
    wifi_send_AT(cmd);

    if(1 == wifi_wait_for_res("OK", 3000)){
		wifi_print_recv();
        printf("[wifi_mqtt_connect] : Set success\r\n");
        return 1;
    }
    printf("[wifi_mqtt_connect] : Set failed\r\n");
    return 0;
}

/**
 * @param topic : 订阅的 topic。（$sys/%s/%s/thing/property/set PROID, DEVICE_NAME）
 * @param qos : 订阅的 QoS。（1）
 *              Qos0: 只发送一次，不保证正确性
 *              Qos1: Broker会向发送者发送确认消息，如果发送者没有收到确认，它会重试发送消息
 *              Qos2: 发送者和接收者之间会进行多次确认和重试（四次握手），以确保消息的准确传输
 */
uint8_t wifi_mqtt_subscribe(char* topic, uint8_t qos){
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+MQTTSUB=0,\"%s\",%d\r\n", topic, qos);   //! 居然忘记\r\n，又浪费时间
    wifi_send_AT(cmd);

    if(1 == wifi_wait_for_res("OK", 3000)){
        printf("[wifi_mqtt_subscribe] : Set success\r\n");
		wifi_print_recv();
        return 1;
    }
    printf("[wifi_mqtt_subscribe] : Set failed\r\n");
    wifi_print_recv();
    return 0;
}

/**
 * @param topic : MQTT topic，最大长度：128 字节。
 * @param data : MQTT 字符串消息。
 * @param qos : 发布消息的 QoS，参数可选 0、1、或 2，默认值：0。
 * @param retain : 发布 retain。retain即保留消息，broker接收到该位为1的消息后，会将这条消息保留以提供给后面新加入的订阅者，一个topic只能有一个retain，后者会覆盖前者
 */
uint8_t wifi_mqtt_publish(char* topic, char* data, uint8_t qos, uint8_t retain){
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",%d,%d\r\n", topic, data, qos, retain);
    wifi_send_AT(cmd);

    if(1 == wifi_wait_for_res("OK\r\n", 3000)){
        printf("[wifi_mqtt_publish] : Publish success\r\n");
		wifi_print_recv();
        return 1;
    }
    printf("[wifi_mqtt_publish] : Publish failed\r\n");
    wifi_print_recv();
    return 0;
}

/**
 * @param topic : MQTT topic，最大长度：128 字节。
 * @param length : MQTT 消息长度
 * @param qos : 发布消息的 QoS，参数可选 0、1、或 2，默认值：0。
 * @param retain : 发布 retain。
 * @note 后续需要发送参数消息，无需回车换行，超过消息长度即发送
 */
uint8_t wifi_mqtt_publish_raw(char* topic, uint16_t length, uint8_t qos, uint8_t retain){
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%d,%d,%d", topic, length, qos, retain);
    wifi_send_AT(cmd);

    if(1 == wifi_wait_for_res("OK\r\n>", 3000)){
		wifi_print_recv();
        return 1;
    }
    return 0;
}





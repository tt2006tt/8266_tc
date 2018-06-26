

#include "user_config.h"


LOCAL void ICACHE_FLASH_ATTR websocket_dns_check_cb(void *arg);
LOCAL void ICACHE_FLASH_ATTR websocket_dns_found(const char *name, ip_addr_t *ipaddr, void *arg);
void ICACHE_FLASH_ATTR http_dns_found(const char *name, ip_addr_t *ipaddr, void *arg);
void ICACHE_FLASH_ATTR CheckIpStart(void);
char* getJsonTree(void);

os_timer_t test_timer;
struct espconn websocket_tcp_conn;
struct espconn baidumap_tcp_conn;
struct espconn http_tcp_conn;
struct _esp_tcp websocket_tcp;
struct _esp_tcp baidumap_tcp;
struct _esp_tcp http_tcp;
ip_addr_t tcp_server_ip;
ip_addr_t baidumap_server_ip;
ip_addr_t http_server_ip;

extern volatile uint8_t WebSocketOK;
extern volatile uint8_t rxcount;
extern volatile uint8_t uart_poll;
//extern uint8_t DatePackege[64];

extern uint8_t len;

extern uint8_t uart_dst_addr;
extern uint8_t Cnt_Hour ;//小时记录
extern uint8_t Cnt_Day; //天记录
extern uint8_t Cnt_AlarmOFF;//记录不报警时间
extern uint8_t Cnt_second;
extern uint8_t Cnt_10second ;
extern uint8_t rxi ;
extern uint8_t cnt;
extern uint16_t test;
extern uint8_t len;
extern os_timer_t gas_main;//用主程序轮询






#define NET_DOMAIN "dev.flytronlink.net"

#define websocketphead  "GET /SmartHomeServer1/devws HTTP/1.1\r\nConnection:Upgrade\r\nHost:dev.flytronlink.net:80\r\nOrigin: http://dev.flytronlink.net\r\nSec-WebSocket-Key:Rmx6dHJvbmxpbmswMjcxNg==\r\nSec-WebSocket-Version:13\r\nUpgrade:websocket\r\n\r\n"
#define packet_size   (2 * 1024)
#define AK	"ak=mESCOGnQvpVDm0E7WxrIhb71U5HhRQD0&coor=bd09ll"
#define POST "POST /%s HTTP/1.1\r\nAccept: */*\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n%s"
#define GET "GET /%s HTTP/1.1\r\nAccept: */*\r\nHost: %s:80\r\nConnection: Keep-Alive\r\n\r\n?ak=mESCOGnQvpVDm0E7WxrIhb71U5HhRQD0"//ak:mESCOGnQvpVDm0E7WxrIhb71U5HhRQD0&coor=bd09ll\r\n

LOCAL void ICACHE_FLASH_ATTR map_data(char *point);

void ICACHE_FLASH_ATTR led_time_calcu_cb(void);
//#define Postdata "action=saveDevid&devid="  //"\"data\":{\"action\":\"saveDevid\",\"params\":\"{\"devid\":\"GA87654321\"}\"}"
//GA87654321



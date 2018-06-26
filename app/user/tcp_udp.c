#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"
#include "smartconfig.h"
#include "user_config.h"
#include "espconn.h"
#include "tcp_udp.h"
#include "ip_addr.h"
#include "espconn.h"
//#include "json/json.h"

#include "eagle_soc.h"
#include "networking.h"

extern uint8 Mode;	//系统状态
volatile uint8_t WebSocketOK = 0;
volatile uint8_t HttpOK = 0;
volatile uint8_t uart_poll = 0;//由于8266会莫名改变UartPackege[0]的数据 所以用此来判断
os_timer_t gas_main;//用主程序轮询
os_timer_t led_main;//用呼吸灯等轮询
volatile uint8_t SensorMode = 0;	//系统状态

Typdef_ProgramPara ProgramPara = {0}; //初始化结构体
Typdef_MapFormat  MapFormat = {0};
uint8_t Cnt_AlarmOFF = 0;//记录不报警时间
uint8_t Cnt_second = 0;

uint8_t Cnt_10second = 0;
uint8_t rxi = 0;
uint8_t cnt = 0;
uint16_t test = 0;
uint8_t len = 0;


extern uint8_t	gassmartconfig;

char ubWebRxBuf[RXMAX] = {0};//用来储存网络数据

volatile uint8_t rxcount = 0;//用于记录数据 因为websocket分包不确定
unsigned int x = 0;

uint8_t map_addlen = 0;


void  ICACHE_FLASH_ATTR led_time_calcu_cb(void)
{
    //pwm_set_duty(DUTYMIN,LED_GREEN);
    //pwm_start();

	if( ++ProgramPara.Led_Cnt_second >= 1800){//一分半钟
		SensorMode = Sensor_Normal;
		ProgramPara.Led_Cnt_second = 0;
		pwm_set_duty(DUTYMAX,LED_BLUE);

		pwm_set_duty(DUTYMAX,LED_RED);
		pwm_start();

		//os_printf("\r\n SensorMode~~~~\r\n%d",SensorMode);
	}
}

LOCAL void ICACHE_FLASH_ATTR
ChangeChar2Hex(uint8_t * Chars ,uint8_t * hexbyte,uint32_t charlen)
{

	//把2个字符转换成1个16进制
	//{'1', 'b'} == 0x1b
	uint32_t hexpos = 0;//十六进制第几个
	uint32_t charpos = 0;//字符第几个
	uint32_t i = 0;
	while(charpos<charlen)
	{

		if((Chars[charpos]<='9')&&(Chars[charpos]>='0'))
		{
			i = Chars[charpos] - '0';
		}
		else if((Chars[charpos]<='F')&&(Chars[charpos]>='A'))
		{
			i = Chars[charpos] - 'A'+10;

		}
		 else if((Chars[charpos]<='f')&&(Chars[charpos]>='a'))
		{
			i = Chars[charpos] - 'a'+10;

		}
		 else
		{
			i = 0x0;
		}
		i = (i<<4)&0xF0;
		charpos++;
		if((Chars[charpos]<='9')&&(Chars[charpos]>='0'))
		{
			i = i|((Chars[charpos] - '0')&0xF);
		}
		else if((Chars[charpos]<='F')&&(Chars[charpos]>='A'))
		{
			i = i|((Chars[charpos] - 'A'+10)&0xF);
		}
		else if((Chars[charpos]<='f')&&(Chars[charpos]>='a'))
		{
			i = i|((Chars[charpos] - 'a'+10)&0xF);
		}
		else
		{
			i = i|0x0;
		}

		hexbyte[hexpos] = i&0xFF;

		hexpos++;
		charpos++;
	}

}

/******************************************************************************
 * FunctionName : user_tcp_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
websocket_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
   //received some data from tcp connection
	struct espconn *pespconn = arg;
	uint8_t rxi=0;
	uint8_t x = 0;



	os_memcpy(&ubWebRxBuf[rxcount],pusrdata,length);
	rxcount += length;
	if(strstr(pusrdata,"HTTP/1.1 101"))
	{
		WebSocketOK = 1;
		ProgramPara.SubMode = SMARTCONFIG_FINISH;
		Mode = Mode_Nomal;//tcp连上 可以进行数据传输
		//os_printf("websocket connect ok !!! \r\n");

#if(SENSOR_TYPE == MP_503NEW)
		if(ProgramPara.Dev_ID[0] == 'H' && ProgramPara.Dev_ID[1] == 'T'){
			if(ProgramPara.Dev_ID[2] == 'F'){
				DataFormat.command = COMMAND_GETNEWID;//还未注册，先注册
				Receive = 0x01;
			}
#endif

			else{
				DataFormat.command = COMMAND_LOGIN;//已注册，进行登录
				Receive = 0x04;
			}
		}else{
			DataFormat.command = COMMAND_GETNEWID;//还未注册，先注册
			Receive = 0x01;
		}
		rxcount = 0;

	}
	//os_delay_us(300);
	if(WebSocketOK)
	{
		//os_printf("COMMAND_READGAS \r\n");
		if((ubWebRxBuf[0] == 0x82) && (rxcount > 5))
		{
			//os_printf("COMMAND_READGAS 2222\r\n");
			//os_printf("WebRecvData_Proc~~~~ok !!! %s,,, %d\r\n", ubWebRxBuf,rxcount);

			WebRecvData_Proc(ubWebRxBuf,0);

		}

	}

	//os_printf("WebRecvData_Proc cnt   ok !!! %d \r\n", cnt);
	//os_printf("tcp recv!!! %s  %d \r\n",pusrdata,length);


	//os_printf("tcp recv!!! %s  %d \r\n",pusrdata,length);


	//os_printf("\r\n%s\r\n",pusrdata);
}
char v[48] = {0};
LOCAL void ICACHE_FLASH_ATTR
map_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{

	uint8_t i ;
	//os_printf("tcp recv!!! %s   \r\n",pusrdata);
	map_data(pusrdata);
	//os_printf("\r\nyyyy%s\r\n",MapFormat.y);
	//os_printf("\r\nxxxx%s\r\n",MapFormat.x);
//	os_printf("\r\n%d\r\n",MapFormat.city_code);
	//os_printf("\r\naddr:%s\r\n",v);
//	for(i = 0;i < 48;i++)
	//{
	//	os_printf("%x  ",MapFormat.addr[i]);
//	}
	//os_printf("\r\n%s\r\n",pusrdata);
}

LOCAL void ICACHE_FLASH_ATTR map_data(char *point)
{
	char *p = strstr(point,":{\"address\":");
	p = p + os_strlen(":{\"address\":\"");
	uint32_t i = 0;
	while(p[map_addlen] != '"' && p[map_addlen+1] != ',')
	{

		if(p[map_addlen] == '\\' && p[map_addlen+1] == 'u')
		{
			p = p + 2;
		}

		v[map_addlen] = p[map_addlen];
		map_addlen++;
	}
	ChangeChar2Hex(v,MapFormat.addr,map_addlen);

	p = strstr(point,"\"city_code");
	p = p +os_strlen("\"city_code:\"");
	char x[10] = {0};
	while(p[i] != ',')
	{
		x[i] = p[i];
		i++;
	}

	i = 0;
	MapFormat.city_code = atoi(x);

	p = strstr(p,"{\"x");
	p = p + os_strlen("{\"x\":\"");
	while(p[i] != '"')
	{
		MapFormat.x[i] = p[i];
		i++;
	}
	i = 0;
	p = strstr(p,"\"y\":");
	p = p + os_strlen("\"y\":\"");
	while(p[i] != '"')
	{
		MapFormat.y[i] = p[i];
		i++;
	}
	i = 0;
}

LOCAL void ICACHE_FLASH_ATTR
http_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	if(strstr(pusrdata,"HTTP/1.1 200"))
	{
		HttpOK = 1;

	}
	if(strstr(pusrdata,"{\"result\":\"ok\"}")) //收到POST返回了正常的消息
	{
		Mode = Mode_Nomal;
		WebSocketOK = 1;
		gassmartconfig = 0;
		os_printf("http \"result\":\"ok\"} \r\n");
		rxcount = 0;
	}
	//os_printf("httttttppppp recv!!! %s  %d \r\n",pusrdata,length);
}

/******************************************************************************
 * FunctionName : websocket_tcp_sent_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/

LOCAL void ICACHE_FLASH_ATTR
websocket_tcp_sent_cb(void *arg)
{
   //data sent successfully
	//struct espconn *pespconn = arg;
	//Reflash(0x0000,0xFF,0);
    os_printf("tcp sent succeed !!! \r\n");
}
/******************************************************************************
 * FunctionName :http_sent_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/

LOCAL void ICACHE_FLASH_ATTR
http_sent_cb(void *arg)
{
   //data sent successfully
	//struct espconn *pespconn = arg;
	//Reflash(0x0000,0xFF,0);
    os_printf("http sent succeed !!! \r\n");
}
/******************************************************************************
 * FunctionName : websocket_tcp_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
websocket_tcp_discon_cb(void *arg)
{
   //tcp disconnect successfully
	ProgramPara.Led_Cnt_second = 0;
	Cnt_second = 0;
	Cnt_10second = 0;
	ClrBit(ProgramPara.init_flag_time,BIT_10S);
	ClrBit(ProgramPara.init_flag_time,BIT_30S);
	WebSocketOK = 0;
	HttpOK = 0;
	Mode = Mode_Bias;

    os_printf("tcp disconnect succeed !!! \r\n");
}

LOCAL void ICACHE_FLASH_ATTR
http_discon_cb(void *arg)
{
   //tcp disconnect successfully

   // os_printf("http_discon disconnect succeed !!! \r\n");
}

LOCAL void ICACHE_FLASH_ATTR
map_tcp_discon_cb(void *arg)
{
   //tcp disconnect successfully

    os_printf("http_discon disconnect succeed !!! \r\n");
}
/******************************************************************************
 * FunctionName : websocket_esp_platform_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
websocket_sent_data(struct espconn *pespconn)
{
	char pbuf[512] = {0};
	espconn_send(&websocket_tcp_conn, websocketphead, os_strlen(websocketphead));

	//os_sprintf(pbuf,POST,"location/ip",os_strlen(AK),"api.map.baidu.com",AK);//
	//espconn_send(&websocket_tcp_conn, pbuf, os_strlen(pbuf));

}

LOCAL void ICACHE_FLASH_ATTR
map_sent_data(struct espconn *pespconn)
{
	char pbuf[512] = {0};
	//espconn_send(&websocket_tcp_conn, websocketphead, os_strlen(websocketphead));
	//os_sprintf(pbuf,GET,"location/ip","api.map.baidu.com");
	os_sprintf(pbuf,POST,"location/ip",os_strlen(AK),"api.map.baidu.com",AK);//
	espconn_send(&baidumap_tcp_conn, pbuf, os_strlen(pbuf));

}


/******************************************************************************
 * FunctionName : websocket_tcp_connect_cb
 * Description  : A new incoming tcp connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
websocket_tcp_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;

    //os_printf("connect succeed !!! \r\n");

    espconn_regist_recvcb(pespconn, websocket_tcp_recv_cb);
   // espconn_regist_sentcb(pespconn, websocket_tcp_sent_cb);
    espconn_regist_disconcb(pespconn, websocket_tcp_discon_cb);

    websocket_sent_data(pespconn);

}

LOCAL void ICACHE_FLASH_ATTR
map_tcp_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;

   // os_printf("map_connect succeed !!! \r\n");

    espconn_regist_recvcb(pespconn, map_tcp_recv_cb);
   // espconn_regist_sentcb(pespconn, websocket_tcp_sent_cb);
    //espconn_regist_disconcb(pespconn, map_tcp_discon_cb);

    map_sent_data(pespconn);

}


/******************************************************************************
 * FunctionName : http_connect_cb
 * Description  : A new incoming http connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
http_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;
    uint8_t post[] = "POST /%s HTTP/1.1\r\nAccept: */*\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n%s";
    uint8_t postdata[33] = "action=saveDevid&devid=";
    // os_printf("http_connect succeed !!! \r\n");


    espconn_regist_recvcb(pespconn, http_tcp_recv_cb);
    espconn_regist_sentcb(pespconn, http_sent_cb);//下面两个回调不重要因此函数用和websocket同一个
    espconn_regist_disconcb(pespconn, http_discon_cb);

	if(gassmartconfig == 1
		&& ProgramPara.Dev_ID[2] != 'F')
	{
		char pbuf[194] = {0};
		postdata[23] = ProgramPara.Dev_ID[0];
		postdata[24] = ProgramPara.Dev_ID[1];
		postdata[25] = ProgramPara.Dev_ID[2];
		postdata[26] = ProgramPara.Dev_ID[3];
		postdata[27] = ProgramPara.Dev_ID[4];
		postdata[28] = ProgramPara.Dev_ID[5];
		postdata[29] = ProgramPara.Dev_ID[6];
		postdata[30] = ProgramPara.Dev_ID[7];
		postdata[31] = ProgramPara.Dev_ID[8];
		postdata[32] = ProgramPara.Dev_ID[9];
		os_sprintf(pbuf,post,"baas/tianhesm",sizeof(postdata),"flytronlink.net",postdata);//
		//os_printf("\r\npost 111111  %s !!!!!!!!! %d\r\n",pbuf,os_strlen(pbuf));
		espconn_send(&http_tcp_conn, pbuf, 194);
	}



}

/******************************************************************************
 * FunctionName : websocket_tcp_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
websocket_tcp_recon_cb(void *arg, sint8 err)
{
   //error occured , tcp connection broke. user can try to reconnect here.

    os_printf("websocket_reconnect callback, error code %d !!! \r\n",err);
}


LOCAL void ICACHE_FLASH_ATTR
map_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

    if (ipaddr == NULL) {
        os_printf("baidu_dns_found NULL \r\n");
        //Mode = Mode_Wait;
        return;
    }

    //dns got ip

    //os_printf("map_dns_found %d.%d.%d.%d \r\n",
    //        *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
    //        *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
    if (baidumap_server_ip.addr == 0 && ipaddr->addr != 0) {
        // dns succeed, create tcp connection
        //os_timer_disarm(&test_timer);
    	baidumap_server_ip.addr = ipaddr->addr;
        os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4); // remote ip of tcp server which get by dns
        pespconn->proto.tcp->remote_port = 80; // remote port of tcp server
        pespconn->proto.tcp->local_port = espconn_port(); //local port of ESP8266


        espconn_regist_connectcb(pespconn, map_tcp_connect_cb); // register connect callback
        //espconn_regist_reconcb(pespconn, websocket_tcp_recon_cb); // register reconnect callback as error handler
        espconn_connect(pespconn); // tcp connect



    }
}

void ICACHE_FLASH_ATTR
CheckIpStart(void)
{
	// Connect to tcp server as NET_DOMAIN


	websocket_tcp_conn.proto.tcp = &websocket_tcp;
	websocket_tcp_conn.type = ESPCONN_TCP;
	websocket_tcp_conn.state = ESPCONN_NONE;
	tcp_server_ip.addr = 0;

	http_tcp_conn.proto.tcp = &http_tcp;
	http_tcp_conn.type = ESPCONN_TCP;
	http_tcp_conn.state = ESPCONN_NONE;
	http_server_ip.addr = 0;

	baidumap_tcp_conn.proto.tcp = &baidumap_tcp;
	baidumap_tcp_conn.type = ESPCONN_TCP;
	baidumap_tcp_conn.state = ESPCONN_NONE;
	tcp_server_ip.addr = 0;

	espconn_gethostbyname(&websocket_tcp_conn, NET_DOMAIN, &tcp_server_ip, websocket_dns_found); // DNS function

	//espconn_gethostbyname(&baidumap_tcp_conn, "api.map.baidu.com", &baidumap_server_ip, map_dns_found); // DNS function

}

LOCAL void ICACHE_FLASH_ATTR
websocket_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    if (ipaddr == NULL) {
        os_printf("websocket_dns_found NULL \r\n");
        //Mode = Mode_Wait;
        return;
    }

    //dns got ip
  //  os_printf("user_dns_found %d.%d.%d.%d \r\n",
   //         *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
   //         *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
//    os_printf("\r\n\r\nMode === %x\r\n\r\n",Mode);

    if (tcp_server_ip.addr == 0 && ipaddr->addr != 0) {
        // dns succeed, create tcp connection
        //os_timer_disarm(&test_timer);
        tcp_server_ip.addr = ipaddr->addr;
        http_server_ip.addr = ipaddr->addr;
        os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4); // remote ip of tcp server which get by dns
        pespconn->proto.tcp->remote_port = 80; // remote port of tcp server
        pespconn->proto.tcp->local_port = espconn_port(); //local port of ESP8266

        os_memcpy(http_tcp_conn.proto.tcp->remote_ip, &ipaddr->addr, 4); // remote ip of tcp server which get by dns
        http_tcp_conn.proto.tcp->remote_port = 80; // remote port of tcp server
        http_tcp_conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266

        espconn_regist_connectcb(pespconn, websocket_tcp_connect_cb); // register connect callback
        espconn_regist_reconcb(pespconn, websocket_tcp_recon_cb); // register reconnect callback as error handler
        espconn_connect(pespconn); // tcp connect


        espconn_regist_connectcb(&http_tcp_conn, http_connect_cb); // register connect callback
        espconn_connect(&http_tcp_conn); // tcp connect




    }
}















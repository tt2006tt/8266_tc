#include "user_config.h"
#include "os_type.h"

#define DEVICE_TYPE 		"gh_9e2cff3dfa51" //wechat public number
#define DEVICE_ID 			"122475" //model ID

#define DEFAULT_LAN_PORT 	12476

esp_udp ssdp_udp;
struct espconn airkissdpudpconn;
os_timer_t ssdp_time_serv;
extern os_timer_t gas_main;//用串口轮询
extern volatile uint8_t SensorMode;
struct espconn PhoneConn;  //网络连接的结构体，里面定义了连接的类型和回调函数等
esp_udp PhoneConnUdp;      //用来声明连接是udp，存放了端口、ip等数据

uint8_t  lan_buf[200];
uint16_t lan_buf_len;
uint8_t 	 udp_sent_cnt = 0;
uint8_t	gassmartconfig = 0;
struct	ip_info info;

const airkiss_config_t akconf =
{
	(airkiss_memset_fn)&memset,
	(airkiss_memcpy_fn)&memcpy,
	(airkiss_memcmp_fn)&memcmp,
	0,
};


LOCAL void ICACHE_FLASH_ATTR
airkiss_wifilan_time_callback(void)
{
	uint16 i;
	airkiss_lan_ret_t ret;
	uint8_t ret1;
	Typdef_MyIPAddress user_mask;
	Typdef_MyIPAddress user_ip;

	if ((udp_sent_cnt++) >2) {
		udp_sent_cnt = 0;
		os_timer_disarm(&ssdp_time_serv);//s
		//return;
	}
	/*udp 内网广播地址*/
    wifi_get_ip_info(STATION_IF,&info);
    os_memcpy(&user_mask,&info.netmask,sizeof(user_mask));
    os_memcpy(&user_ip,&info.ip,sizeof(user_ip));

	user_ip.Arr[0] |= (~user_mask.Arr[0]);
	user_ip.Arr[1] |= (~user_mask.Arr[1]);
	user_ip.Arr[2] |= (~user_mask.Arr[2]);
	user_ip.Arr[3] |= (~user_mask.Arr[3]);
	ssdp_udp.remote_port = DEFAULT_LAN_PORT;
	ssdp_udp.local_port = espconn_port();
	lan_buf_len = sizeof(lan_buf);
	ret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD,
		DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf, &lan_buf_len, &akconf);
	if (ret != AIRKISS_LAN_PAKE_READY) {
		os_printf("Pack lan packet error!");
		return;
	}
	//ret = espconn_send(&airkissdpudpconn, lan_buf, lan_buf_len);
	ret1 = espconn_sendto(&airkissdpudpconn, lan_buf, lan_buf_len);
	if (ret1 != 0) {

		os_printf("UDP send error code %d!",ret1);

	}
	ProgramPara.flag_Time =  0;
	os_printf("Finish send notify!\n");
}


LOCAL void ICACHE_FLASH_ATTR
airkiss_wifilan_recv_callbk(void *arg, char *pdata, unsigned short len)
{
	uint16 i;
	remot_info* pcon_info = NULL;

	airkiss_lan_ret_t ret = airkiss_lan_recv(pdata, len, &akconf);
	airkiss_lan_ret_t packret;

	switch (ret){
	case AIRKISS_LAN_SSDP_REQ:
		espconn_get_connection_info(&airkissdpudpconn, &pcon_info, 0);
		os_printf("\r\n8266 gas remote ip: %d.%d.%d.%d \r\n",pcon_info->remote_ip[0],pcon_info->remote_ip[1],
			                                    pcon_info->remote_ip[2],pcon_info->remote_ip[3]);
		os_printf("\r\n8266 gasremote port: %d \r\n",pcon_info->remote_port);

		airkissdpudpconn.proto.udp->remote_port = pcon_info->remote_port;
		os_memcpy(airkissdpudpconn.proto.udp->remote_ip,pcon_info->remote_ip,4);
		ssdp_udp.remote_port = DEFAULT_LAN_PORT;

		lan_buf_len = sizeof(lan_buf);
		packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_RESP_CMD,
			DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf, &lan_buf_len, &akconf);

		if (packret != AIRKISS_LAN_PAKE_READY) {
			os_printf("Pack lan packet error!");
			return;
		}

		os_printf("\r\nAIRKISS_LAN_SSDP_REQ8266AIRKISS_LAN_SSDP_REQAIRKISS_LAN_SSDP_REQ\r\n");
		for (i=0; i<lan_buf_len; i++)
			os_printf("%c",lan_buf[i]);
		os_printf("\r\n\r\n");

		packret = espconn_sendto(&airkissdpudpconn, lan_buf, lan_buf_len);
		if (packret != 0) {
			os_printf("LAN UDP Send err!");
		}

		break;
	case AIRKISS_LAN_ERR_CMD:
		os_printf("\r\n AIRKISS_LAN_ERR_CMD \r\n");
	break;
	case AIRKISS_LAN_ERR_OVERFLOW:
		os_printf("\r\n AIRKISS_LAN_ERR_CMD \r\n");
	break;

	case AIRKISS_LAN_ERR_PAKE:
		os_printf("\r\n AIRKISS_LAN_ERR_CMD \r\n");
	break;
	case AIRKISS_LAN_ERR_PARA:
		os_printf("\r\n AIRKISS_LAN_ERR_CMD \r\n");
	break;
	case AIRKISS_LAN_ERR_PKG:
		os_printf("\r\n AIRKISS_LAN_ERR_CMD \r\n");
	break;

	case AIRKISS_LAN_CONTINUE:
		os_printf("\r\n AIRKISS_LAN_ERR_CMD \r\n");
	break;
	case AIRKISS_LAN_PAKE_READY:
		os_printf("\r\n AIRKISS_LAN_ERR_CMD \r\n");
	break;

	default:
		os_printf("Pack is not ssdq req!%d\r\n",ret);
		break;
	}
}

void ICACHE_FLASH_ATTR
airkiss_start_discover(void)
{

	ssdp_udp.local_port = DEFAULT_LAN_PORT;
	airkissdpudpconn.type = ESPCONN_UDP;
	airkissdpudpconn.proto.udp = &ssdp_udp;
	espconn_regist_recvcb(&airkissdpudpconn, airkiss_wifilan_recv_callbk);
	espconn_create(&airkissdpudpconn);

	os_timer_disarm(&ssdp_time_serv);
	os_timer_setfn(&ssdp_time_serv, (os_timer_func_t *)airkiss_wifilan_time_callback, NULL);
	os_timer_arm(&ssdp_time_serv, 5000, 1);//1s
}


static uint8_t SmartACK[18]        ="smartok,";

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{

    switch(status) {
        case SC_STATUS_WAIT:

            os_printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            os_printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
        	ClrBit(ProgramPara.init_flag_time,BIT_30S);
        	ProgramPara.SubMode = SMARTCONFIG_WAIT;
            os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
			sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:

        	gassmartconfig = 1;
    		Mode = Mode_CreateSession;//连上wifi后立刻 连接websocket 进行ID的上传  如果放在link over会出现问题 覆盖掉mode 模式
    		os_delay_us(100);
			/*********************************************
			 * 重新打开主程序
		    ********************************************/
			os_timer_setfn(&gas_main, (os_timer_func_t *)gas_main_cb,0);
			os_timer_arm(&gas_main, 100, 1);//1s扫描一次
            os_printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;

	        wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            os_printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
				//SC_TYPE_ESPTOUCH
                uint8 phone_ip[4] = {0};
                os_memcpy(phone_ip, (uint8*)pdata, 4);
                //os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);SmartACK[i+8]

                PhoneConn.proto.udp->remote_ip[0] = phone_ip[0];
                PhoneConn.proto.udp->remote_ip[1] = phone_ip[1];
                PhoneConn.proto.udp->remote_ip[2] = phone_ip[2];
                PhoneConn.proto.udp->remote_ip[3] = 255;

                espconn_create(&PhoneConn);//建立UDP传输

                espconn_sendto(&PhoneConn, SmartACK, 18);
                espconn_sendto(&PhoneConn, SmartACK, 18);
                espconn_sendto(&PhoneConn, SmartACK, 18);
                espconn_sendto(&PhoneConn, SmartACK, 18);
            } else {
            	//SC_TYPE_AIRKISS - support airkiss v2.0
				airkiss_start_discover();
            }
            SensorMode = Sensor_Init;
            pwm_set_duty(DUTYMAX,LED_BLUE);
            pwm_set_duty(DUTYMIN,LED_GREEN);
            pwm_start();
    		SetBit(ProgramPara.Status,BIT_ONLINE);//成功连接网络
    		SetBit(ProgramPara.Status,BIT_HASNET);//已经连接网络

			ProgramPara.SubMode = SMARTCONFIG_FINISH;
			espconn_delete(&PhoneConn);//关闭udp


            smartconfig_stop();




            break;
    }

}

void user_rf_pre_init(void)
{
}

void gas_smartconfig()
{
	uint8_t i = 0;
	/*********************************************
	 * 主程序关闭
	 ********************************************/
	os_timer_disarm(&gas_main);
	ProgramPara.Led_Cnt_second = 0;
	pwm_set_duty(DUTYMAX,LED_RED);
	pwm_set_duty(DUTYMAX,LED_BLUE);
	pwm_set_duty(DUTYMAX,LED_GREEN);
	pwm_start();
	SensorMode = Sensor_Config;

	wifi_station_disconnect();
	PhoneConn.type = ESPCONN_UDP;           //要建立的连接类型为UDP
	PhoneConn.proto.udp = &PhoneConnUdp;
	PhoneConn.proto.udp->local_port = espconn_port();; //设置本地端口
	PhoneConn.proto.udp->remote_port = 18267;//设置远程端口

    for(i = 0;i<10;i++){
        SmartACK[i+8] = ProgramPara.Dev_ID[i];
    }//装载数据
    //os_printf("\r\n%s\r\n",SmartACK);
    ClrBit(ProgramPara.init_flag_time,BIT_30S);
    ClrBit(ProgramPara.init_flag_time,BIT_10S);
    ProgramPara.flag_Time =  0;

    //ClrBit(ProgramPara.flag_Time,BIT_MINITE);

	wifi_station_disconnect();
	ProgramPara.SubMode = SMARTCONFIG_START;
	smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS
    smartconfig_start(smartconfig_done);




}


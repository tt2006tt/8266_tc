#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "ets_sys.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"
#include "user_json.h"
#include "user_interface.h"
#include "smartconfig.h"
#include "airkiss.h"
#include "tcp_udp.h"
#include  "eagle_soc.h"
#include  "pwm.h"
#include "key.h"
#include "networking.h"
#include "key.h"
//#define USE_OPTIMIZE_PRINTF	//串口打印宏

/* 不同传感器的 ID前两位不同 */
#define TEXXXTT   	0//使用新命令  但是用的传感器是mp4
#define MP_4		1//可燃气体 GS
#define MP_502		2//HC 甲醛	不报警
#define MP_9		3
#define MP_7		4	 // CO
#define MP_503		5//VO	不报警
#define MP_901		6
#define MP_2		7	//烟雾 SM
#define MP_503NEW		8	//甲醛 酒精等 传电压


#define SENSOR_TYPE		MP_503NEW   //选择传感器类型



#if(SENSOR_TYPE == MP_503NEW)
	#define PPM_MAX 3500
	#define PPM_MIN 2500
#endif


#define DUTYMAX	222222
#define DUTYMIN	0

//#define DEBUG
#ifdef DEBUG
	#define	CNT_10S	   10	//10S
	#define	CNT_30S	   20	//30S
    #define CNT_MINITE 20//20s
    #define CNT_HOUR   3 //1min
    #define CNT_DAY    60
    #define CNT_2DAY	  120
	#define	CNT_70S
#else
	#define	CNT_10S	   200	//10S
	#define	CNT_30S	   400	//20S
    #define CNT_MINITE 2400//2min
    #define CNT_HOUR   30//1hour
    #define CNT_DAY    24//1day
    #define CNT_2DAY		48	//48hour
	#define	CNT_60S    1200

#endif

#define BIT_MINITE      0
#define BIT_HOUR 	1
#define BIT_DAY 	2
#define BIT_10S      3
#define BIT_30S      4

#define BIT_RECONNECT   7  //处于重连等待状态
#define BIT_ALARMOFF    6  //强制结束报警
#define BIT_SELFTEST    5  //自测
//#define BIT_KEYOK       4
#define BIT_LOGINOK     3  //完成登录
#define BIT_HASBIAS     2  //已校正（下次就不用再校正了）
#define BIT_HASNET      1  //曾经连接过网络(触发断网重连的条件)
#define BIT_ONLINE      0  //当前已连接wifi


#define BIT_SHORT   	0  //短按
#define BIT_LONG   		1  //长按
#define BIT_WARNING		3	//报警


#define BIT_RED      0
#define BIT_GREEN    1
#define BIT_BLUE     2
#define BIT_BEEP     3

#define SMARTCONFIG_INIT        0x00
#define SMARTCONFIG_START       0x01
#define SMARTCONFIG_WAIT        0x02
#define SMARTCONFIG_IP          0x03
#define SMARTCONFIG_BROAD       0x04
#define SMARTCONFIG_TRAN        0x05
#define SMARTCONFIG_SEND        0x06
#define SMARTCONFIG_FINISH      0x07
#define SMARTCONFIG_ERROR       0xFF

/***************************
 	 	 数据长度
 ***************************/
#define LENGTH_KEY              16
#define LENGTH_DEV_ID           10
#define LENGTH_DST_ID           10
#define LENGTH_SRC_ID           10
#define LENGTH_ENCRYPT          1
#define LENGTH_IDX               2
#define LENGTH_PAYLOADLENGTH   2
#define LENGTH_COMMAND          2
#define LENGTH_PPM              4
#define LENGTH_KEY              16
#define LENGTH_SOFTNUM          2
#define LENGTH_DEVVER           4
#define LENGTH_FIRDATA          50
#define LENGTH_UPDATE LENGTH_COMMAND + LENGTH_SOFTNUM + LENGTH_DEVVER
#define LENGTH_VERSION  4
#define LENGTH_ALARMVAL 4
#define LENGTH_NETMASK  4
#define LENGTH_IPADDR   4


#define MESMAX          128


#define RESENDCNT       10
#define WebMaxSize      128
#define PATTERN_SIZE	128
#define RXMAX           128

/***************************
 	 	 加密类型
 ***************************/
#define NONEKEY         0
#define OPENKEY         1
#define PRIMARYKEY      2





typedef union
{
    uint32_t    Num;   //警报值
    uint8_t     Arr[LENGTH_ALARMVAL];
}Typdef_AlarmValue;


struct startup_config{
	char Dev_id[12];
	unsigned int adoffset;
};


extern char ubWebRxBuf[RXMAX];


struct light_saved_param {
    uint32  pwm_period;
    uint32  pwm_duty[3];
};

typedef union

{
    uint32_t    Num;
    uint8_t     Arr[LENGTH_IPADDR];
}Typdef_MyIPAddress;

typedef union
{
    uint32_t    Num;         //版本号
    uint8_t     Arr[LENGTH_VERSION];      //设备版本号
}Typdef_Version;







typedef union
{
    uint32_t    Num;
    uint8_t     Arr[LENGTH_IPADDR];
}Typdef_IPAddress;

typedef struct
{
    uint8_t DST_ID[LENGTH_DST_ID];         //目标ID
    uint8_t SRC_ID[LENGTH_SRC_ID];         //源ID
    uint8_t mems[MESMAX];           //数据长度
    uint8_t encrypt;            //加密类型
    uint16_t idx;               //包序
    uint16_t payloadlength;     //命令数据长度
    uint16_t command;           //命令
}Typdef_DataFormat;

typedef struct
{
    uint16_t payloadlength;     //命令数据长度
    uint16_t command;           //命令
    uint8_t mems[MESMAX];           //数据长度
}Typdef_UartFormat;

typedef struct
{
	uint32_t city_code;
	char	x[16];	//经度
	char 	y[16];	//纬度
	char	addr[48];
}Typdef_MapFormat;

typedef struct
{
    uint16_t  Cnt_ERROR;        //计时器，错误状态倒计时
    uint16_t  Cnt_Minite;       //计时器，分钟
    uint16_t  Cnt_Hour;       //计时器，分钟
    uint16_t  Cnt_Day;       //计时器，分钟
    uint16_t  Cnt_Key;       //计时器，按键时间
    uint8_t  Cnt_Warning0;       //计时器，分钟
    uint8_t  Cnt_Warning1;       //计时器，分钟
    uint8_t  Cnt_Warning2;       //计时器，分钟
    uint8_t  Cnt_Warning3;       //计时器，分钟
    uint8_t   SubMode;          //子模式散转
    uint8_t   flag_Time;	//时间 bit0 分钟,bit1 小时, bit2 天。
    uint8_t	  init_flag_time;	//用来判断mode_init时连接服务器的延时
    uint16_t  Led_Cnt_second;
    uint8_t   Status;           //bit 0 网络连接，    bit 1网络已连接   ，bit 2 完成校正 ,   ，bit 3 成功登陆 ,
                                //bit 4 成功获得私钥 ,bit 5 自测        , bit 6 手动停止报警 , bit 7 重连中   （bit4已经不用了）
    uint8_t Flag_Test;          //测试灯两灭   bit0 红灯 bit1 绿灯 bit2 蓝灯 bit3 蜂鸣器
    uint8_t flag_Alarm;         //报警标志位，当此位为 1 时 报警
    uint8_t Dev_ID[LENGTH_DEV_ID];       //由服务器分配的设备ID
    uint32_t PPM_H;               //读到的PPM的值
    uint32_t PPM_OFFSET;               //PPM偏移值
    uint32_t PPM_Updata;               //PPM上传值
    uint32_t PPM_Updata0;               //PPM上传值
    uint32_t PPM_Updata1;               //PPM上传值
    uint32_t PPM_Updata2;               //PPM上传值
    Typdef_AlarmValue Alarm_H;    //报警值 高电平模块
    Typdef_Version   Version;   //设备版本号
    Typdef_Version   HWVersion; //设备硬件版本号
    Typdef_Version   SWVersion; //设备软件版本号
    uint8_t Key_Type;			//记录按键状态和报警状态
}Typdef_ProgramPara;

extern Typdef_ProgramPara ProgramPara;
extern Typdef_MapFormat MapFormat;
extern Typdef_DataFormat DataFormat;
extern struct startup_config config;
extern struct startup_config configcmp;

extern char ubWebRxBuf[RXMAX];//用来储存网络数据
/***************************
 	 	 系统状态
 ***************************/
uint8 Mode;	//系统状态

#define Mode_Init    		0x01
#define Mode_Wait           0x02	//此模式用来等待  用来避免 gas_main_cb 不断调用
#define Mode_Bias			0x03
#define Mode_Config  		0x04	//smartconfig状态
#define Mode_Nomal			0x08	//正常运作状态
#define Mode_Detect			0x10
#define Mode_Warning		0x20
#define Mode_CreateSession  0x40	//websocket连接
#define Mode_Test           0x80
#define Mode_Connect        0xA0



uint8 Mode;	//系统状态
uint8 SensorMode1;	//系统状态
uint8 Mode1;	//系统状态
/***************************
 	 	 工作状态
 ***************************/


#define Sensor_Init    		0x01
#define Sensor_Normal       0x02
#define Sensor_Test			0x03	//标定
#define	Sensor_ConectError	0x04
#define	Sensor_Config		0x05
#define Sensor_DeError		0x06
#define Sensor_Test1		0x07	//自检
#define Sensor_ConnError		0x08


#define LED_GREEN    		0
#define LED_RED         	2
#define LED_BLUE			1

/***************************
 	 	 协议命令
 ***************************/

#define COMMAND_READGAS         0x3001
#define COMMAND_READGASACK      0x3002

#define COMMAND_WARNING         0x3003

#define COMMAND_HEARTRATE       0x3005
#define COMMAND_HEARTRATEACK    0x3006
#define COMMAND_UPDATEPPM       0x3007
#define COMMAND_GETNEWID        0x300b
#define COMMAND_NEWIDACK        0x300c
#define COMMAND_LOGIN           0x300d
#define COMMAND_LOGINACK        0x300e
#define	COMMAND_MAPDATA			0x3013
//#define COMMAND_UPDATEFIR       0x220f
//#define COMMAND_UPDATEFIRACK    0x2210
#define COMMAND_SETPPM          0x300f
#define COMMAND_SETPPMACK       0x3010

#define COMMAND_LEDCOLOR        0x2213
#define COMMAND_LEDCOLORACK     0x2214
#define COMMAND_SELFTEST        0x2215
#define COMMAND_SELFTESTACK     0x2216
#define COMMAND_STOPALARM       0x2219
#define COMMAND_STOPALARMACK    0x221a

#define COMMAND_READAIRDETECTOR 	0x3200
#define COMMAND_READAIRDETECTORACK 	0x3201
#define COMMAND_UPDATEAIRPPM 		0x3202
#define COMMAND_SETLEDTIME			0x3208


#define SetBit(VAR,Place)         ( (VAR) |= (uint8_t)((uint8_t)1<<(uint8_t)(Place)) )
#define ClrBit(VAR,Place)         ( (VAR) &= (uint8_t)((uint8_t)((uint8_t)1<<(uint8_t)(Place))^(uint8_t)255) )

void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata);
void ICACHE_FLASH_ATTR time_calcu_cb(void);
void gas_main_cb(void);
void uart_Tx(void);

#endif


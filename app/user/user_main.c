/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/

#include "user_config.h"
#include "tcp_udp.h"



struct startup_config config = {0};
struct startup_config configcmp = {0};
struct light_saved_param light_param;
int32 greenpwm_duty = 0;
LOCAL uint8 dir = 1;                /** 占空比加减标志 */
extern os_timer_t led_main;//用主程序轮询
extern volatile uint8_t SensorMode;

void ICACHE_FLASH_ATTR time_calcu_cb(void)
{
	if(++ProgramPara.Cnt_Key >= 60){
		ProgramPara.Key_Type = 0;
		ProgramPara.Cnt_Key = 0;
	}
	if((++Cnt_second >= CNT_10S) && ((Mode == Mode_Init)||(Mode == Mode_Bias))){
		SetBit(ProgramPara.init_flag_time,BIT_10S);
		Cnt_10second++;
		Cnt_second = 0;
		if(Cnt_10second == 2){//20S
			Cnt_10second = 0;
			//os_printf("\r\nCNT_second!!!!\r\n");
			SetBit(ProgramPara.init_flag_time,BIT_30S);
		}
	}
	if(++ProgramPara.Cnt_Minite >= CNT_MINITE){
		ProgramPara.Cnt_Minite = 0;
		//os_printf("\r\nCNT_Minite!!!!\r\n");
		SetBit(ProgramPara.flag_Time,BIT_MINITE);

		if(++ProgramPara.Cnt_Hour>= CNT_HOUR){// 3
			ProgramPara.Cnt_Hour = 0;
			//os_printf("\r\nCNT_HOUR!!!!\r\n");
			SetBit(ProgramPara.flag_Time,BIT_HOUR);
			if(++ProgramPara.Cnt_Day >= CNT_DAY){//60
				//os_printf("\r\nCNT_DAY!!!!\r\n");
				ProgramPara.Cnt_Day = 0;
				SetBit(ProgramPara.flag_Time,BIT_DAY);
			}
		}
	}

	if(ProgramPara.Cnt_Minite >= CNT_60S && ProgramPara.SubMode != SMARTCONFIG_FINISH)//2分钟依然没连上wifi 关闭smartconfig
	{
		smartconfig_stop();
		SensorMode = Sensor_Init;
		pwm_set_duty(DUTYMAX,LED_RED);
		pwm_set_duty(DUTYMAX,LED_BLUE);
		pwm_set_duty(DUTYMIN,LED_GREEN);
		pwm_start();
		ProgramPara.Led_Cnt_second = 0;
		ProgramPara.SubMode = SMARTCONFIG_FINISH;
		os_timer_disarm(&gas_main);
		os_timer_setfn(&gas_main, (os_timer_func_t *)gas_main_cb,0);
		os_timer_arm(&gas_main, 100, 1);//1s扫描一次
		Mode = Mode_Nomal;

		os_printf("\r\n smartconfig_stop \r\n");
	}
}

/******************************************************************************
 * FunctionName : Mode_init
 * Description  : 开机初始化状态 用于检测是否开机连接上wifi如果没默认连上wifi就进入 smartconfig模式
 * Parameters   : void
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR Mode_init(void)
{
	struct	ip_info init_info;
	uint8_t i ;
	wifi_get_ip_info(STATION_IF,&init_info);

	if((strcmp((char *)ipaddr_ntoa(&init_info.ip),"0.0.0.0") !=0) && (ProgramPara.init_flag_time & BIT(BIT_10S)) )//已经连上wifi
	{
		//ClrBit(ProgramPara.init_flag_time,BIT_30S);//清除30s的标志
		ClrBit(ProgramPara.init_flag_time,BIT_10S);//清除10s的标志 让其重新计时
		//os_printf("\r\n11111111111111111  \r\n");
		//os_printf("my_ip:%s",ipaddr_ntoa(&init_info.ip));
		Mode = Mode_Bias;
	}
	else if(ProgramPara.init_flag_time & BIT(BIT_30S))
	{
		ClrBit(ProgramPara.init_flag_time,BIT_10S);//清除10s的标志 让其重新计时
		ClrBit(ProgramPara.init_flag_time,BIT_30S);//清除30s的标志
		ProgramPara.Led_Cnt_second = 0;
		SensorMode = Sensor_Config;
		Mode = Mode_Config;//未连接过wifi 进入smartconfig模式
	}
}

void ICACHE_FLASH_ATTR InitOK(void)
{
	if((ProgramPara.init_flag_time & BIT(BIT_30S)))
	{
		ClrBit(ProgramPara.init_flag_time,BIT_10S);//清除10s的标志 让其重新计时
		ClrBit(ProgramPara.init_flag_time,BIT_30S);
		Mode = Mode_CreateSession;

	}

}

void ICACHE_FLASH_ATTR tcpwm_init(void)
{

	uint32 pwm_duty[3]= {DUTYMIN,DUTYMAX,DUTYMAX};
	uint32 io_info[3][3]={
	{PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO12,12},
	{PERIPHS_IO_MUX_MTCK_U,FUNC_GPIO13,13},
	{PERIPHS_IO_MUX_MTMS_U,FUNC_GPIO14,14}
	};

	pwm_init(10000,pwm_duty,3,io_info);  //pwM的管脚 10K

	//pwm_set_duty(0,3);
	pwm_start();
}


void ICACHE_FLASH_ATTR connect_test_wifi(void){
	struct station_config station_cfg;
	uint8 ssid[]="dlink_edio";         //wifi名
	uint8 password[]="epform020716";     //wifi密码
	os_strcpy(station_cfg.ssid, ssid);          //ssid名称
	os_strcpy(station_cfg.password, password);  //密码
	wifi_station_set_config(&station_cfg);      //设置WIFI帐号和密码
	wifi_station_connect();
}

void ICACHE_FLASH_ATTR led_test1(void)
{

	static uint8_t i = 0;
	static uint8_t status = 0;
	struct	ip_info init_info;
	wifi_get_ip_info(STATION_IF,&init_info);


	if(++i > 20)
	{
		i = 0;
		if(status == 0)
		{
			status = 1;
			pwm_set_duty(DUTYMAX,LED_RED);
			pwm_set_duty(DUTYMIN,LED_BLUE);
			pwm_set_duty(DUTYMAX,LED_GREEN);
		}
		else if(status == 1)
		{
			status = 2;
			pwm_set_duty(DUTYMAX,LED_GREEN);
			pwm_set_duty(DUTYMIN,LED_RED);
			pwm_set_duty(DUTYMAX,LED_BLUE);
		}
		else if(status == 2)
		{
			pwm_set_duty(DUTYMIN,LED_GREEN);
			pwm_set_duty(DUTYMAX,LED_BLUE);
			pwm_set_duty(DUTYMAX,LED_RED);
			status = 3;
		}
		else if(status == 3)
		{
			dir = 0;
			status = 4;
			ProgramPara.PPM_H = system_adc_read();
			if(ProgramPara.PPM_H < 50 || ProgramPara.PPM_H > 700){//自检失败
				pwm_set_duty(DUTYMAX,LED_GREEN);
				pwm_set_duty(DUTYMIN,LED_BLUE);
				pwm_set_duty(DUTYMAX,LED_RED);
				pwm_start();
			}
			else{
				pwm_set_duty(DUTYMAX,LED_GREEN);
				pwm_set_duty(DUTYMAX,LED_BLUE);
				pwm_set_duty(DUTYMAX,LED_RED);
				pwm_start();
				configcmp.adoffset = 106;
				config.adoffset = 106;
			    spi_flash_erase_sector(0x7C);
				spi_flash_write(0x7C * 4096, (uint32 *)&config, sizeof(config));
				spi_flash_erase_sector(0x7D);
				spi_flash_write(0x7D * 4096, (uint32 *)&configcmp, sizeof(configcmp));
				connect_test_wifi();

				//os_printf("test 1111 done!!!\r\n");
				//SensorMode = Sensor_Init;
			}

		}
		else if(status == 4)
		{
			status = 5;
		}
		else if(status == 5)
		{
			status = 6;
		}
		else if(status == 6)
		{
			status = 7;
		}
		else if(status == 7)
		{
			status = 8;
		}
		else if(status == 8)
		{
			status = 9;
		}
		else if(status == 9)
		{

			if((strcmp((char *)ipaddr_ntoa(&init_info.ip),"0.0.0.0") !=0))//已经连上wifi
			{
				//os_printf("test 2222 done!!!\r\n");
				status = 10;
				CheckIpStart();

			}
			else
			{
				status = 0;
				pwm_set_duty(DUTYMAX,LED_GREEN);
				pwm_set_duty(DUTYMAX,LED_BLUE);
				pwm_set_duty(DUTYMAX,LED_RED);
				SensorMode = Sensor_ConnError;
				//os_printf("test 2222 erro!!!\r\n");
			}
		}
		else if(status == 10)
		{
			status = 11;
		}
		else if(status == 11)
		{
			status = 12;

		}
		else if(status == 12)
		{
			status = 0;
			if(WebSocketOK == 1){
				pwm_set_duty(DUTYMIN,LED_GREEN);
				pwm_set_duty(DUTYMAX,LED_BLUE);
				pwm_set_duty(DUTYMAX,LED_RED);
				//os_printf("test 3333 done!!!\r\n");
				SensorMode = Sensor_Init;
			}
			else{
				pwm_set_duty(DUTYMAX,LED_GREEN);
				pwm_set_duty(DUTYMAX,LED_BLUE);
				pwm_set_duty(DUTYMAX,LED_RED);
				SensorMode = Sensor_ConnError;
			}

		}

		pwm_start();
	}
}

void ICACHE_FLASH_ATTR led_test(void)
{

	static uint8_t i = 0;
	static uint8_t status = 0;
	if(++i > 20)
	{
		i = 0;
		if(status == 0)
		{
			status = 1;
			pwm_set_duty(DUTYMAX,LED_RED);
			pwm_set_duty(DUTYMAX,LED_BLUE);
			pwm_set_duty(DUTYMIN,LED_GREEN);
		}
		else if(status == 1)
		{
			status = 2;
			pwm_set_duty(DUTYMAX,LED_GREEN);
			pwm_set_duty(DUTYMIN,LED_RED);
			pwm_set_duty(DUTYMAX,LED_BLUE);
		}
		else if(status == 2)
		{
			pwm_set_duty(DUTYMAX,LED_GREEN);
			pwm_set_duty(DUTYMIN,LED_BLUE);
			pwm_set_duty(DUTYMAX,LED_RED);
			status = 3;
		}
		else if(status == 3)
		{
			pwm_set_duty(DUTYMAX,LED_GREEN);
			pwm_set_duty(DUTYMAX,LED_BLUE);
			pwm_set_duty(DUTYMAX,LED_RED);
			dir = 0;
			greenpwm_duty = 0;
			config.adoffset = system_adc_read();
			if(config.adoffset < 0) {config.adoffset = 0;}
			configcmp.adoffset = config.adoffset;
		    spi_flash_erase_sector(0x7C);
			spi_flash_write(0x7C * 4096, (uint32 *)&config, sizeof(config));
			spi_flash_erase_sector(0x7D);
			spi_flash_write(0x7D * 4096, (uint32 *)&configcmp, sizeof(configcmp));
			status = 0;
			SensorMode = Sensor_Normal;

		}

		pwm_start();
	}
}

void ICACHE_FLASH_ATTR Detect(void)
{
	int adtranf = 0;
	adtranf = system_adc_read() - config.adoffset;
	if( adtranf <= 0) ProgramPara.PPM_H = 0;
	else ProgramPara.PPM_H = adtranf;

	if(++ProgramPara.Cnt_Warning3 <= 10){
		if(ProgramPara.PPM_H <= 300){//小于1.5V
			ProgramPara.Cnt_Warning0++;
			ProgramPara.PPM_Updata0 = ProgramPara.PPM_H;
		}
		else if(ProgramPara.PPM_H > 300 && ProgramPara.PPM_H <= 600){//小于3V 大于1.5V
			ProgramPara.PPM_Updata1 = ProgramPara.PPM_H;
			ProgramPara.Cnt_Warning1++;
		}
		else if(ProgramPara.PPM_H > 600){//大于3V
			ProgramPara.PPM_Updata2 = ProgramPara.PPM_H;
			ProgramPara.Cnt_Warning2++;
		}

	}
	else{

		ProgramPara.Cnt_Warning3 = 0;

		if(ProgramPara.Cnt_Warning0 >= 7){
			ProgramPara.PPM_Updata = (uint32_t)(ProgramPara.PPM_Updata0 * 5 / 10);
			pwm_set_duty(DUTYMAX,LED_RED);
			pwm_set_duty(DUTYMAX,LED_BLUE);
			pwm_start();
			ProgramPara.flag_Alarm = 0;
		}
		else if(ProgramPara.Cnt_Warning1 >= 7){
			ProgramPara.PPM_Updata = (uint32_t)(ProgramPara.PPM_Updata1 * 5 / 10);
			ProgramPara.flag_Alarm = 1;
			//pwm_set_duty(DUTYMAX,LED_GREEN);
			//pwm_set_duty(DUTYMAX,LED_RED);
			pwm_start();
		}
		else if(ProgramPara.Cnt_Warning2 >= 7){
			ProgramPara.PPM_Updata = (uint32_t)(ProgramPara.PPM_Updata2 * 5 / 10);
			ProgramPara.flag_Alarm = 2;

			pwm_set_duty(DUTYMAX,LED_GREEN);
			pwm_set_duty(DUTYMAX,LED_BLUE);
			pwm_start();
		}
		else {
			ProgramPara.Cnt_Warning0 = 0;
			ProgramPara.Cnt_Warning1 = 0;
			ProgramPara.Cnt_Warning2 = 0;

		}
		ProgramPara.Cnt_Warning0 = 0;
		ProgramPara.Cnt_Warning1 = 0;
		ProgramPara.Cnt_Warning2 = 0;
	}

}

void ICACHE_FLASH_ATTR pwmad_run(void)
{
	//os_printf("\r\nad_read~~~~\r\n%d",system_adc_read());
	if(ProgramPara.flag_Alarm == 0)
	{
		if (1 == dir)
		{
			greenpwm_duty += 4440;
			if ( greenpwm_duty >= DUTYMAX )
			{
				dir=0;
			}
			//os_printf("\r\nduty add\r\n");
		}
		else
		{
			greenpwm_duty -= 4440;
			if ( greenpwm_duty <= DUTYMIN )
			{
				dir=1;
			}
			//os_printf("\r\n duty lesss\r\n");
		}

		pwm_set_duty(greenpwm_duty,LED_GREEN);

		pwm_start();
	}
	else if(ProgramPara.flag_Alarm == 1)
	{
		if(++ProgramPara.Led_Cnt_second > 20){
			ProgramPara.Led_Cnt_second = 0;
			switch(dir){
				case	1:pwm_set_duty(100000,LED_GREEN);pwm_set_duty(DUTYMIN,LED_RED);dir = 0;
				break;
				case	0:pwm_set_duty(DUTYMAX,LED_GREEN);pwm_set_duty(DUTYMAX,LED_RED);dir = 1;//
				break;
			}
			pwm_start();

		}
	}
	else if(ProgramPara.flag_Alarm == 2)
	{
		pwm_set_duty(DUTYMAX,LED_GREEN);
		if(++ProgramPara.Led_Cnt_second > 20){
			ProgramPara.Led_Cnt_second = 0;
			switch(dir){
				case	1:pwm_set_duty(DUTYMIN,LED_RED);dir = 0;
				break;
				case	0:pwm_set_duty(DUTYMAX,LED_RED);dir = 1;
				break;
			}
			pwm_start();

		}
	}

}

void ICACHE_FLASH_ATTR led_config(void)
{
	if(++ProgramPara.Led_Cnt_second > 20){
		ProgramPara.Led_Cnt_second = 0;
		switch(dir){
			case	1:pwm_set_duty(DUTYMIN,LED_BLUE);dir = 0;
			break;
			case	0:pwm_set_duty(DUTYMAX,LED_BLUE);dir = 1;
			break;
			default:dir = 1;
			break;
		}
		pwm_start();

	}
}

void ICACHE_FLASH_ATTR pwmad_cb(void)
{
	//os_printf("\r\n SensorMode~~~~\r\n%d",SensorMode);
	time_calcu_cb();
    if((SensorMode != Sensor_Init) && (SensorMode != Sensor_Test) && (SensorMode != Sensor_Config) && (SensorMode != Sensor_Normal)
    		&& (SensorMode != Sensor_Test1) && (SensorMode != Sensor_ConnError))
    {
    	SensorMode = SensorMode1;//
    }

	switch(SensorMode)
	{
		case Sensor_Init:			led_time_calcu_cb();SensorMode1 = SensorMode;
		break;
		case Sensor_Normal:			Detect();pwmad_run();SensorMode1 = SensorMode;
		break;
		case Sensor_Test:			led_test();SensorMode1 = SensorMode;
		break;
		case Sensor_Config:			led_config();SensorMode1 = SensorMode;
		break;
		case Sensor_Test1:			led_test1();
		break;
		case Sensor_ConnError:		pwm_set_duty(DUTYMIN,LED_RED);pwm_start();
		break;
		default:					SensorMode = Sensor_Init;ProgramPara.Led_Cnt_second = 0;
		break;
	}
}



void gas_main_cb(void)
{


	//os_printf("\r\nad_read~~~~\r\n%d",SensorMode);
	//os_printf("\r\n ProgramPara.flag_Alarm~~~~\r\n%d",ProgramPara.flag_Alarm);
	//os_printf("\r\n SENSORMODE~~~~\r\n%d",SensorMode);
	//os_printf("\r\nSensorMode %d~~~~~~~~\r\n",ProgramPara.Led_Cnt_second);
	//os_printf("\r\nconfig.adoffset %d~~~~~~~~\r\n",config.adoffset);
	//os_printf("\r\nProgramPara.PPM_H~~~~\r\n%d",ProgramPara.PPM_H);


    if((Mode != Mode_Init) && (Mode != Mode_Bias) && (Mode != Mode_CreateSession) && (Mode != Mode_Config) && (Mode != Mode_Nomal)  && (Mode != Mode_Wait))
    {
        Mode = Mode1;//
    }

	switch(Mode)
	{
		case Mode_Init:				Mode_init();Mode1 = Mode;
		break;
		case Mode_Bias:				InitOK();Mode1 = Mode;
		break;
		case Mode_CreateSession:	CheckIpStart();Mode1 = Mode;  //websocket连接服务器
		break;
		case Mode_Config:         	ProgramPara.Cnt_Minite = 0;gas_smartconfig();Mode1 = Mode;  //os_printf("\r\n!!!!!text!!!\r\n");
	    break;
		case Mode_Nomal:			networking();Mode1 = Mode;
		break;
		case Mode_Wait:				Mode1 = Mode;
		break;
		default:					Mode = Mode_Init;Cnt_second = 0;
		break;
	}


}

void user_init(void)
{
	uint8_t i;
	uart_init(9600,9600);
	system_set_os_print(1);//开关打印 log 功能 0关闭 1打开
	UART_SetPrintPort(0);//默认tx口为 Log口
    wifi_set_opmode(STATION_MODE);

    tcpwm_init();
    drv_Switch_Init();

	system_update_cpu_freq(SYS_CPU_160MHZ);//
    //os_printf("\r\nSDK version:%s\n", system_get_sdk_version());
    ProgramPara.Cnt_ERROR = 1800;//错误灯（断开网络）180s后重启
    Mode = Mode_Init;
    SensorMode = Sensor_Init;


    os_memset(&config, 0, sizeof(config));
    spi_flash_read(0x7C * 4096, (uint32 *)&config, sizeof(config));
    os_memset(&configcmp, 0, sizeof(configcmp));
    spi_flash_read(0x7D * 4096, (uint32 *)&configcmp, sizeof(configcmp));

    if((os_memcmp(config.Dev_id,configcmp.Dev_id,10) == 0) && (configcmp.Dev_id[0] == 'H') && (config.Dev_id[1] == 'T'))
    {
    	os_memcpy(ProgramPara.Dev_ID,config.Dev_id,10);

    }
    else
    {
    	os_memcpy(ProgramPara.Dev_ID,"HTFXXXXXXX",10);
    	os_printf("\r\nProgramPara.Dev_ID error~~~~~~~~\r\n");
    }
    if(GPIO_INPUT_GET(GPIO_ID_PIN(4)))
    {
    }
    else//按键开机按下
    {
    	SensorMode = Sensor_Test1;
    }
    //os_printf("\r\nconfig.adoffset %d~~~~~~~~\r\n",config.adoffset);
    //os_printf("\r\nProgramPara.Dev_ID %s~~~~~~~~\r\n",ProgramPara.Dev_ID);
	/*********************************************
	 * 主程序
    ********************************************/
	os_timer_disarm(&gas_main);
	os_timer_setfn(&gas_main, (os_timer_func_t *)gas_main_cb,0);
	os_timer_arm(&gas_main, 100, 1);//1s扫描一次

	os_timer_disarm(&led_main);
	os_timer_setfn(&led_main, (os_timer_func_t *)pwmad_cb,0);
	os_timer_arm(&led_main, 50, 1);//50ms扫描一次


}

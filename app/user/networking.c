#include "networking.h"
#include "user_config.h"

uint8_t DatePackege[DataMax] = {0};
Typdef_DataFormat DataFormat = {0};	//��ʼ�����ݽṹ��
uint8_t primaryKey[16] = {0};//��ע��͵�¼�������Ķ���˽Կ
volatile uint8_t Receive = 0;
const uint8_t hardversion = 0x03;
const uint32_t softversion = 180326;
uint8_t Postdata[33] = "action=saveDevid&devid=";
extern uint8_t gassmartconfig;
extern char ubWebRxBuf[RXMAX];
extern uint8_t map_addlen;
extern struct espconn http_tcp_conn;
extern volatile uint8_t HttpOK;
void Reflash(uint16_t command,uint8_t receive,uint16_t waittime)
{
    DataFormat.command = command;
    Receive = receive;
    os_memset(ubWebRxBuf,0,RXMAX);
    //os_delay_us(1000);
}

void ICACHE_FLASH_ATTR Post_Id(void)
{
	uint8_t postdata1[33] = "action=saveDevid&devid=";
	char pbuf[194] = {0};
	postdata1[23] = ProgramPara.Dev_ID[0];
	postdata1[24] = ProgramPara.Dev_ID[1];
	postdata1[25] = ProgramPara.Dev_ID[2];
	postdata1[26] = ProgramPara.Dev_ID[3];
	postdata1[27] = ProgramPara.Dev_ID[4];
	postdata1[28] = ProgramPara.Dev_ID[5];
	postdata1[29] = ProgramPara.Dev_ID[6];
	postdata1[30] = ProgramPara.Dev_ID[7];
	postdata1[31] = ProgramPara.Dev_ID[8];
	postdata1[32] = ProgramPara.Dev_ID[9];
	os_sprintf(pbuf,"POST /%s HTTP/1.1\r\nAccept: */*\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n%s",
			"baas/tianhesm",sizeof(postdata1),"flytronlink.net",postdata1);//
	//os_printf("\r\npost 2222222  %s !!!!!!!!! %d\r\n",pbuf,os_strlen(pbuf));
	espconn_send(&http_tcp_conn, pbuf, 194);
}

void networking(void)
{
	static uint8_t i = 0;
	static y = 0;
	uint8_t x = 0;

	if(WebSocketOK)
	{
		if(ProgramPara.Cnt_ERROR >0) //����3������Ȼ����������
		{
			ProgramPara.Cnt_ERROR--;
		}
		else if(ProgramPara.Cnt_ERROR == 0)
		{
			ProgramPara.Cnt_ERROR = 180;
			espconn_disconnect(&websocket_tcp_conn);
			//os_printf("disconnect 111111111\r\n");
			ProgramPara.Led_Cnt_second = 0;
			Cnt_second = 0;
			Cnt_10second = 0;
			ClrBit(ProgramPara.init_flag_time,BIT_10S);
			ClrBit(ProgramPara.init_flag_time,BIT_30S);
			WebSocketOK = 0;
			HttpOK = 0;
			pwm_start();
			Mode = Mode_Bias;
			return ;
		}
		if(gassmartconfig == 1
			&& ProgramPara.Dev_ID[2] != 'F')
		{
			gassmartconfig = 1;
			Post_Id();
		}
		ESP8266_Monitor();

		if((ProgramPara.Status & BIT(BIT_LOGINOK)))
		{//��¼�ɹ�if(programpara->Flag_LogInOK)
			//os_printf("\r\n BIT_LOGINOK \r\n");
			if((ProgramPara.flag_Time & BIT(BIT_MINITE)))
			{
		//  ÿ�����ӷ�һ��������Debug��ֻ��20s��
				ClrBit(ProgramPara.flag_Time,BIT_MINITE);
				DataFormat.command = COMMAND_HEARTRATE;
			}
			if((ProgramPara.flag_Time & BIT(BIT_HOUR)))
			{
				//ÿСʱ�ϴ�һ��PPM��Debug��ֻ��60s��
				ClrBit(ProgramPara.flag_Time,BIT_HOUR);
				DataFormat.command = COMMAND_UPDATEPPM;
			}

			if((ProgramPara.flag_Time & BIT(BIT_DAY)))
			{
				//������һ�ι̼�(Debug��ֻ��60����)
				ClrBit(ProgramPara.flag_Time,BIT_DAY);
				//DataFormat.command = COMMAND_UPDATEFIR ;
			}

		}
		else
		{
		//��¼���ɹ�
			if(ProgramPara.Cnt_ERROR >0)
			{
				ProgramPara.Cnt_ERROR --;
			}
			else
			{
				ProgramPara.Led_Cnt_second = 0;
				Cnt_second = 0;
				Cnt_10second = 0;
				espconn_disconnect(&websocket_tcp_conn);
				os_printf("disconnect 2222222222222\r\n");
				ClrBit(ProgramPara.init_flag_time,BIT_10S);
				ClrBit(ProgramPara.init_flag_time,BIT_30S);
				WebSocketOK = 0;
				HttpOK = 0;
				Mode = Mode_Bias;//����
				ProgramPara.Cnt_ERROR = 180;
			}
		}
		//os_printf("\r\n wwz  ==== %s \r\n",test);

	}
	else//û��������websocket
	{
		if((ProgramPara.Status & BIT(BIT_HASNET)))
		{//����������Ϲ���������������˸������
			if(ProgramPara.Cnt_ERROR >0)
			{
				ProgramPara.Cnt_ERROR --;
			}
			else
			{
				Cnt_second = 0;
				Cnt_10second = 0;
				ClrBit(ProgramPara.init_flag_time,BIT_10S);
				ClrBit(ProgramPara.init_flag_time,BIT_30S);
				WebSocketOK = 0;
				HttpOK = 0;
				Mode = Mode_Bias;//����
				ProgramPara.Cnt_ERROR = 1800;
				pwm_start();

				//ClrBit(ProgramPara.Status,BIT_RECONNECT);//��������־λ
			}
		}
		else
		{//��δ����������
			//ClrBit(ProgramPara.Status,BIT_RECONNECT);//��������־λ
		}
	}
}


void ESP8266_Monitor(void)
{

	static uint8_t  Cnt_Reboot;

	  uint8_t length;//Ҫ���͵����ݰ�����
	  uint32_t AddrBias = 0x00000000; //��ַƫ�Ƴ���
	  uint8_t dst[] = "SSFFFFFFFF";//Ŀ��ID
	  uint8_t i;

	  switch(Receive){
	      case 0x01:DataFormat.command = COMMAND_GETNEWID;       break;//����µ��豸ID
	      case 0x02:DataFormat.command = COMMAND_NEWIDACK;       break;//����µ��豸ID ACK
	      case 0x04:DataFormat.command = COMMAND_LOGIN;          break;//Login���ڳɹ�����websocket�Ự����λ
	      case 0x08:DataFormat.command = COMMAND_LOGINACK;       break;//LoginACK
	      case 0x10:DataFormat.command = COMMAND_READGAS;        break;//APP��Gas�� PPMֵ
	      case 0x11:DataFormat.command = COMMAND_READGASACK;        break;//APP��Gas�� PPMֵ
	      case 0x20:DataFormat.command = COMMAND_SETPPM;         break;//����PPM����ֵ
	      case 0x30:DataFormat.command = COMMAND_LEDCOLOR;       break;//LED���Ե�
	      case 0x40:DataFormat.command = COMMAND_SELFTEST;       break;//�Բ�
	      case 0x50:DataFormat.command = COMMAND_SELFTESTACK;    break;//�Բ�ACK
	      case 0x60:DataFormat.command = COMMAND_MAPDATA;    break;
	      //case 0xE0:DataFormat.command = COMMAND_UPDATEFIR;      break;//���¹̼�
	      //case 0xF0:DataFormat.command = COMMAND_UPDATEFIRACK;   break;//���¹̼�ACK
	      default:break;
	    }
	  //os_printf("\r\nCOMMAND_=== 0x%x\r\n",DataFormat.command);
	  switch(DataFormat.command){
	  	  case COMMAND_MAPDATA:
				ProcessPackege(dst,ProgramPara.Dev_ID,PRIMARYKEY,88,COMMAND_MAPDATA);
				for(i = 0;i<4;i++)//װ�س��д���
				{
					DataFormat.mems[3-i] = MapFormat.city_code>>(8*i);
				}
				os_printf("\r\nCOMMAND_=== 0x%d\r\n",MapFormat.city_code);
				for(i = 0;i < 16;i++)
				{
					DataFormat.mems[4+i] = MapFormat.x[i];
					DataFormat.mems[20+i]= MapFormat.y[i];
				}
				for(i = 0; i < map_addlen;i++)
				{
					DataFormat.mems[36+i] = MapFormat.addr[i];
				}
				length = WebSocket_Data();
				Reflash(0x0000,0xFF,0);
				espconn_send(&websocket_tcp_conn, DatePackege, length);
	  		  break;
	  	  case COMMAND_READGAS:
	  		  Reflash(COMMAND_READGASACK,0x11,0);
	  		//os_printf("\r\nCOMMAND_READGAS === %d\r\n",DataFormat.command);
				break;
	  	  case COMMAND_READGASACK:

				for(i = 0;i<10;i++){

				  dst[i] = DataFormat.SRC_ID[i];//������������������
				}
				ProcessPackege(dst,ProgramPara.Dev_ID,PRIMARYKEY,0x000a,COMMAND_READGASACK);
				for(i = 0;i<LENGTH_PPM;i++)//װ�ص�ǰPPMֵ
				{
				  DataFormat.mems[3-i] = ProgramPara.PPM_Updata>>(8*i);
				}
				//os_memcpy(&DataFormat.mems[LENGTH_PPM],ProgramPara.Alarm_H.Arr,LENGTH_PPM);//װ��PPM�趨ֵ
				for(i = 0;i<LENGTH_PPM;i++)//װ�ص�ǰPPMֵ
				{
				  DataFormat.mems[7-i] = ProgramPara.Alarm_H.Num>>(8*i);
				}

				length = WebSocket_Data();

				Reflash(0x0000,0xFF,0);

				//ClrBit(ProgramPara.Status,BIT_SELFTEST);

				espconn_send(&websocket_tcp_conn, DatePackege, length);
	  		  	break;

	  	  case COMMAND_WARNING:
				ProcessPackege(dst,ProgramPara.Dev_ID,PRIMARYKEY,0x0006,COMMAND_WARNING);//Ŀ��ID,ԴID���������ͣ��������ݳ��ȣ�����


				for(i = 0;i<LENGTH_PPM;i++){
				  DataFormat.mems[3-i] = ProgramPara.PPM_H>>(8*i);
				}
				length = WebSocket_Data();

				Reflash(0x0000,0xFF,0);


				espconn_send(&websocket_tcp_conn, DatePackege, length);
				break;
	  	  case COMMAND_HEARTRATE:
	  		   // os_printf("\r\n  COMMAND_HEARTRATE~~~~ %s   %d\r\n",ProgramPara.Dev_ID);
				ProcessPackege(dst,ProgramPara.Dev_ID,PRIMARYKEY,0x0002,COMMAND_HEARTRATE);

				length = WebSocket_Data();
				DataFormat.command = 0x0000;

				os_memset(ubWebRxBuf,0,RXMAX);

				espconn_send(&websocket_tcp_conn, DatePackege, length);


				break;
	  	  case COMMAND_HEARTRATEACK:

				Cnt_Reboot = 0;
	  		  //DataFormat.command = 0;
	  		  //Receive = 0xFF;
				ProgramPara.Cnt_ERROR = 1800;
				Reflash(0x0000,0xFF,0);
				 //os_printf("\r\n  COMMAND_HEARTRATEACK~~~~ %s \r\n",ProgramPara.Dev_ID);
				break;
	  	  case COMMAND_UPDATEPPM:
	  		  	ProgramPara.Cnt_ERROR = 1800;
				ProcessPackege(dst,ProgramPara.Dev_ID,PRIMARYKEY,0x0006,COMMAND_UPDATEPPM);
				//ProgramPara.PPM_Updata = (uint32_t)(ProgramPara.PPM_H * 5 / 10);
				//os_printf("\r\n  COMMAND_UPDATEPPM~~~~ %d \r\n",ProgramPara.PPM_Updata);
				for(i = 0;i<4;i++){
				  DataFormat.mems[3-i] = ProgramPara.PPM_Updata>>(8*i);
				}
				length = WebSocket_Data();

				Reflash(0x0000,0xFF,0);

				espconn_send(&websocket_tcp_conn, DatePackege, length);
				break;
	  	  case COMMAND_GETNEWID:
				ProcessPackege(dst,ProgramPara.Dev_ID,OPENKEY,0x000c,COMMAND_GETNEWID);

				os_memcpy(DataFormat.mems,ProgramPara.Dev_ID,LENGTH_DEV_ID);
				length = WebSocket_Data();

				Reflash(0x0000,0xFF,0);

				espconn_send(&websocket_tcp_conn, DatePackege, length);
				break;
	  	  case COMMAND_NEWIDACK:
	  		//os_printf("\r\n  COMMAND_NEWIDACK~~~~\r\n");

				for(i = 0;i<10;i++){

				  ProgramPara.Dev_ID[i] = DataFormat.mems[i];//������������������
				}
				os_memcpy(config.Dev_id,ProgramPara.Dev_ID,10);
				os_memcpy(configcmp.Dev_id,ProgramPara.Dev_ID,10);
				if(gassmartconfig == 1)
				{
					Post_Id();
				}
				os_printf("ProgramPara.Dev_ID == %s \r\n",ProgramPara.Dev_ID);
			    spi_flash_erase_sector(0x7C);
			    spi_flash_write(0x7C * 4096, (uint32 *)&config, sizeof(config));
			    spi_flash_erase_sector(0x7D);
			    spi_flash_write(0x7D * 4096, (uint32 *)&configcmp, sizeof(configcmp));
				os_printf("ProgramPara.Dev_ID == %s \r\n",ProgramPara.Dev_ID);
				//os_printf("COMMAND_NEWIDACK\r\n");
				Reflash(COMMAND_LOGIN,0x04,0);

				break;
	  	  case COMMAND_LOGIN:
				ProcessPackege(dst,ProgramPara.Dev_ID,OPENKEY,0x000A,COMMAND_LOGIN);
				ProgramPara.HWVersion.Num = hardversion;
				ProgramPara.SWVersion.Num = softversion;

				for(i = 0;i<4;i++){
				  DataFormat.mems[3-i] = ProgramPara.HWVersion.Num>>(8*i);
				}
				for(i = 0;i<4;i++){
				  DataFormat.mems[7-i] = ProgramPara.SWVersion.Num>>(8*i);
				}
				length = WebSocket_Data();

				Reflash(0x0000,0xFF,0);
				//os_printf("\r\nCOMMAND_LOGIN === %d\r\n",COMMAND_LOGIN);
				//os_printf("\r\nCOMMAND_LOGIN  playload_length === %d\r\n", DataFormat.payloadlength);
				espconn_send(&websocket_tcp_conn, DatePackege, length);

	  		  	  /*  8266����δ֪ԭ��ʹ��memcpy zalloc ��malloc ��buff������ ��� �˴��ַ���ʹ������*/



				break;
	  	  case COMMAND_LOGINACK:
				os_memcpy(primaryKey,DataFormat.mems,LENGTH_KEY);
				//os_printf("\r\nprimaryKeyprimaryKey 111=== %s  \r\n ",DataFormat.mems);
				//os_printf("\r\nprimaryKeyprimaryKey === %s  \r\n ",primaryKey);
				SetBit(ProgramPara.Status,BIT_LOGINOK);
				Reflash(0x0000,0xFF,0);
				//Reflash(COMMAND_MAPDATA,0x60,0);
				break;

	  	  case COMMAND_SETPPM:
				os_memcpy(dst,DataFormat.SRC_ID,LENGTH_SRC_ID);
				for(i = 0;i<4;i++){
					ProgramPara.Alarm_H.Arr[i]= DataFormat.mems[3-i];
				}


				if(ProgramPara.Alarm_H.Num > PPM_MAX){
					ProgramPara.Alarm_H.Num= PPM_MAX;
				}
				//else if(ProgramPara.Alarm_H.Num< PPM_MIN){
					//ProgramPara.Alarm_H.Num= PPM_MIN;
				//}


				ProcessPackege(dst,ProgramPara.Dev_ID,PRIMARYKEY,0x0002,COMMAND_SETPPMACK);
				length = WebSocket_Data();

				Reflash(0x0000,0xFF,0);
				espconn_send(&websocket_tcp_conn, DatePackege, length);
				break;
	  	  case COMMAND_LEDCOLOR:
	  		  	os_memcpy(dst,DataFormat.SRC_ID,LENGTH_SRC_ID);
	  		  	os_printf("COMMAND_LEDCOLOR dst == %s  \r\n",dst);
				if(DataFormat.mems[2] == 0xff)
				{
					if(DataFormat.mems[1] == 0xff)
					{
						if(DataFormat.mems[0] == 0xff)
						{//������
							os_printf("COMMAND_LEDCOLOR BEEEEEEEEEEP��������  \r\n");
							ProgramPara.Flag_Test =0;
							SetBit(ProgramPara.Flag_Test,BIT_BEEP);
						}
					}
					else
					{//��
						os_printf("COMMAND_LEDCOLOR BLUEEEEEEEE��������  \r\n");
						ProgramPara.Flag_Test =0;
						SetBit(ProgramPara.Flag_Test,BIT_BLUE);
					}
				}
				else
				{
					if(DataFormat.mems[1] == 0xff)
					{//��
						os_printf("COMMAND_LEDCOLOR GREEEEEEEEENNNNNN��������  \r\n");
						ProgramPara.Flag_Test =0;
						SetBit(ProgramPara.Flag_Test,BIT_GREEN);
					}
					else
					{//��
						os_printf("COMMAND_LEDCOLOR REDDDDDDD��������  \r\n");
						ProgramPara.Flag_Test =0;
						SetBit(ProgramPara.Flag_Test,BIT_RED);
					}
				}

				ProcessPackege(dst,ProgramPara.Dev_ID,PRIMARYKEY,0x0002,COMMAND_LEDCOLORACK);
				length = WebSocket_Data();

				Reflash(0x0000,0xFF,0);

				ClrBit(ProgramPara.Status,BIT_SELFTEST);
				espconn_send(&websocket_tcp_conn, DatePackege, length);
				break;
	  	  case COMMAND_STOPALARM:
				SetBit(ProgramPara.Status,BIT_ALARMOFF);
				ClrBit(ProgramPara.flag_Time,BIT_MINITE);
				ProgramPara.Cnt_Minite = 0;
				Reflash(0x0000,0xFF,0);

				break;
	  	  default:
				break;
}
}

void WebRecvData_Proc(char * buf,uint8 index)
{

	uint16_t i = index;
	rxcount = 0; //��¼��־ ��0
	uint8_t open_key[]      = "Ftl_201207166688";//˽Կ�������ʼ��ע�ᡢ��¼
	//if(buf[i] != 0x82) //�����һ�������ǿ����İ�ȫ�ԣ����ǻ������Щ������������
	//return;
	i++;//1
	//len = buf[i%RXMAX];//%max
	i++;//2
	//if(buf[i%RXMAX] != 'F')//
	{
		//return ;
	}
	i++;//3
	//------------------��ȡDST_ID------------------
	os_memcpy(DataFormat.DST_ID,&buf[i],10);
	i += 10;
	//------------------��ȡSRC_ID------------------
	os_memcpy(DataFormat.SRC_ID,&buf[i],10);
	i += 10;
	//------------------��ȡ��������--------------
	DataFormat.encrypt= buf[i];
	i++;
	//----------------------��ȡ����-----------------
	DataFormat.idx = (uint16_t)((buf[i]<<8) | (buf[(i+1)]));
	i += 2;
	//------------------��ȡ���ݲ��ֳ���----------
	DataFormat.payloadlength = (uint16_t)(buf[i]<<8) | (buf[(i+1)]);
	i += 2;
	if(DataFormat.encrypt == 1)//��Կ����
	{
		RC4(&buf[i],open_key,DataFormat.payloadlength);
	}
	else if(DataFormat.encrypt == 3)//˽Կ����
	{
		RC4(&buf[i],primaryKey,DataFormat.payloadlength);
	}

	//----------------------��ȡ����-----------------
	DataFormat.command = (uint16_t)(buf[i]<<8) | (buf[(i+1)]);
	i = i + 2;
	//----------------------���ݲ���-----------------
	if((DataFormat.payloadlength <= (MESMAX + 2)) && (DataFormat.payloadlength >2))
	{
		os_memcpy(DataFormat.mems,&buf[i],DataFormat.payloadlength - 2);
	}
	os_memset(ubWebRxBuf,0,RXMAX);

	switch(DataFormat.command)
	{
		case COMMAND_NEWIDACK:      Receive = 0x02;     break;
		case COMMAND_LOGINACK:      Receive = 0x08;     break;
		case COMMAND_READGAS:       Receive = 0x10;     break;
		case COMMAND_SETPPM:        Receive = 0x20;     break;
		case COMMAND_LEDCOLOR:      Receive = 0x30;     break;
		//case COMMAND_SELFTEST:      Receive = 0x40;     break;
		//case COMMAND_UPDATEFIR:     Receive = 0xE0;     break;
		//case COMMAND_UPDATEFIRACK:  Receive = 0xF0;     break;
		default:break;
	}

}


/******************************************************************************
 * FunctionName : ProcessPackege
 * Description  : RC4����
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
void RC4(uint8_t* message,uint8_t* key,uint8_t msglen)//
{

	  uint8_t iS[256] ={0};
	  uint16_t i = 0;
	  uint8_t keylen = LENGTH_KEY;//(sizeof(key)/sizeof(key[0]))
	  uint8_t temp = 0;

	  uint8_t j = 0;
	  uint8_t x = 0;

	  uint8_t *iInputChar = 0;

	  uint8_t iOutputChar[DataMax] = {0};

	  uint8_t t = 0 ;

	  iInputChar = message;

	  for ( i = 0; i < 256; i++)
	  {
	    iS[i] = i;
	  }
	  for ( i = 0; i < 256; i++)
	  {
	    j = (j + iS[i] + key[i%keylen]) % 256;
	    temp = iS[i];
	    iS[i] = iS[j];
	    iS[j] = temp;
	  }
	  for (x = 0,i=0,j=0; x < msglen; x++)
	  {
	    i = (i + 1) % 256;
	    j = (j + iS[i]) % 256;
	    temp = iS[i];
	    iS[i] = iS[j];
	    iS[j] = temp;
	    t = (iS[i] + (iS[j] % 256)) % 256;
	    iOutputChar[x] = (uint8_t) (iInputChar[x] ^ iS[t]);
	  }
	  os_memset( message,0,msglen);
	  os_memcpy( message,iOutputChar,msglen);
}

/******************************************************************************
 * FunctionName : ProcessPackege
 * Description  : Ŀ���ַ�ӿ�
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
void ProcessPackege(uint8_t *dst,uint8_t *src,uint8_t encrypt,uint16_t length,uint16_t command)
{
  os_memcpy(DataFormat.DST_ID,dst,LENGTH_DST_ID);
  os_memcpy(DataFormat.SRC_ID,src,LENGTH_SRC_ID);
  DataFormat.encrypt = encrypt;
  DataFormat.payloadlength = length;
  DataFormat.command = command;

}

/******************************************************************************
 * FunctionName : WebSocket_Data
 * Description  : websocket ���ݴ��
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
uint8_t WebSocket_Data(void)
{
  uint8_t sendlen = 0;
  uint8_t i;
  uint8_t open_key[]      = "Ftl_201207166688";//˽Կ�������ʼ��ע�ᡢ��¼
  for(i=0;i<DataMax;i++){
    DatePackege[i] = 0;
  }
  //-----------�涨�����ݸ�ʽͷ----------
  DatePackege[0] = 0x82;
  DatePackege[2] = 0x00;
  DatePackege[3] = 0x00;
  DatePackege[4] = 0x00;
  DatePackege[5] = 0x00;
  //--------------------Flag-------------------
  DatePackege[6] = 'F';
  sendlen = 7;
  //--------------------DST_ID-----------------
  os_memcpy(&DatePackege[sendlen],DataFormat.DST_ID,LENGTH_DST_ID);
  sendlen += LENGTH_DST_ID;//17
  //--------------------SRC_ID-----------------

  os_memcpy(&DatePackege[sendlen],DataFormat.SRC_ID,LENGTH_SRC_ID);
  sendlen += LENGTH_SRC_ID;//27
  //------------------��������---------------

  DatePackege[sendlen] = DataFormat.encrypt;
  ///os_memcpy(&DatePackege[sendlen],(uint8_t*)&DataFormat.encrypt,LENGTH_ENCRYPT);
  sendlen += LENGTH_ENCRYPT;//28
  //--------------------����------------------

  DataFormat.idx +=1;
  DatePackege[sendlen]      |= ((DataFormat.idx >> 8 )&0xFF);
  DatePackege[sendlen+1]    |= ((DataFormat.idx >> 0 )&0xFF);
 // memcpy(&DatePackege[sendlen],(uint16_t*)&DataFormat.idx,LENGTH_IDX);

  sendlen += LENGTH_IDX;//30
  //-------------�������ݳ���--------------

  DatePackege[sendlen]      |= ((DataFormat.payloadlength >> 8 )&0xFF);
  DatePackege[sendlen+1]    |= ((DataFormat.payloadlength >> 0 )&0xFF);
  sendlen += LENGTH_PAYLOADLENGTH;//32

  //-------------------����-------------------
  DatePackege[sendlen]      |= ((DataFormat.command >> 8 )&0xFF);
  DatePackege[sendlen+1]    |= ((DataFormat.command >> 0 )&0xFF);
  //memcpy(&DatePackege[sendlen],(uint16_t*)&DataFormat.command,LENGTH_COMMAND);
  sendlen += LENGTH_COMMAND;//34

  //-------------------����-------------------
  for(i = 0;i < (DataFormat.payloadlength - 2);i++)
  {
	  DatePackege[sendlen + i] = DataFormat.mems[i];
  }
  //os_memcpy(&DatePackege[sendlen],DataFormat.mems,(DataFormat.payloadlength - 2));

  if(DataFormat.encrypt == 1){
    RC4(&DatePackege[sendlen - LENGTH_COMMAND],open_key,DataFormat.payloadlength);
  }
  else if(DataFormat.encrypt == 2){
    RC4(&DatePackege[sendlen - LENGTH_COMMAND],primaryKey,DataFormat.payloadlength);
  }

  sendlen += DataFormat.payloadlength;

  DatePackege[1] = 0x80 | (sendlen - 6-2);

  return (sendlen - 2);
}

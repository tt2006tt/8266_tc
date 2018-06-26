
#include "user_config.h"

#define DataMax         128
extern volatile uint8_t Receive;
extern uint8_t DatePackege[DataMax];
extern uint8_t Postdata[];
void RC4(uint8_t* message,uint8_t* key,uint8_t msglen);
void ProcessPackege(uint8_t *dst,uint8_t *src,uint8_t encrypt,uint16_t length,uint16_t command);
void ESP8266_Monitor(void);
void Reflash(uint16_t command,uint8_t receive,uint16_t waittime);
uint8_t WebSocket_Data(void);

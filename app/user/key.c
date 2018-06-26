/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: key.c
 *
 * Description: key driver, now can use different gpio and install different function
 *
 * Modification history:
 *     2014/5/1, v1.0 create this file.
*******************************************************************************/

#include "os_type.h"
#include "osapi.h"
#include "user_config.h"

extern volatile uint8_t SensorMode;
/**
 *******************************************************************************
 * @brief      ����������������
 *******************************************************************************
 */
#define SWITCH_Pin_NUM         4
#define SWITCH_Pin_FUNC        FUNC_GPIO4
#define SWITCH_Pin_MUX         PERIPHS_IO_MUX_GPIO4_U


#define SWITCH_Pin_Rd_Init()   GPIO_DIS_OUTPUT(SWITCH_Pin_NUM)
#define SWITCH_Pin_Wr_Init()   GPIO_OUTPUT_SET(SWITCH_Pin_NUM,0)
#define SWITCH_Pin_Set_High()  GPIO_OUTPUT_SET(SWITCH_Pin_NUM,1)
#define SWITCH_Pin_Set_Low()   GPIO_OUTPUT_SET(SWITCH_Pin_NUM,0)
#define SWITCH_Pin_State       ( GPIO_INPUT_GET(SWITCH_Pin_NUM) != 0 )
    /**
 *******************************************************************************
 * @brief     ������ر���
 *******************************************************************************
 */
static struct keys_param switch_param;
static struct single_key_param *switch_signle;
/**
 *******************************************************************************
 * @brief       ���ض̰�״̬������
 * @param       [in/out]  void
 * @return      void
 * @note        None
 *******************************************************************************
 */
static void Switch_ShortPress_Handler( void )
{
	//os_printf("\r\nSwitch_ShortPress_Handler!!!!!!!!!\r\n");
	ProgramPara.Key_Type++;
	if(ProgramPara.Key_Type >=3)
	{
		//os_printf("\r\nSwitch_ShortPress_Handler!!!!!!!!!\r\n");
		SensorMode = Sensor_Test;
	}
}

/**
 *******************************************************************************
 * @brief       ���س���״̬������
 * @param       [in/out]  void
 * @return      void
 * @note        None
 *******************************************************************************
 */
static void Switch_LongPress_Handler( void )
{
	//os_printf("\r\nSwitch_LongPress_Handler!!!!!!!!!\r\n");
	ProgramPara.Led_Cnt_second = 0;
	pwm_set_duty(DUTYMAX,LED_RED);
	pwm_set_duty(DUTYMAX,LED_BLUE);
	pwm_set_duty(DUTYMIN,LED_GREEN);
	pwm_start();
	Mode = Mode_Config;
	SensorMode = Sensor_Config;

}


/**
*******************************************************************************
 * @brief       �����ʼ������
 * @param       [in/out]  void
 * @return      void
 * @note        None
*******************************************************************************
 */
void drv_Switch_Init( void )
{
    switch_signle = key_init_single( SWITCH_Pin_NUM, SWITCH_Pin_MUX,
                                     SWITCH_Pin_FUNC,
									 &Switch_LongPress_Handler,
                                     &Switch_ShortPress_Handler );
    switch_param.key_num = 1;
    switch_param.single_key = &switch_signle;

    key_init( &switch_param );
}

LOCAL void key_intr_handler(struct keys_param *keys);

/******************************************************************************
 * FunctionName : key_init_single
 * Description  : init single key's gpio and register function
 * Parameters   : uint8 gpio_id - which gpio to use
 *                uint32 gpio_name - gpio mux name
 *                uint32 gpio_func - gpio function
 *                key_function long_press - long press function, needed to install
 *                key_function short_press - short press function, needed to install
 * Returns      : single_key_param - single key parameter, needed by key init
*******************************************************************************/
struct single_key_param *ICACHE_FLASH_ATTR
key_init_single(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func, key_function long_press, key_function short_press)
{
    struct single_key_param *single_key = (struct single_key_param *)os_zalloc(sizeof(struct single_key_param));

    single_key->gpio_id = gpio_id;
    single_key->gpio_name = gpio_name;
    single_key->gpio_func = gpio_func;
    single_key->long_press = long_press;
    single_key->short_press = short_press;

    return single_key;
}

/******************************************************************************
 * FunctionName : key_init
 * Description  : init keys
 * Parameters   : key_param *keys - keys parameter, which inited by key_init_single
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
key_init(struct keys_param *keys)
{
    uint8 i;

    ETS_GPIO_INTR_ATTACH(key_intr_handler, keys);

    ETS_GPIO_INTR_DISABLE();

    for (i = 0; i < keys->key_num; i++) {
        keys->single_key[i]->key_level = 1;

        PIN_FUNC_SELECT(keys->single_key[i]->gpio_name, keys->single_key[i]->gpio_func);

        gpio_output_set(0, 0, 0, GPIO_ID_PIN(keys->single_key[i]->gpio_id));

        gpio_register_set(GPIO_PIN_ADDR(keys->single_key[i]->gpio_id), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
                          | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
                          | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));

        //clear gpio14 status
        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(keys->single_key[i]->gpio_id));

        //enable interrupt
        gpio_pin_intr_state_set(GPIO_ID_PIN(keys->single_key[i]->gpio_id), GPIO_PIN_INTR_NEGEDGE);
    }

    ETS_GPIO_INTR_ENABLE();
}

/******************************************************************************
 * FunctionName : key_5s_cb �Ѿ���Ϊ2s
 * Description  : long press 5s timer callback
 * Parameters   : single_key_param *single_key - single key parameter
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
key_5s_cb(struct single_key_param *single_key)
{
    os_timer_disarm(&single_key->key_5s);

    // low, then restart
    if (0 == GPIO_INPUT_GET(GPIO_ID_PIN(single_key->gpio_id))) {
        if (single_key->long_press) {
            single_key->long_press();
        }
    }
}

/******************************************************************************
 * FunctionName : key_50ms_cb
 * Description  : 50ms timer callback to check it's a real key push
 * Parameters   : single_key_param *single_key - single key parameter
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
key_50ms_cb(struct single_key_param *single_key)
{
    os_timer_disarm(&single_key->key_50ms);
    // high, then key is up
    if (1 == GPIO_INPUT_GET(GPIO_ID_PIN(single_key->gpio_id))) {
        os_timer_disarm(&single_key->key_5s);
        single_key->key_level = 1;
        gpio_pin_intr_state_set(GPIO_ID_PIN(single_key->gpio_id), GPIO_PIN_INTR_NEGEDGE);

        if (single_key->short_press) {
            single_key->short_press();
        }
    } else {

        gpio_pin_intr_state_set(GPIO_ID_PIN(single_key->gpio_id), GPIO_PIN_INTR_POSEDGE);
    }
}

/******************************************************************************
 * FunctionName : key_intr_handler
 * Description  : key interrupt handler
 * Parameters   : key_param *keys - keys parameter, which inited by key_init_single
 * Returns      : none
*******************************************************************************/
LOCAL void
key_intr_handler(struct keys_param *keys)
{
    uint8 i;
    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

    for (i = 0; i < keys->key_num; i++) {
        if (gpio_status & BIT(keys->single_key[i]->gpio_id)) {
            //disable interrupt
            gpio_pin_intr_state_set(GPIO_ID_PIN(keys->single_key[i]->gpio_id), GPIO_PIN_INTR_DISABLE);

            //clear interrupt status
            GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(keys->single_key[i]->gpio_id));

            if (keys->single_key[i]->key_level == 1) {
                // 2s, restart & enter softap mode
                os_timer_disarm(&keys->single_key[i]->key_5s);
                os_timer_setfn(&keys->single_key[i]->key_5s, (os_timer_func_t *)key_5s_cb, keys->single_key[i]);
                os_timer_arm(&keys->single_key[i]->key_5s, 2000, 0);//2s
                keys->single_key[i]->key_level = 0;
                gpio_pin_intr_state_set(GPIO_ID_PIN(keys->single_key[i]->gpio_id), GPIO_PIN_INTR_POSEDGE);
            } else {
                // 50ms, check if this is a real key up
                os_timer_disarm(&keys->single_key[i]->key_50ms);
                os_timer_setfn(&keys->single_key[i]->key_50ms, (os_timer_func_t *)key_50ms_cb, keys->single_key[i]);
                os_timer_arm(&keys->single_key[i]->key_50ms, 50, 0);
            }
        }
    }
}


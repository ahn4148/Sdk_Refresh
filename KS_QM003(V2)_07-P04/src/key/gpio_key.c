////////////////////////////////////////////////////////////////////////////////
//                   Mountain View Silicon Tech. Inc.
//		Copyright 2014, Mountain View Silicon Tech. Inc., ShangHai, China
//                   All rights reserved.
//
//		Filename	:power_key_driver.c
//
//		Description	:
//					power keyboard driver(soft push botton)
//
//             maintainer: lujiangang
//
//		Changelog	:
//					2014-09-25	Created Source File
///////////////////////////////////////////////////////////////////////////////
#include "type.h"
#include "adc.h"
#include "app_config.h"
#include "os.h"
#include "gpio.h"
#include "irqs.h"
#include "msgq.h"
#include "app_msg.h"
#include "wakeup.h"
#include "bt_app_func.h"
#include "bt_control_api.h"
#include "sys_app.h"

#define 	GPIO_KEY_JITTER_TIME		10			//消抖时间，该时间和软开关开关机硬件时间有关
#define 	GPIO_KEY_CP_TIME			500		
#define 	GPIO_KEY_PWRON_TIME			10000	
#define 	GPIO_KEY_SCAN_JITTER		500			

#ifdef FUNC_GPIO_KEY_EN

#ifdef GPIO_KEY1_PORT_BIT
  #define GPIO_KEY1_PORT_IN				  (GPIO_KEY1_PORT_OUT - 1)
  #define GPIO_KEY1_PORT_IE				  (GPIO_KEY1_PORT_OUT + 1)
  #define GPIO_KEY1_PORT_OE				  (GPIO_KEY1_PORT_OUT + 2)
  #define GPIO_KEY1_PORT_PU				  (GPIO_KEY1_PORT_OUT + 5)
  #define GPIO_KEY1_PORT_PD				  (GPIO_KEY1_PORT_OUT + 6)
#endif

#ifdef GPIO_KEY2_PORT_BIT
  #define GPIO_KEY2_PORT_IN				  (GPIO_KEY2_PORT_OUT - 1)
  #define GPIO_KEY2_PORT_IE				  (GPIO_KEY2_PORT_OUT + 1)
  #define GPIO_KEY2_PORT_OE				  (GPIO_KEY2_PORT_OUT + 2)
  #define GPIO_KEY2_PORT_PU				  (GPIO_KEY2_PORT_OUT + 5)
  #define GPIO_KEY2_PORT_PD				  (GPIO_KEY2_PORT_OUT + 6)
#endif

typedef enum _GPIO_KEY_STATE
{
	GPIO_KEY_STATE_IDLE,
	GPIO_KEY_STATE_JITTER,
	GPIO_KEY_STATE_PRESS_DOWN,
	GPIO_KEY_STATE_CP,

} GPIO_KEY_STATE;

TIMER			GpioKeyWaitTimer;
TIMER			GpioKeyScanTimer;
GPIO_KEY_STATE	GpioKeyState;
uint16_t		GpioKeyMsg;
										
static const uint16_t GpioKeyEvent[2][5] = 
{
//	PDS(按键开始)				SPR(短按松开)			CPS(长按开始)			CPH(长按保持)			  	CPR(长按松开)
	{MSG_NONE,				MSG_WIFI_PREV_CH,	MSG_WIFI_PREV_CH,	 MSG_NONE,			  	MSG_NONE	},	//SW1
	{MSG_NONE,				MSG_WIFI_NEXT_CH,   MSG_WIFI_NEXT_CH,    MSG_NONE,			  	MSG_NONE  	},	//SW5
};	

// Initialize POWER_KEY scan operation.
void GpioKeyScanInit(void)
{
	GpioKeyState = GPIO_KEY_STATE_IDLE;
	GpioKeyMsg = MSG_NONE;
#ifdef GPIO_KEY1_PORT_BIT
	GpioClrRegOneBit(GPIO_KEY1_PORT_OE, GPIO_KEY1_PORT_BIT);
	GpioClrRegOneBit(GPIO_KEY1_PORT_PU, GPIO_KEY1_PORT_BIT);
	GpioClrRegOneBit(GPIO_KEY1_PORT_PD, GPIO_KEY1_PORT_BIT);
	GpioSetRegOneBit(GPIO_KEY1_PORT_IE, GPIO_KEY1_PORT_BIT);
#endif

#ifdef GPIO_KEY2_PORT_BIT
	GpioClrRegOneBit(GPIO_KEY2_PORT_OE, GPIO_KEY2_PORT_BIT);
	GpioClrRegOneBit(GPIO_KEY2_PORT_PU, GPIO_KEY2_PORT_BIT);
	GpioClrRegOneBit(GPIO_KEY2_PORT_PD, GPIO_KEY2_PORT_BIT);
	GpioSetRegOneBit(GPIO_KEY2_PORT_IE, GPIO_KEY2_PORT_BIT);
#endif

	APP_DBG("Gpio Key  Init\n");
}

uint8_t GetGpioKeyIndex(void)							
{
	if(!(GpioGetReg(GPIO_KEY1_PORT_IN) & GPIO_KEY1_PORT_BIT))
	{
		return 0;
	}
	else if(!(GpioGetReg(GPIO_KEY2_PORT_IN) & GPIO_KEY2_PORT_BIT))
	{
		return 1;
	}
	
	return 0xff;
}

void InterruptGpioKeyScan(void)							
{
	static uint8_t PrevGpioKeyIndex = 0xff;
	uint8_t GpioKeyIndex = 0xff;

	GpioKeyIndex = GetGpioKeyIndex();

	if (0xff != GpioKeyIndex) {
		TimeOutSet(&GpioKeyScanTimer, GPIO_KEY_SCAN_JITTER);
	}
	
	switch(GpioKeyState)
	{
		case GPIO_KEY_STATE_IDLE:
			if(GpioKeyIndex == 0xff)
			{
				PrevGpioKeyIndex = 0xff;
			}
			else
			{	
				TimeOutSet(&GpioKeyWaitTimer, GPIO_KEY_JITTER_TIME);
				PrevGpioKeyIndex = GpioKeyIndex;
				GpioKeyState = GPIO_KEY_STATE_JITTER;
			}
			break;
			
		case GPIO_KEY_STATE_JITTER:
			if(PrevGpioKeyIndex != GpioKeyIndex)
			{
				GpioKeyState = GPIO_KEY_STATE_IDLE;
			}
			else if(IsTimeOut(&GpioKeyWaitTimer))
			{
				GpioKeyState = GPIO_KEY_STATE_PRESS_DOWN;
				TimeOutSet(&GpioKeyWaitTimer, GPIO_KEY_CP_TIME);
				//GpioKeyMsg = GpioKeyEvent[PrevGpioKeyIndex][0];			//return key sp value
			}
			break;
			
		case GPIO_KEY_STATE_PRESS_DOWN:
			if(PrevGpioKeyIndex != GpioKeyIndex)
			{
				GpioKeyState = GPIO_KEY_STATE_IDLE;
				GpioKeyMsg = GpioKeyEvent[PrevGpioKeyIndex][1];
			}
			else if(IsTimeOut(&GpioKeyWaitTimer))
			{
				GpioKeyState = GPIO_KEY_STATE_CP;
				GpioKeyMsg = GpioKeyEvent[PrevGpioKeyIndex][2];
			}
			break;
			
		case GPIO_KEY_STATE_CP:
			//此处仅保证一次按键不会响应多次短按
			if(PrevGpioKeyIndex != GpioKeyIndex)
			{
				GpioKeyState = GPIO_KEY_STATE_IDLE;
			}
			break;
			
		default:
			GpioKeyState = GPIO_KEY_STATE_IDLE;
			break;
	}
}


uint16_t GpioKeyScan(void)							
{
	uint16_t Msg = MSG_NONE;
	
	if ((MSG_NONE != GpioKeyMsg) && IsTimeOut(&GpioKeyScanTimer)) {
		Msg = GpioKeyMsg;
		GpioKeyMsg = MSG_NONE;
	}

	return Msg;
}

#endif
//end 

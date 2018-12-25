///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2012, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: key.c
//  maintainer: Halley
///////////////////////////////////////////////////////////////////////////////
#include "app_config.h"
#include "beep.h"
#include "sys_app.h"
#include "dac.h"
#include "sys_vol.h"
#include "dev_detect_driver.h"
#ifdef FUNC_WIFI_EN
#include "wifi_control.h"
#include "wifi_uart_com.h"
#include "wifi_mcu_command.h"
#endif


#if (defined(FUNC_ADC_KEY_EN) || defined(FUNC_IR_KEY_EN) || defined(FUNC_CODING_KEY_EN) || defined(FUNC_POWER_KEY_EN)) || defined(FUNC_IIC_KEY_EN)

extern void I2cKeyInit(void);
extern void AdcKeyInit(void);
extern void IrKeyInit(void);
extern void CodingKeyInit(void);
extern void GpioKeyScanInit(void);
extern void PowerKeyScanInit(void);

extern uint16_t I2cKeyScan(void);
extern uint16_t AdcKeyScan(void);
extern uint16_t IrKeyScan(void);
extern uint16_t CodingKeyScan(void);
extern uint16_t GpioKeyScan(void);
extern uint16_t PowerKeyScan(void);
extern uint16_t GetIrKeyState(void);

#ifdef FUNC_AUDIO_MENU_EN
static TIMER MenuTimer;
#endif
#ifdef FUNC_KEY_BEEP_SOUND_EN
static TIMER BeepDelayMuteTimer;
#endif

void KeyInit(void)
{
#ifdef FUNC_ADC_KEY_EN
	AdcKeyInit();
#endif

#ifdef FUNC_IR_KEY_EN
	IrKeyInit();
#endif

#ifdef FUNC_CODING_KEY_EN
	CodingKeyInit();
#endif

#if (defined(FUNC_POWER_KEY_EN) && defined(USE_POWERKEY_SOFT_PUSH_BUTTON))
	PowerKeyScanInit();
#endif
	
#ifdef FUNC_IIC_KEY_EN 
	I2cKeyInit();
#endif
	
#ifdef FUNC_GPIO_KEY_EN 
	GpioKeyScanInit();
#endif	
	
#ifdef FUNC_AUDIO_MENU_EN
	TimeOutSet(&MenuTimer, 0);
	gSys.AudioSetMode = AUDIO_SET_MODE_MAIN_VOL;	
#endif
	
#ifdef FUNC_KEY_BEEP_SOUND_EN
  TimeOutSet(&BeepDelayMuteTimer, 0);
#endif
}

#ifdef FUNC_KEY_BEEP_SOUND_EN
bool IsBeepSoundEnd(void)
{
	if(gSys.CurModuleID == MODULE_ID_IDLE)
	{
		return TRUE;
	}
	return IsTimeOut(&BeepDelayMuteTimer);
}
#endif

#ifdef FUNC_KEY_CPH_EN
uint8_t GetKeyState(void)
{
	uint8_t State = 0;
	
#ifdef FUNC_ADC_KEY_EN
	if(State == FALSE)
	{
		State = GetAdcKeyState();
	}
#endif

#ifdef FUNC_IR_KEY_EN
	if(State == FALSE)
	{
		State = GetIrKeyState();
	}
#endif
	
#ifdef FUNC_IIC_KEY_EN 
	if(State == FALSE)
	{
		State = GetI2cKeyState();
	}
#endif
	return State;
}
#endif
void KeyScan(void)
{
	uint16_t Msg = MSG_NONE;
#ifdef FUNC_KEY_BEEP_SOUND_EN
	static bool KeyBeepSoundFlag = FALSE;
  uint32_t SampleRateTmp;
#endif

#ifdef FUNC_AUDIO_MENU_EN
	if((gSys.AudioSetMode != AUDIO_SET_MODE_MAIN_VOL) && IsTimeOut(&MenuTimer))
	{
		gSys.AudioSetMode = AUDIO_SET_MODE_MAIN_VOL;
		DBG("menu time out!\n");
	}
#endif
	
#ifdef FUNC_ADC_KEY_EN
	if(Msg == MSG_NONE)
	{
		Msg = AdcKeyScan();
	}
#endif

#ifdef FUNC_IR_KEY_EN
	if(Msg == MSG_NONE)
	{
		Msg = IrKeyScan();
	}
#endif

#ifdef FUNC_CODING_KEY_EN
	if(Msg == MSG_NONE)
	{
		Msg = CodingKeyScan();
	}
#endif

#if (defined(FUNC_POWER_KEY_EN) && defined(USE_POWERKEY_SOFT_PUSH_BUTTON))
	if(Msg == MSG_NONE)
	{
		Msg = PowerKeyScan();
	}
#endif
	
#ifdef FUNC_GPIO_KEY_EN
	if(Msg == MSG_NONE)
	{
		Msg = GpioKeyScan();
	}
#endif
	
#ifdef FUNC_IIC_KEY_EN 
	if(Msg == MSG_NONE)
	{
		Msg = I2cKeyScan();
	}
#endif

#ifdef FUNC_KEY_BEEP_SOUND_EN
//��Щ����Unmute����ʱ������Beep beep���ʵ���ʱ���ڲ���
  if(KeyBeepSoundFlag)
	{	
#if defined(FUNC_AMP_MUTE_EN) && defined(AMP_SILENCE_MUTE_EN)
		if(PastTimeGet(&BeepDelayMuteTimer) >= 50)
#endif
		{
      KeyBeepSoundFlag = FALSE;		
	  	SampleRateTmp = DacAdcSampleRateGet();
      if(0 == SampleRateTmp)
      {
        SampleRateTmp = 44100;
      }
		  BeepStart(2800, 258, SampleRateTmp, 35, 10);
	  }
	}	
#endif	
	
#ifdef FUNC_KEY_DBCK_EN                                  //����˫������
	if(Msg == MSG_NONE || Msg == MSG_PLAY_PAUSE)
	{
		static uint16_t PrevKeyMsg = MSG_NONE;
	  static TIMER PowerKeyTimer;
		if(PrevKeyMsg == MSG_NONE && Msg != MSG_NONE)
		{
		  TimeOutSet(&PowerKeyTimer, 500);
			PrevKeyMsg = Msg;
			Msg = MSG_NONE;
			DBG("Start!\n");
		}
		else
		{
			if(IsTimeOut(&PowerKeyTimer) && Msg == MSG_NONE)
			{
				Msg = PrevKeyMsg;
				PrevKeyMsg = MSG_NONE;				
			}
			else if(Msg != MSG_NONE && Msg == PrevKeyMsg)
			{
			  Msg = MSG_STOP;
			  PrevKeyMsg = MSG_NONE;
			  DBG("End.......!\n");
			}	
		}
	}
#endif
	
#ifdef FUNC_KEY_CPH_EN
		{
			static uint16_t KeyCpCount;
		  if(Msg == MSG_WIFI_WPS)
		  {
			  KeyCpCount++;
				if(KeyCpCount == 10)
			  {
				  Msg = MSG_WIFI_WPS;
			  }
				else
				{
					Msg = MSG_NONE;
				}
			}
			
			if(Msg == MSG_STOP)
		  {
			  KeyCpCount++;
				if(KeyCpCount == 10)
			  {
				  Msg = MSG_STOP;
			  }
				else
				{
					Msg = MSG_NONE;
				}
			}
			
			if(GetKeyState() == FALSE)
			{
				KeyCpCount = 0;
			}
	  }
#endif
		
	if(Msg != MSG_NONE)
	{
#if defined(FUNC_WIFI_ALI_PROJECT_EN)    //������Ŀ�������н�ֹ���а�������
		if(((gWiFi.WPSState == WIFI_STATUS_WPS_SCANNING) && (Msg != MSG_WIFI_WPS) && (Msg != MSG_MODE))
			 || ((gSys.CurModuleID == MODULE_ID_WIFI) && (gWiFi.InitState == FALSE) && (Msg != MSG_MODE)))
		{
				Msg = MSG_NONE;
				return;
		}
#endif		
		
		// quick response
		if(Msg == MSG_MODE)
		{
			SetQuickResponseFlag(TRUE);
			SetModeSwitchState(MODE_SWITCH_STATE_START);
		}
#ifdef FUNC_KEY_BEEP_SOUND_EN
		if((Msg != MSG_WIFI_MIC) && (Msg != MSG_WIFI_TALK))
		{
      KeyBeepSoundFlag = TRUE;		
#if defined(FUNC_AMP_MUTE_EN) && defined(AMP_SILENCE_MUTE_EN)
	  	AmpMuteControl(0);
		  TimeOutSet(&BeepDelayMuteTimer, 150);
#endif
		}
#endif

#ifdef  FUNC_UPDATE_CONTROL	
		{
			extern uint8_t ConfirmUpgradeFlag;
			if(ConfirmUpgradeFlag == 1)
			{
				if(Msg == MSG_PLAY_PAUSE)
					ConfirmUpgradeFlag = 2;
			}
		}
#endif


#ifdef FUNC_AUDIO_MENU_EN
		//�˵������������������ӡ�����������Ϊ��ͬ�Ĺ���
		if(Msg == MSG_MENU)
		{
			gSys.AudioSetMode = (gSys.AudioSetMode + 1) % AUDIO_SET_MODE_SUM;	
			TimeOutSet(&MenuTimer, MENU_TIMEOUT);			
		}
		else if((Msg != MSG_VOL_UP) && (Msg != MSG_VOL_DW))
		{
			gSys.AudioSetMode = AUDIO_SET_MODE_MAIN_VOL;	
		}		
		
		if((Msg == MSG_VOL_UP) || (Msg == MSG_VOL_DW))
		{
			TimeOutSet(&MenuTimer, MENU_TIMEOUT);	
			//��MSG_VOL_UP��MSG_VOL_DWת��Ϊ��Ӧ�ĸ��ù�����Ϣ		
			switch(gSys.AudioSetMode)
			{
				case AUDIO_SET_MODE_MIC_VOL:
					Msg = (Msg == MSG_VOL_UP) ? MSG_MIC_VOL_UP : MSG_MIC_VOL_DW;
					break;				
				case AUDIO_SET_MODE_ECHO_DEPTH:
					Msg = (Msg == MSG_VOL_UP) ? MSG_ECHO_DEPTH_UP : MSG_ECHO_DEPTH_DW;
					break;				
				case AUDIO_SET_MODE_ECHO_DELAY:
					Msg = (Msg == MSG_VOL_UP) ? MSG_ECHO_DELAY_UP : MSG_ECHO_DELAY_DW;
					break;				
				case AUDIO_SET_MODE_TREB:
					Msg = (Msg == MSG_VOL_UP) ? MSG_TREB_UP : MSG_TREB_DW;
					break;				
				case AUDIO_SET_MODE_BASS:
					Msg = (Msg == MSG_VOL_UP) ? MSG_BASS_UP : MSG_BASS_DW;
					break;
			}
		}				
#endif

#ifdef FUNC_SLEEP_EN
		gSys.SleepTimeCnt = FALSE;
		gSys.SleepStartPowerOff = FALSE;
#endif

#ifdef FUNC_SLEEP_LEDOFF_EN
		gSys.SleepLedOffCnt = FALSE;
		gSys.SleepLedOffFlag = FALSE;
#endif

#ifdef FUNC_WIFI_EN                             //WiFi�����н�ֹ��������
		if(WiFiFirmwareUpgradeStateGet() != TRUE)
#endif
		{
			APP_DBG("Key Msg : %d\n", Msg);
			MsgSend(Msg);
		}
	}
}
#endif
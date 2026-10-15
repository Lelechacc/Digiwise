#include "common.h"
#include "key.h"
#include "keymsgprocess.h"
#include "state.h"
#include "timer.h"
#include "param.h"

KEY_CB keyCB;

// --------------------------------------------------------------------------
// 2026-10-14 17:29 CST | defer single-key UP messages after i + DEC
static BOOL decPowerKeyUpRecoveryRequest = FALSE;
// --------------------------------------------------------------------------

void KEY_Init(void)
{	 
	GPIO_InitType GPIO_InitStructure;
	
	// Enable GPIO clock  
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOD, ENABLE); 
	
	// GPIO配置
	GPIO_InitStruct(&GPIO_InitStructure);

 	GPIO_InitStructure.Pin            = GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_10;
  	GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
  	GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
  	GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
  	GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Input;
  	GPIO_InitPeripheral(GPIOD, &GPIO_InitStructure);

	// 一定要在KEYMSG_RegisterKeyScanCallback()函数之前调用
	KEYMSG_Init();
	
	// 注册按键扫描接口
	KEYMSG_RegisterKeyScanCallback(KEY_CALLBACK_Scan);
}

// 定时器回调重新释放+、-键弹起请求
void KEY_CALLBACK_ClearAddDecKeyUpMaskRequest(uint32 param)
{
	// 清除+键和-键的释放屏蔽请求
	KEYMSG_ClearMaskRequest(KEY_NAME_ADD, KEY_MSG_UP);
	KEYMSG_ClearMaskRequest(KEY_NAME_DEC, KEY_MSG_UP);

	// +键重新开放
	keyMsgCB.addKeyInvalidRequest = FALSE;
}

// 定时器回调重新释放+、i键弹起请求
void KEY_CALLBACK_ClearAddInfoKeyUpMaskRequest(uint32 param)
{
	// 清除+键和i键的释放屏蔽请求
	KEYMSG_ClearMaskRequest(KEY_NAME_ADD, KEY_MSG_UP);
	KEYMSG_ClearMaskRequest(KEY_NAME_POWER, KEY_MSG_UP);

	// +键重新开放
	keyMsgCB.addKeyInvalidRequest = FALSE;
}

// --------------------------------------------------------------------------
// 2026-10-14 17:29 CST | restore i/DEC release messages after both are up
void KEY_CALLBACK_ClearDecPowerKeyUpMaskRequest(uint32 param)
{
	KEYMSG_ClearMaskRequest(KEY_NAME_DEC, KEY_MSG_UP);
	KEYMSG_ClearMaskRequest(KEY_NAME_POWER, KEY_MSG_UP);
}
// --------------------------------------------------------------------------

uint8 keyTest = 0;

void KEY_CALLBACK_Scan(uint8* p8bitKeyValueArray)
{
	uint8 keyMask = 0x07;
	uint8 keyAddDecMask = 0x03; 	// 加减同时按时
	uint8 keyAddInfo = 0x05;		// +,i同时按下
	// --------------------------------------------------------------------------
	// 2026-10-14 17:29 CST | bit1 = DEC, bit2 = i/POWER
	uint8 keyDecPowerMask = 0x06;
	// --------------------------------------------------------------------------
	uint8 mask = (0xFF>>(8-KEY_FILTER_LEVEL_N));
	uint8 i;
	uint8 keyValue = 0;
	uint8 keyRead = 0;
	
	keyRead = GPIO_ReadInputDataBit(GPIOD, GPIO_PIN_7); // 电源键
	keyRead <<= 1;
	keyRead |= GPIO_ReadInputDataBit(GPIOD, GPIO_PIN_9);//-
	keyRead <<= 1;	
	keyRead |= GPIO_ReadInputDataBit(GPIOD, GPIO_PIN_10);//+
	
	// ■■这里增加按键滤波措施■■
	// 【第一步】将读取到的按键放入缓冲区
	// --------------------------------------------------------------------------
	// 2026-10-14 17:29 CST | only the three physical keys need GPIO filtering
	for(i=0; i<=KEY_NAME_POWER; i++)
	// --------------------------------------------------------------------------
	{
		// 读取到的每个按键的信息添加到对应的缓冲区里
		keyCB.buff[i] <<= 1;
		keyCB.buff[i] |= (keyRead>>i)&0x01;
	}

	// 【第二步】缓冲区处理，连续N次读到同一个状态才确认，N可设定
	// --------------------------------------------------------------------------
	// 2026-10-14 17:29 CST | do not filter virtual combination keys as GPIOs
	for(i=0; i<=KEY_NAME_POWER; i++)
	// --------------------------------------------------------------------------
	{
		// 当前按键缓冲区从低位开始连续N个均为1，判定为释放
		if((keyCB.buff[i] & mask) == mask)
		{
			keyCB.lastValue[i] = 0x01<<i;
		}
		// 从低位开始连续N个均为0，则判定为按下
		else if(0 == (keyCB.buff[i] & mask))
		{
			keyCB.lastValue[i] = 0;
		}
		// 其余场景，为不稳定期，不处理
		else
		{}

		keyValue |= keyCB.lastValue[i];
	}
	
	PARAM_SetKeyValue(keyRead);

	// 复位虚拟键为抬起状态
	// --------------------------------------------------------------------------
	// 2026-10-14 17:29 CST | all three virtual keys start released (active low)
	keyValue |= (uint8)(KEY_MASK_ADD_DEC | KEY_MASK_ADD_POWER | KEY_MASK_DEC_POWER);
	// --------------------------------------------------------------------------
	
	// 判断组合键是否开启
	if (!KEYMSG_GetGroupKeyUpRecoveryRequest())
	{
		// 三个按键全部释放，清除按键消息全局屏蔽请求
		if (keyMask == (keyValue & keyMask))
		{
			// 清除全局屏蔽请求
			KEYMSG_ClearGlobalMaskRequest();

			// 组合键引起的屏蔽+-键弹起功能，所以当按键释放后指定延时重新开启UP功能
			if (KEYMSG_GetAddDecKeyUpRecoveryRequest())
			{
				KEYMSG_SetAddDecKeyUpRecoveryRequest(FALSE);
			
				// 开启一个+-键弹起后定时器，指定时间恢复弹起功能
				TIMER_AddTask(TIMER_ID_RECOVERY_KEY_UP_ADD_DEC,
								KEY_SCAN_TIME*2,
								KEY_CALLBACK_ClearAddDecKeyUpMaskRequest,
								TRUE,
								1,
								ACTION_MODE_ADD_TO_QUEUE);
			}

			// 组合键引起的屏蔽+ i键弹起功能，所以当按键释放后指定延时重新开启UP功能
			if (KEYMSG_GetAddInfoKeyUpRecoveryRequest())
			{
				KEYMSG_SetAddInfoKeyUpRecoveryRequest(FALSE);

				// 开启一个+i键弹起后定时器，指定时间恢复弹起功能
				TIMER_AddTask(TIMER_ID_RECOVERY_KEY_UP_ADD_INFO,
								KEY_SCAN_TIME*2,
								KEY_CALLBACK_ClearAddInfoKeyUpMaskRequest,
								TRUE,
								1,
								ACTION_MODE_ADD_TO_QUEUE);
			}
			// ------------------------------------------------------------------
			// 2026-10-14 17:29 CST | restore i/DEC UP after the last key is up
			if (decPowerKeyUpRecoveryRequest)
			{
				decPowerKeyUpRecoveryRequest = FALSE;
				TIMER_AddTask(TIMER_ID_RECOVERY_KEY_UP_DEC_POWER,
								KEY_SCAN_TIME * 2,
								KEY_CALLBACK_ClearDecPowerKeyUpMaskRequest,
								TRUE,
								1,
								ACTION_MODE_ADD_TO_QUEUE);
			}
			// ------------------------------------------------------------------
		}

		// 检测到加减按键同时按下，根据原理图修改(bit0, +; bit1, -; bit2, i)
		if (0 == (keyValue & keyAddDecMask))
		{
			// 只要是组合键按下立刻允许当全部按键释放后重新恢复+-键弹起功能
			KEYMSG_SetAddDecKeyUpRecoveryRequest(TRUE);

			// 推车助力使能后+键的任意消息都拦截
			if (PARAM_IsPushAssistOn())
			{
				return;
			}
		
			// 将虚拟的按键置位
			keyValue &= ~KEY_MASK_ADD_DEC;

			// 消除单独的按键
			keyValue |= keyAddDecMask;
	
			// 屏蔽加键和减键的释放消息
			KEYMSG_SetMaskRequest(KEY_NAME_ADD, KEY_MSG_UP);
			KEYMSG_SetMaskRequest(KEY_NAME_DEC, KEY_MSG_UP);
		}
	
		// 检测到+,i按键同时按下，根据原理图修改(bit0, +; bit1, -; bit2, i)
		if (0 == (keyValue & keyAddInfo))
		{
			// 只要是组合键按下立刻允许当全部按键释放后重新恢复+-键弹起功能
			KEYMSG_SetAddInfoKeyUpRecoveryRequest(TRUE);

			// 将虚拟的按键置位
			keyValue &= ~KEY_MASK_ADD_POWER;

			// 消除单独的按键
			keyValue |= keyAddInfo;

			KEYMSG_SetMaskRequest(KEY_NAME_POWER, KEY_MSG_UP);
			KEYMSG_SetMaskRequest(KEY_NAME_ADD, KEY_MSG_UP);
		}

		// ------------------------------------------------------------------
		// 2026-10-14 17:29 CST | i + DEC is a distinct virtual key
		if (0 == (keyValue & keyDecPowerMask))
		{
			decPowerKeyUpRecoveryRequest = TRUE;
			keyValue &= (uint8)(~KEY_MASK_DEC_POWER);
			keyValue |= keyDecPowerMask;
			KEYMSG_SetMaskRequest(KEY_NAME_DEC, KEY_MSG_UP);
			KEYMSG_SetMaskRequest(KEY_NAME_POWER, KEY_MSG_UP);
		}
		// ------------------------------------------------------------------
	}
	else
	{
		KEYMSG_ClearMaskRequest(KEY_NAME_POWER, KEY_MSG_UP);
		KEYMSG_ClearMaskRequest(KEY_NAME_ADD, KEY_MSG_UP);
		KEYMSG_ClearMaskRequest(KEY_NAME_DEC, KEY_MSG_UP);
	}
	
	keyTest = keyValue;
	*p8bitKeyValueArray = keyValue;

	// 只要有按键触发就立刻重设自动关机时间
	if (keyValue != KEY_MASK_ALL)
	{
		// 重置自动关机任务
		STATE_ResetAutoPowerOffControl();
	}
}


#include "common.h"
#include "system.h"
#include "timer.h"
#include "delay.h"
#include "state.h"
#include "param.h"
#include "iap.h"
#include "uartdrive.h"
#include "dtconfig.h"
#include "keyMsgProcess.h"
#include "powerCtl.h"
#include "key.h"
#include "adc.h"
#include "lcd.h"
#include "backlight.h"
#include "BleProtocol.h"
#include <stdio.h>


// 状态机结构体定义
STATE_CB stateCB;
// 开机数字逐位显示计数器
static uint8 bootDigitCount = 0;

// 每次动画定时到期，在主循环的任务队列中执行
static void STATE_BootDigitTick(uint32 param)
{
	(void)param; // 本动画不需要定时任务传来的参数

	// 如果已经不在开机动画状态，就不再继续
	if (stateCB.state != STATE_BOOT_DIGIT)
	{
		TIMER_KillTask(TIMER_ID_BOOT_DIGIT);
		return;
	}

	if (bootDigitCount < 10)
	{
		bootDigitCount++;
		LCD_ShowBootDigits(bootDigitCount);
	}
	else
	{
		TIMER_KillTask(TIMER_ID_BOOT_DIGIT);
		STATE_EnterState(STATE_POWER_KEY_KEYUP);
	}
}

// 状态机初始化
void STATE_Init(void)
{
	// 默认状态为空
	stateCB.state = STATE_NULL;
	stateCB.preState = STATE_NULL;
	
	stateCB.dataMode = RID_DATA_MODE_TRIP;

	// 延时进入入口状态
	TIMER_AddTask(TIMER_ID_STATE_CONTROL,
					10,
					STATE_EnterState,
					STATE_NULL,
					1,
					ACTION_MODE_ADD_TO_QUEUE);	
}




// 重置自动关机任务
void STATE_ResetAutoPowerOffControl(void)
{
	uint32 time = PARAM_GetPowerOffTime();

	// 时间为0不关机
	if (0 == time)
	{
		return;
	}
	
	time *= 60000UL;

#if __SYSTEM_DEBUG__			// 研发调试状态，不自动关机
#else
	TIMER_AddTask(TIMER_ID_AUTO_POWER_OFF,
					time,
					STATE_EnterState,
					STATE_SHOW_POWER_OFF,
					1,
					ACTION_MODE_ADD_TO_QUEUE);
#endif
}


// 档位切换
void STATE_Service_AssistLevelChange(uint32 param)
{
	uint16 uwAssist = PARAM_GetAssistLevel();	
	uint16 uwAssistMax = PARAM_GetMaxAssist();
	
	if (param)
	{ 	
		// 拦截长按推车助力后+键第一次弹起的消息
		if (keyMsgCB.addKeyInvalidRequest)
		{
			keyMsgCB.addKeyInvalidRequest = FALSE;
			return;
		}
		
		uwAssist++;
		LIMIT_TO_MAX(uwAssist, uwAssistMax);	
		
	}
    else
    {  	
		// 当前为p档，则关闭p档，并拦截按键释放的消息
		if (PARAM_IsPushAssistOn())
		{
			STATE_Service_AssistPushOnOff(FALSE);

			return;
		}
		
        CHECK_PARAM_SAME_RETURN(ASSIST_0, uwAssist);
		uwAssist --;
	}
	
	if(PARAM_GetBleConnectState())
	{
		// 蓝牙上传档位消息
		BLE_PROTOCOL_SendCmdControlAck(ASSIST, 0x00, param);
	}
	else
	{
		//上传蓝牙档位消息里面设置的档位
		PARAM_SetAssistLevel((ASSIST_ID_E)uwAssist);
	}
	
}
   
// 大灯
void STATE_Service_LightOnOff(uint32 param)
{

	BOOL bLigth = (BOOL)(!(PARAM_GetLightSwitch()));
    
	PARAM_SetLightSwitch(bLigth);
	
	// 大灯开启后，屏幕亮度变为最暗
	if (bLigth)
	{
		BACK_LIGHT_Start(BACK_LIGHT_LEVEL_20_100);
	}
	// 大灯关闭后，恢复之前的背光等级
	else
	{
		BACK_LIGHT_Start((BACK_LIGHT_LEVEL_E)PARAM_GetBrightness());
	}

	if(PARAM_GetBleConnectState())
	{
		// 蓝牙上传大灯状态改变信息
		BLE_PROTOCOL_SendCmdControlAck(LIGHT, 0x00, bLigth);
	}
}

// 推车助力
void STATE_Service_AssistPushOnOff(uint32 onoff)
{
	if (PARAM_GetPushSpeedSwitch())
	{	
		// 打开推车助力
		PARAM_EnablePushAssist(TRUE);

		LCD_DisplayAssistUp(TRUE);

		// 注销所有按键服务
		KEYMSG_StopAllService();

		// 置位+键推车助力后拦截标志
		keyMsgCB.addKeyInvalidRequest = TRUE;

		// 清除-键释放消息屏蔽
		KEYMSG_ClearMaskRequest(KEY_NAME_DEC, KEY_MSG_UP);
		
		// -键释放，停止p档
		KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_UP, STATE_Service_AssistLevelChange, FALSE);

		// 注册电源键长按关机
		KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_HOLD, STATE_EnterState, STATE_SHOW_POWER_OFF);
	}
	else
	{
		PARAM_EnablePushAssist(FALSE);

		LCD_DisplayAssistUp(FALSE);
		
		// 强制开启全局屏蔽功能
		stateCB.preState = STATE_NULL;
		STATE_EnterState(stateCB.state);

		KEYMSG_SetAddDecKeyUpRecoveryRequest(TRUE);
	}

	if (!onoff)
	{
		PARAM_EnablePushAssist(FALSE);

		LCD_DisplayAssistUp(FALSE);
		
		// 强制开启全局屏蔽功能
		stateCB.preState = STATE_NULL;
		STATE_EnterState(stateCB.state);

		KEYMSG_SetAddDecKeyUpRecoveryRequest(TRUE);
	}
}

// 进入菜单页面
void STATE_Service_EnterMenu(uint32 param)
{
	if (0 == PARAM_GetSpeed())
	{
		STATE_EnterState(STATE_MENU_ROOT);
	}
}

// 骑行界面信息切换
void STATE_Service_ChangeDataDisplayMode(uint32 param)
{
	if(!(stateCB.powerOnKeyUpFlag)) 
	{
		stateCB.powerOnKeyUpFlag =TRUE;
		return;
	}
	stateCB.dataMode ++;
	stateCB.dataMode %= RID_DATA_MODE_MAX;
}


/*****************************************************************************
******************************************************************************
******************************菜单库设置内容**********************************
******************************************************************************
******************************************************************************/
// 参数初始化
void STATE_MenuParam(uint32 param)
{

	// 闪烁初始化
	stateCB.menu.root.blinkAble =  TRUE;
	stateCB.menu.root.menuId = 0;

	// 单位
	stateCB.menu.root.unit = PARAM_GetUnit();

	// 限速
	stateCB.menu.root.speedLimit = PARAM_GetUnit() == UNIT_METRIC ? paramCB.nvm.param.common.speed.limitVal:limitKmhToMph[(paramCB.nvm.param.common.speed.limitVal+5)/10]*10;

	// 关机时间
	stateCB.menu.root.powerTime = PARAM_GetPowerOffTime();
	for(uint8 i =0; i< CONFIG_POWER_OFFTIME_MAX;i++)
	{	
		// 查找索引
		if(stateCB.menu.root.powerTime == Config_PowerOffTime[i])
		{
			stateCB.menu.root.powerTimeIndex = i;
		}
	}
	

	// 背光 
	stateCB.menu.root.lightLevel = PARAM_GetBrightness();
	for(uint8 i =0; i< CONFIG_LIGHTLEVEL_MAX;i++)
	{	
		// 查找索引
		if(stateCB.menu.root.lightLevel == Config_lightLevel[i])
		{
			stateCB.menu.root.lightLevelIndex = i;
		}
	}

	// 版本号显示
	sprintf((char *)stateCB.menu.root.hmiFwVer, (char *)"31000");
	sprintf((char *)stateCB.menu.root.hmiHwVer, (char *)"01");

	sprintf((char *)stateCB.menu.root.controllerVer, (char *)"01");
	sprintf((char *)stateCB.menu.root.batVer, (char *)"01");

	
	// 轮径
	stateCB.menu.root.wheelSizeId = PARAM_GetWheelSizeID();
	stateCB.menu.root.wheelSize = PARAM_GetNewWheelSizeInch();
	stateCB.menu.root.wheelSizeIdIndex = 0;	
	for(uint8 i =0; i< CONFIG_WHELLID_MAX;i++)
	{	
		// 查找索引
		if(stateCB.menu.root.wheelSizeId == Config_wheelIdSize[i][0])
		{
			stateCB.menu.root.wheelSizeIdIndex = i;
			break;
		}
	}

	// 周长
	stateCB.menu.root.perimeter = PARAM_GetNewperimeter();	

	// 电压
	stateCB.menu.root.voltage = PARAM_GetBatteryVoltageLevel();
	stateCB.menu.root.voltageIndex = 0;
	for(uint8 i =0; i< CONFIG_VOLTLEVET_X_MAX;i++)
	{	
		// 查找索引
		if(stateCB.menu.root.voltage == Config_VoltageLevel[i][0])
		{
			stateCB.menu.root.voltageIndex = i;
			break;
		}
	}

	// 欠压
	stateCB.menu.root.lowVoltage = PARAM_GetLowVoltageThreshold() / 100;

	//限流
	stateCB.menu.root.currentLimit = PARAM_GetCurrentLimit()/1000;

	// 最大档位
	stateCB.menu.root.maxAssit = PARAM_GetMaxAssist();
	for(uint8 i =0; i< CONFIG_MAXASSIT_NUM;i++)
	{	
		// 查找索引
		if(stateCB.menu.root.maxAssit == Config_MaxAssit[i])
		{
			stateCB.menu.root.maxAssitIndex= i;
			break;
		}
	}
	//每档PWM
	for(uint8 i =0; i<= stateCB.menu.root.maxAssit;i++)
	{	
		stateCB.menu.root.pwmFixed[i] = paramCB.nvm.param.protocol.pwmFixed[i];
	}
	stateCB.menu.root.options = 1;

	// 测试磁钢
	stateCB.menu.root.speedSensor = PARAM_GetCycleOfSteelNum();

	// 定速巡航
	stateCB.menu.root.cruiseSwitch = PARAM_GetCruiseEnableSwitch();

	// 恢复出厂
	stateCB.menu.root.reset = FALSE;

}


// 闪烁控制
void STATE_BlinkBack(uint32 param)
{
  stateCB.menu.root.blinkAble =  TRUE;
  TIMER_ChangeTime(TIMER_ID_LCD_BLINK,500);
}


// 清除里程和恢复出厂设置返回
void STATE_ClearTripAndResetBack(uint32 param)
{
	stateCB.menu.root.clearState = FALSE;
	stateCB.menu.root.reset = FALSE;
}
					  	
// 普通菜单信息选择
void STATE_MenuChoose(uint32 param)
{	

	MENU_ID_E menuId = stateCB.state ==  STATE_MENU_ROOT ? Config_menu[stateCB.menu.root.menuId]:Config_advMenu[stateCB.menu.root.menuId];
    
    // 重置一分钟返回菜单
	TIMER_ResetTimer(TIMER_ID_MENU_QUIT);
 
	// 清除里程中
	if(stateCB.menu.root.clearState)
	{
		return; 
	}

	// 恢复出厂设置中
	if(stateCB.menu.root.reset)
	{
		return; 
	}

	// 单位
	PARAM_SetUnit(stateCB.menu.root.unit);

	// 以公制保存
	paramCB.nvm.param.common.speed.limitVal = UNIT_METRIC == PARAM_GetUnit() ? stateCB.menu.root.speedLimit :limitMphToKmp[stateCB.menu.root.speedLimit/10]*10;
	uartProtocolCB.paramSetOK = FALSE;	
	// 关机时间
	PARAM_SetPowerOffTime(stateCB.menu.root.powerTime);
    STATE_ResetAutoPowerOffControl();

	// 背光 
	PARAM_SetBrightness(stateCB.menu.root.lightLevel);
	
	// 大灯开启后，屏幕亮度变为最暗
	if (PARAM_GetLightSwitch() && (menuId != MENU_ID_AUTO_POWER_OFF_TIME))
	{
		BACK_LIGHT_Start(BACK_LIGHT_LEVEL_20_100);
	}
	// 大灯关闭后，恢复之前的背光等级
	else
	{
		BACK_LIGHT_Start((BACK_LIGHT_LEVEL_E)PARAM_GetBrightness());
	}

	// 轮径
	PARAM_SetWheelSizeID(stateCB.menu.root.wheelSizeId);
	PARAM_SetNewWheelSizeInch(stateCB.menu.root.wheelSize);
	PARAM_SetNewperimeter(stateCB.menu.root.perimeter);

	// 周长
	PARAM_SetNewperimeter(stateCB.menu.root.perimeter);

	// 电压等级
	PARAM_SetBatteryVoltageLevel(stateCB.menu.root.voltage);
	
	// 欠压
	PARAM_SetLowVoltageThreshold(stateCB.menu.root.lowVoltage*100);

	// 限流
	PARAM_SetCurrentLimit(stateCB.menu.root.currentLimit*1000);

	// 最大档位
	PARAM_SetMaxAssist(stateCB.menu.root.maxAssit);




	//每档PWM
	for(uint8 i =0; i<= stateCB.menu.root.maxAssit;i++)
	{	
		paramCB.nvm.param.protocol.pwmFixed[i] = stateCB.menu.root.pwmFixed[i];
	}

	// 测试磁钢
	PARAM_SetCycleOfSteelNum(stateCB.menu.root.speedSensor);
	
	// 定速巡航
	PARAM_SetCruiseEnableSwitch(stateCB.menu.root.cruiseSwitch);

	// 切换界面就保存数据
	// NVM更新，立即写入
	NVM_SetDirtyFlag(TRUE);

	// 菜单ID处理
 	switch(menuId)
 	{
		case MENU_ID_LEVEL_PER:


			stateCB.menu.root.options++;
			if(stateCB.menu.root.options > stateCB.menu.root.maxAssit)
			{
				stateCB.menu.root.options = 1;
				// 菜单ID累加
				stateCB.menu.root.menuId ++; 
			}
			break;

		default:
		
			stateCB.menu.root.options = 1;
			
			// 菜单ID累加
			stateCB.menu.root.menuId ++;

			menuId =  stateCB.state ==	STATE_MENU_ROOT ? Config_menu[stateCB.menu.root.menuId]:Config_advMenu[stateCB.menu.root.menuId];
			
			// 判断是否需要蓝牙配置
			if ((MENU_ID_BLE_BROADCAST == menuId) && (CONFIG_SYSTEM_EXISTBLE))
			{
				stateCB.menu.root.menuId++;
			}
			else if(MENU_ID_BLE_BROADCAST == menuId)
			{
				// 开启蓝牙广播
				BLE_PROTOCOL_SendAdv(0);
			
			} 
			break;
			
 	}
   
	stateCB.menu.root.menuId %= (stateCB.state == STATE_MENU_ROOT ? CONFIG_MENU_MAX_ID:CONFIG_ADVMENU_MAX_ID);

}

 
// 快速调节
void STATE_MenuFastChange(uint32 param)
{	
	MENU_ID_E menuId;

	menuId =  stateCB.state ==	STATE_MENU_ROOT ? Config_menu[stateCB.menu.root.menuId]:Config_advMenu[stateCB.menu.root.menuId];

	/*****************************************************
	 ********************闪烁控制*************************
	 *****************************************************/
	// 蓝牙界面直接返回
	if (menuId == MENU_ID_BLE_BROADCAST)
	{
		return;
	}

	// 参数是否需要快速调节判断

	
	// 重置一分钟返回菜单
	TIMER_ResetTimer(TIMER_ID_MENU_QUIT);	

	// 改变闪烁周期
	TIMER_ChangeTime(TIMER_ID_LCD_BLINK,1000);

	// 关闭闪烁
	stateCB.menu.root.blinkAble =  FALSE;
	
	LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);

	// 一秒钟后恢复正常
	TIMER_AddTask(TIMER_ID_BlinkBack,
					1000,
					STATE_BlinkBack,
					1,
					1,
					ACTION_MODE_ADD_TO_QUEUE);

			
    /*****************************************************
	**********************参数调节************************
	******************************************************/				
	switch (menuId)
	{
	 
        //限速设置
        case  MENU_ID_SPEED_LIMIT:

			if (UNIT_METRIC == PARAM_GetUnit())
			{
				if ((BOOL)!param)	// --
				{
					if (stateCB.menu.root.speedLimit > CONFIG_SPEEDLIMIT_KMH_MIN)
					{
						stateCB.menu.root.speedLimit-=10;
					}
					
					if(stateCB.menu.root.speedLimit <= CONFIG_SPEEDLIMIT_KMH_MIN)
					{
						stateCB.menu.root.speedLimit = CONFIG_SPEEDLIMIT_KMH_MIN;
					}
				}
				else
				{
					if (stateCB.menu.root.speedLimit < CONFIG_SPEEDLIMIT_KMH_MAX)
					{
						stateCB.menu.root.speedLimit += 10;
					}
			
					if (stateCB.menu.root.speedLimit > CONFIG_SPEEDLIMIT_KMH_MAX)
					{
						stateCB.menu.root.speedLimit = CONFIG_SPEEDLIMIT_KMH_MAX;
					}
				
				}
			}
			else
			{
				if ((BOOL)!param)	// --
				{
					if (stateCB.menu.root.speedLimit > CONFIG_SPEEDLIMIT_MPH_MIN)
					{
						stateCB.menu.root.speedLimit-=10;
					}
					
					if(stateCB.menu.root.speedLimit < CONFIG_SPEEDLIMIT_MPH_MIN)
					{
						stateCB.menu.root.speedLimit = CONFIG_SPEEDLIMIT_MPH_MIN;						
					}
			
					
				}
				else
				{
				
					if (stateCB.menu.root.speedLimit < CONFIG_SPEEDLIMIT_MPH_MAX)
					{
						stateCB.menu.root.speedLimit += 10;
					}
			
					if (stateCB.menu.root.speedLimit > CONFIG_SPEEDLIMIT_MPH_MAX)
					{
						stateCB.menu.root.speedLimit = CONFIG_SPEEDLIMIT_MPH_MAX;
					}
					
				}
			}

			break;
			
			
		/*********************************************************
		 *********************************************************/
		//周长
		case MENU_ID_CY:
		
			//+
			if (param)
			{
				if (stateCB.menu.root.perimeter < (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f+CONFIG_PERIMETER_PNVALUE))
				{
					stateCB.menu.root.perimeter+=10;
				}
			
				if (stateCB.menu.root.perimeter > (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f+CONFIG_PERIMETER_PNVALUE))
				{
					stateCB.menu.root.perimeter = (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f+CONFIG_PERIMETER_PNVALUE);
				}
			}
			//-
			else
			{
				if (stateCB.menu.root.perimeter > (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f-CONFIG_PERIMETER_PNVALUE))
				{
					stateCB.menu.root.perimeter-=10;
				}
			
				if (stateCB.menu.root.perimeter < (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f-CONFIG_PERIMETER_PNVALUE))
				{
			
					stateCB.menu.root.perimeter = (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f-CONFIG_PERIMETER_PNVALUE);
				}
			}

			break;
			
		// 欠压
		case MENU_ID_BAT_U:
            //+
            if (param)
            {
				stateCB.menu.root.lowVoltage++;
				if( stateCB.menu.root.lowVoltage > stateCB.menu.root.voltage*10 - CONFIG_VOLT_UPPERLIMIT)
				{
					stateCB.menu.root.lowVoltage = stateCB.menu.root.voltage*10 - CONFIG_VOLT_UPPERLIMIT;
				}
			
			}
			else
			{
				stateCB.menu.root.lowVoltage--;
				if( stateCB.menu.root.lowVoltage < stateCB.menu.root.voltage*10 - CONFIG_VOLT_LOWERLIMIT)
				{
					stateCB.menu.root.lowVoltage = stateCB.menu.root.voltage*10 - CONFIG_VOLT_LOWERLIMIT;
				}
		
			}
			
			break;	

		// 限流	
		case MENU_ID_EU_A:
		
			//+
			if (param)
			{
				stateCB.menu.root.currentLimit++;
				if(stateCB.menu.root.currentLimit > CONFIG_CURLIMMIT_MAX)
				{
					stateCB.menu.root.currentLimit = CONFIG_CURLIMMIT_MAX;
				}
				
			}
			else
			{
				
				if(stateCB.menu.root.currentLimit < CONFIG_CURLIMMIT_MIN)
				{
					stateCB.menu.root.currentLimit = CONFIG_CURLIMMIT_MIN;
				}
				else
				{
					stateCB.menu.root.currentLimit--;
				}

			}
				
			break;	

		// 每档助力百分比
		case MENU_ID_LEVEL_PER:
			//+
			if(param)
			{
				stateCB.menu.root.pwmFixed[stateCB.menu.root.options]++;
				if( stateCB.menu.root.pwmFixed[stateCB.menu.root.options] >= CONFIG_ASSITPER_MAX)
				{
					stateCB.menu.root.pwmFixed[stateCB.menu.root.options] = CONFIG_ASSITPER_MAX;
				}
			}
			else
			{
				 
				if( stateCB.menu.root.pwmFixed[stateCB.menu.root.options] <= CONFIG_ASSITPER_MIN)
				{
					stateCB.menu.root.pwmFixed[stateCB.menu.root.options] = CONFIG_ASSITPER_MIN;
				}
				else
				{
					stateCB.menu.root.pwmFixed[stateCB.menu.root.options]--;

				}
			}

			break;

		/********************************************************
		 ***************************预留新增内容*****************
		 ********************************************************/
			
		default:			
			break;
			
	}
	
}

// 菜单库信息修改
void STATE_MenuChange(uint32 param)
{	

	MENU_ID_E menuId;

	menuId =  stateCB.state ==	STATE_MENU_ROOT ? Config_menu[stateCB.menu.root.menuId]:Config_advMenu[stateCB.menu.root.menuId];

	/*****************************************************
	 ********************闪烁控制*************************
	 *****************************************************/
	// 蓝牙界面直接返回
	if (menuId == MENU_ID_BLE_BROADCAST)
	{
		return;
	}
	
	// 重置一分钟返回菜单
	TIMER_ResetTimer(TIMER_ID_MENU_QUIT);	

	// 改变闪烁周期
	TIMER_ChangeTime(TIMER_ID_LCD_BLINK,1000);

	// 关闭闪烁
	stateCB.menu.root.blinkAble =  FALSE;
	
	LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);

	// 一秒钟后恢复正常
	TIMER_AddTask(TIMER_ID_BlinkBack,
					1000,
					STATE_BlinkBack,
					1,
					1,
					ACTION_MODE_ADD_TO_QUEUE);

			
    /*****************************************************
	**********************参数调节************************
	******************************************************/
	switch (menuId)
	{

		// 清除里程
		case MENU_ID_CLEARTRIP:
		
			if ((!stateCB.menu.root.clearState) &&((BOOL)param))
			{
			
               stateCB.menu.root.clearState = TRUE;
                
                // 清除里程
				PARAM_ClearTrip();
				
               	// 清除里程返回
				TIMER_AddTask(TIMER_ID_CLEARTRIP_BACK,
								1000,
								STATE_ClearTripAndResetBack,
								1,
								1,
								ACTION_MODE_ADD_TO_QUEUE);	
			}
	
			break;	

	
		// 速度单位
		case MENU_ID_UNIT:
		
			if(UNIT_INCH == stateCB.menu.root.unit)
			{
				stateCB.menu.root.unit = UNIT_METRIC;
			}
			else
			{
				stateCB.menu.root.unit = UNIT_INCH;
			}
			
			// 限速
			stateCB.menu.root.speedLimit = UNIT_METRIC == stateCB.menu.root.unit ? paramCB.nvm.param.common.speed.limitVal:limitKmhToMph[(paramCB.nvm.param.common.speed.limitVal+5)/10]*10;

			break;

		// 限速
		case MENU_ID_SPEED_LIMIT:

			if (UNIT_METRIC == PARAM_GetUnit())
			{
				if ((BOOL)!param)	// --
				{
					if (stateCB.menu.root.speedLimit > CONFIG_SPEEDLIMIT_KMH_MIN)
					{
						stateCB.menu.root.speedLimit-=10;
					}
					
					if(stateCB.menu.root.speedLimit <= CONFIG_SPEEDLIMIT_KMH_MIN)
					{
						stateCB.menu.root.speedLimit = CONFIG_SPEEDLIMIT_KMH_MIN;
					}
				}
				else
				{
					if (stateCB.menu.root.speedLimit < CONFIG_SPEEDLIMIT_KMH_MAX)
					{
						stateCB.menu.root.speedLimit += 10;
					}
			
					if (stateCB.menu.root.speedLimit > CONFIG_SPEEDLIMIT_KMH_MAX)
					{
						stateCB.menu.root.speedLimit = CONFIG_SPEEDLIMIT_KMH_MAX;
					}	
				}
			}
			else
			{
				if ((BOOL)!param)	// --
				{
					if (stateCB.menu.root.speedLimit > CONFIG_SPEEDLIMIT_MPH_MIN)
					{
						stateCB.menu.root.speedLimit-=10;
					}
					
					if(stateCB.menu.root.speedLimit < CONFIG_SPEEDLIMIT_MPH_MIN)
					{
						stateCB.menu.root.speedLimit = CONFIG_SPEEDLIMIT_MPH_MIN;						
					}

			
					
				}
				else
				{
					if (stateCB.menu.root.speedLimit < CONFIG_SPEEDLIMIT_MPH_MAX)
					{
						stateCB.menu.root.speedLimit += 10;
					}
			
					if (stateCB.menu.root.speedLimit > CONFIG_SPEEDLIMIT_MPH_MAX)
					{
						stateCB.menu.root.speedLimit = CONFIG_SPEEDLIMIT_MPH_MAX;
					}

				}
			}
			
			break;

		// 关机时间
		case MENU_ID_AUTO_POWER_OFF_TIME:
		
			if ((BOOL)param)
			{
			   if(stateCB.menu.root.powerTimeIndex >= CONFIG_POWER_OFFTIME_MAX-1)
			   {
					stateCB.menu.root.powerTimeIndex = CONFIG_POWER_OFFTIME_MAX-1;
			   }
			   else
			   {
					stateCB.menu.root.powerTimeIndex++;
			   }
			    
			}
			else
			{
			   if(stateCB.menu.root.powerTimeIndex <= 0)
			   {
					stateCB.menu.root.powerTimeIndex = 0;
			   }
			   else
			   {
					stateCB.menu.root.powerTimeIndex--;
			   }
			}

			stateCB.menu.root.powerTime = Config_PowerOffTime[stateCB.menu.root.powerTimeIndex];
			
			break;
			
		// 蓝牙广播
		case MENU_ID_BLE_BROADCAST:
			break;

		// 背光
		case MENU_ID_BACKLIGHT_BRIGHTNESS:
		
			if ((BOOL)param)
			{
				
				if(stateCB.menu.root.lightLevelIndex >= CONFIG_LIGHTLEVEL_MAX-1)
			   {
					stateCB.menu.root.lightLevelIndex = CONFIG_LIGHTLEVEL_MAX-1;
			   }
			   else
			   {
					stateCB.menu.root.lightLevelIndex++;
			   }
			}
			else
			{
				if(stateCB.menu.root.lightLevelIndex <= 0)
			   {
					stateCB.menu.root.lightLevelIndex = 0;
			   }
			   else
			   {
					stateCB.menu.root.lightLevelIndex--;
			   }
			}
			
			stateCB.menu.root.lightLevel= Config_lightLevel[stateCB.menu.root.lightLevelIndex];

			BACK_LIGHT_Start((BACK_LIGHT_LEVEL_E)stateCB.menu.root.lightLevel);
		
			break;

		/*********************************************************
		 *********************************************************/
		//轮径
		case MENU_ID_SIZE:
		
			//+
			if (param)
			{
				stateCB.menu.root.wheelSizeIdIndex ++;
				stateCB.menu.root.wheelSizeIdIndex %= CONFIG_WHELLID_MAX;
			}
			//-
			else
			{
				stateCB.menu.root.wheelSizeIdIndex += CONFIG_WHELLID_MAX-1;
				stateCB.menu.root.wheelSizeIdIndex %= CONFIG_WHELLID_MAX;
			}

			// 赋值
			stateCB.menu.root.wheelSizeId = Config_wheelIdSize[stateCB.menu.root.wheelSizeIdIndex][0];
			stateCB.menu.root.wheelSize   = Config_wheelIdSize[stateCB.menu.root.wheelSizeIdIndex][1];

			if(CONFIG_DEFAULT_WHELL == stateCB.menu.root.wheelSize)
			{
				// 存在默认轮径
				stateCB.menu.root.perimeter = CONFIG_DEFAULT_PERIMETER;
			}
			else
			{	
				// 重新计算周长
				stateCB.menu.root.perimeter = (uint32)((float)stateCB.menu.root.wheelSize *0.1f*25.4f*3.14f);
			}

			break;

		//周长
		case MENU_ID_CY:
		
			//+
			if (param)
			{
				if (stateCB.menu.root.perimeter < (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f+CONFIG_PERIMETER_PNVALUE))
				{
					stateCB.menu.root.perimeter++;
				}
			
				if (stateCB.menu.root.perimeter > (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f+CONFIG_PERIMETER_PNVALUE))
				{
					stateCB.menu.root.perimeter = (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f+CONFIG_PERIMETER_PNVALUE);
				}
			}
			//-
			else
			{
				if (stateCB.menu.root.perimeter > (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f-CONFIG_PERIMETER_PNVALUE))
				{
					stateCB.menu.root.perimeter--;
				}
			
				if (stateCB.menu.root.perimeter < (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f-CONFIG_PERIMETER_PNVALUE))
				{
			
					stateCB.menu.root.perimeter = (PARAM_GetNewWheelSizeInch()*0.1f*25.4f*3.14f-CONFIG_PERIMETER_PNVALUE);
				}
			}

			break;
			
		// 电压
		case MENU_ID_BAT:
            //+
            if (param)
            {
				stateCB.menu.root.voltageIndex++;
				stateCB.menu.root.voltageIndex %= CONFIG_VOLTLEVET_X_MAX;  				
            }
            else
           	{
				stateCB.menu.root.voltageIndex++;
				stateCB.menu.root.voltageIndex %= CONFIG_VOLTLEVET_X_MAX;  	
           	}

			stateCB.menu.root.voltage = Config_VoltageLevel[stateCB.menu.root.voltageIndex][0];

			if(CONFIG_VOLTLEVET_PNVALUE == stateCB.menu.root.voltage)
			{
			   // 存在默认电压
			   stateCB.menu.root.lowVoltage =  CONFIG_LOWVOLTLEVET_PNVALUE; 
			}
			else
			{
				stateCB.menu.root.lowVoltage = Config_VoltageLevel[stateCB.menu.root.voltageIndex][1];
			}
				
			break;

		// 欠压
		case MENU_ID_BAT_U:
            //+
            if (param)
            {
				stateCB.menu.root.lowVoltage++;
				if( stateCB.menu.root.lowVoltage > stateCB.menu.root.voltage*10 - CONFIG_VOLT_UPPERLIMIT)
				{
					stateCB.menu.root.lowVoltage = stateCB.menu.root.voltage*10 - CONFIG_VOLT_UPPERLIMIT;
				}
			
			}
			else
			{
				stateCB.menu.root.lowVoltage--;
				if( stateCB.menu.root.lowVoltage <=stateCB.menu.root.voltage*10 - CONFIG_VOLT_LOWERLIMIT)
				{
					stateCB.menu.root.lowVoltage = stateCB.menu.root.voltage*10 - CONFIG_VOLT_LOWERLIMIT;
				}
			 
			}	
			break;	
			
		// 限流	
		case MENU_ID_EU_A:
		
			//+
			if (param)
			{
				stateCB.menu.root.currentLimit++;
				if(stateCB.menu.root.currentLimit > CONFIG_CURLIMMIT_MAX)
				{
					stateCB.menu.root.currentLimit = CONFIG_CURLIMMIT_MAX;
				}
				
			}
			else
			{
				
				if(stateCB.menu.root.currentLimit < CONFIG_CURLIMMIT_MIN)
				{
					stateCB.menu.root.currentLimit = CONFIG_CURLIMMIT_MIN;
				}
				else
				{
					stateCB.menu.root.currentLimit--;
				}

			}
				
			break;	
			
		// 最大档位	
		case MENU_ID_RANGE:
			//+
			if (param)
			{
				stateCB.menu.root.maxAssitIndex++;
				stateCB.menu.root.maxAssitIndex %= CONFIG_MAXASSIT_NUM; 	
			}
			else
			{
				stateCB.menu.root.maxAssitIndex+= CONFIG_MAXASSIT_NUM-1;
				stateCB.menu.root.maxAssitIndex %= CONFIG_MAXASSIT_NUM;

			}
			
			// 最大档位	
			stateCB.menu.root.maxAssit = Config_MaxAssit[stateCB.menu.root.maxAssitIndex];
			
			//每档PWM
			for(uint8 i =0; i<= stateCB.menu.root.maxAssit;i++)
			{	
				stateCB.menu.root.pwmFixed[i] = paramCB.nvm.param.protocol.pwmFixed[i];
			}

			break;
			
		// 每档助力百分比
		case MENU_ID_LEVEL_PER:
			//+
			if(param)
			{
				stateCB.menu.root.pwmFixed[stateCB.menu.root.options]++;
				if( stateCB.menu.root.pwmFixed[stateCB.menu.root.options] >= CONFIG_ASSITPER_MAX)
				{
					stateCB.menu.root.pwmFixed[stateCB.menu.root.options] = CONFIG_ASSITPER_MAX;
				}
			}
			else
			{
				 
				if( stateCB.menu.root.pwmFixed[stateCB.menu.root.options] <= CONFIG_ASSITPER_MIN)
				{
					stateCB.menu.root.pwmFixed[stateCB.menu.root.options] = CONFIG_ASSITPER_MIN;
				}
				else
				{
					stateCB.menu.root.pwmFixed[stateCB.menu.root.options]--;

				}
			}

			break;
			
		// 测试磁钢	
		case MENU_ID_STEEL:
			//+
			if(param)
			{
				stateCB.menu.root.speedSensor++;
				if(CONFIG_SPPEEDSENSOR_MAX <= stateCB.menu.root.speedSensor)
				{
					stateCB.menu.root.speedSensor = CONFIG_SPPEEDSENSOR_MAX ;
				}
			}
			else
			{
				
				if(CONFIG_SPPEEDSENSOR_MIN >= stateCB.menu.root.speedSensor)
				{
					stateCB.menu.root.speedSensor = CONFIG_SPPEEDSENSOR_MAX;
				}
				else
				{
					stateCB.menu.root.speedSensor--;
				}
			}

			break;

		// 定速巡航
		case MENU_ID_CRUIS:

			if(param)
			{
				stateCB.menu.root.cruiseSwitch = TRUE;
			}
			else
			{
				stateCB.menu.root.cruiseSwitch = FALSE;
	
			}
			
			break;
			
		// 恢复出厂
		case MENU_ID_RESET:

			if ((!stateCB.menu.root.reset) &&((BOOL)param))
			{
			
               stateCB.menu.root.reset = TRUE;
                
                // 恢复出厂
				PARAM_FactoryReset();
               	// 恢复出厂
				TIMER_AddTask(TIMER_ID_CLEARTRIP_BACK,
								1000,
								STATE_ClearTripAndResetBack,
								1,
								1,
								ACTION_MODE_ADD_TO_QUEUE);	
			}

			break;

		/*********************************************************
		***************************预留新增内容*******************
		*********************************************************/
				
		default:
			break;
	}
}


/*****************************************************************************
******************************************************************************
******************************高级菜单密码************************************
******************************************************************************
******************************************************************************/
void STATE_menuPasswordNumChange(uint32 param)
{

	/*****************************************************
	 ********************闪烁控制*************************
	 *****************************************************/
	stateCB.menu.root.blinkAble =  FALSE;
	
	TIMER_ChangeTime(TIMER_ID_LCD_BLINK,1000);
	
	LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);

	TIMER_AddTask(TIMER_ID_BlinkBack,
					1000,
					STATE_BlinkBack,
					1,
					1,
					ACTION_MODE_ADD_TO_QUEUE);	
	/*****************************************************
	 *****************************************************
	 *****************************************************/
	switch (stateCB.menu.passWord.options)
	{
		case 0:

			if((BOOL)param)
			{
				stateCB.menu.passWord.numberOne++;
				stateCB.menu.passWord.numberOne %=10;

			}
			else
			{
				stateCB.menu.passWord.numberOne+=10-1;
				stateCB.menu.passWord.numberOne %=10;

			}
			break;
			
		case 1:
		
			if((BOOL)param)
			{
				stateCB.menu.passWord.numberTwo++;
				stateCB.menu.passWord.numberTwo %=10;

			}
			else
			{
				stateCB.menu.passWord.numberTwo+=10-1;
				stateCB.menu.passWord.numberTwo %=10;

			}

			break;
			
		case 2:
		
			if((BOOL)param)
			{
				stateCB.menu.passWord.numberThree++;
				stateCB.menu.passWord.numberThree %=10;

			}
			else
			{
				stateCB.menu.passWord.numberThree+=10-1;
				stateCB.menu.passWord.numberThree %=10;

			}


			break;
			
		case 3:
		
			if((BOOL)param)
			{
				stateCB.menu.passWord.numberFour++;
				stateCB.menu.passWord.numberFour %=10;

			}
			else
			{
				stateCB.menu.passWord.numberFour+=10-1;
				stateCB.menu.passWord.numberFour %=10;

			}

			break;
			
		default:
			break;
	}

	stateCB.menu.passWord.numberParam = stateCB.menu.passWord.numberOne*1000+stateCB.menu.passWord.numberTwo*100+
						       			stateCB.menu.passWord.numberThree*10+stateCB.menu.passWord.numberFour;
	
}

void STATE_menuPasswordNumEnter(uint32 param)
{

	if(stateCB.menu.passWord.powerOnFrist)
	{
		stateCB.menu.passWord.powerOnFrist = FALSE;
		return;
	}
	
	stateCB.menu.passWord.options++;

	if (stateCB.menu.passWord.options > 3)
	{

		//比对高级菜单密码
		if (stateCB.menu.passWord.numberParam == PARAM_GetMenuPassword())
		{
			STATE_EnterState(STATE_ADV_MENU_ROOT);
		}

		stateCB.menu.passWord.numberOne = 0;
		stateCB.menu.passWord.numberTwo = 0;
		stateCB.menu.passWord.numberThree = 0;
		stateCB.menu.passWord.numberFour = 0;	
		stateCB.menu.passWord.numberParam = 0;
		stateCB.menu.passWord.options=0;

	}
	
}




void STATE_FunctionTest(uint32 param)
{
	LCD_UnitTest();
}

// 开机逻辑
void STATE_PowerOnKeyUpFlag(uint32 param)
{
	stateCB.powerOnKeyUpFlag = param;
}

void STATE_PowerOnKeyLongHold(uint32 param)
{
	stateCB.powerOnKeyLongHoldTime++;
}

			
// 每个状态的入口处理
void STATE_EnterState(uint32 state)
{	
	uint16 buff[32] = {0};
	uint16 Allbuff[32] = {0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
						0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F};

	// 让当前的状态成为历史
	stateCB.preState = stateCB.state;

	// 设置新的状态
	stateCB.state = (STATE_E)state;

	// 开机或者关机状态下屏蔽组合键
	if ((state == STATE_SHOW_POWER_ON) ||
		(state == STATE_NULL) ||
		(STATE_POWER_KEY_KEYUP == state)|| 
		(state == STATE_POWER_OFF))
	{
		KEYMSG_SetGroupKeyRecoveryRequest(TRUE);
	}
	else
	{
		KEYMSG_SetGroupKeyRecoveryRequest(FALSE);
	}

	// 各状态的入口设定
	switch (state)
	{
		// ■■ 空状态 ■■
		// ■■ 长按电源键和减键开灯 ■■
		case STATE_SERVICE_LIGHT_ON_OFF:
			// 注销所有按键服务
			KEYMSG_StopAllService();
			// 注册长按电源键和减键开灯服务
			KEYMSG_RegisterMsgService(KEY_NAME_ADD_LIGHT, KEY_MSG_HOLD, STATE_Service_LightOnOff, 0);
			break;
		case STATE_NULL:
		
			// 注销所有按键服务
			KEYMSG_StopAllService();
			
			// 电源键长按开机，进入状态机入口状态
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_HOLD, STATE_EnterState, STATE_ENTRY);
			break;

		// ■■ 入口状态的入口处理 ■■
		case STATE_ENTRY:
			
			POWER_ON();
			break;
	
		// ■■ 参数加载状态的入口处理 ■■
		case STATE_LOAD_NVM:
			break;

		// ■■ 开机界面状态的入口处理 ■■
		case STATE_SHOW_POWER_ON:

			// LCD初始化
			SEGMENT_Init();
			
			if (stateCB.state != stateCB.preState)
			{
				LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);
				LCD_SetGroupOnOff(SEG_MASK_ALL, TRUE);

				// 全显示
				LCD_DIS_AllState(Allbuff);
			} 

			// 开机按键检测
			stateCB.powerOnKeyUpFlag = FALSE;
			stateCB.powerOnKeyLongHoldTime = 0;
			// 背光切换回最亮模式
			BACK_LIGHT_Start(BACK_LIGHT_LEVEL_ON);
			 

			// 延时1S后进入开机数字显示状态
			TIMER_AddTask(TIMER_ID_STATE_CONTROL,
							1000,
							STATE_EnterState,
							STATE_BOOT_DIGIT,
							1,
							ACTION_MODE_ADD_TO_QUEUE);
			 
			// ■■配置按键消息服务■■
			// 注销所有按键服务
			KEYMSG_StopAllService();
			
			// 电源键释放，设置电源键释放标识
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_UP, STATE_PowerOnKeyUpFlag, TRUE);
			
			// 电源键重复抛出，每触发一次，【电源键按住并保持计数值】+1，下文中根据此计数值判断是否进入高级菜单界面
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_REPEAT, STATE_PowerOnKeyLongHold, TRUE);

			break;
		case STATE_BOOT_DIGIT:
			bootDigitCount = 1;
			LCD_ShowBootDigits(bootDigitCount);
			TIMER_AddTask(TIMER_ID_BOOT_DIGIT,
						  700,
						  STATE_BootDigitTick,
						  0,
						  TIMER_LOOP_FOREVER,
						  ACTION_MODE_ADD_TO_QUEUE);
			break;
		// ■■ 骑行正常界面状态的入口处理 ■■
		case STATE_RIDING:	
			
			if (stateCB.state != stateCB.preState)
			{	
		
				/********************************************************
				**********************数据初始化*************************
				********************************************************/				
				// 关闭全部使能
				LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);
				LCD_SetGroupOnOff(SEG_MASK_ALL, FALSE);
				// 全关掉
				LCD_DIS_AllState(buff);

				// 打开电池标识使能和电量使能
				LCD_SetGroupOnOff((SEG_MASK_E)(SEG_MASK_BATTERY_ICON |
													SEG_MASK_BATTERY_QUANTITY|
													SEG_MASK_POWER |
													SEG_MASK_BLE |
													SEG_MASK_LIGHT |
													SEG_MASK_CRUISE |
													SEG_MASK_ERROR |
													SEG_MASK_ASSIST_UP |
													SEG_MASK_KMH |
													SEG_MASK_MPH |
													SEG_MASK_ASSIST_LEVEL |
													SEG_MASK_BAT_PERCENT |
													SEG_MASK_TRIP |
													SEG_MASK_ODO |
													SEG_MASK_SPEED_AVG |
													SEG_MASK_SPEED_MAX |
 													SEG_MASK_SPEED_NUM |
													SEG_MASK_SMALL_NUM1 |
													SEG_MASK_SMALL_NUM2 |
													SEG_MASK_SMALL_NUM3 |
													SEG_MASK_SMALL_NUM4 |
													SEG_MASK_SMALL_NUM5 |
													SEG_MASK_SMALL_NUM6 |
													SEG_MASK_TIME |
													SEG_MASK_KM |
													SEG_MASK_MILE),
													TRUE);


		
				
				// 大灯开启后，屏幕亮度变为最暗
				if (PARAM_GetLightSwitch())
				{
					BACK_LIGHT_Start(BACK_LIGHT_LEVEL_20_100);
				}
				// 大灯关闭后，恢复之前的背光等级
				else
				{
					BACK_LIGHT_Start((BACK_LIGHT_LEVEL_E)PARAM_GetBrightness());
				}

				// 每次进菜单都从第一个开始
				stateCB.dataMode = RID_DATA_MODE_TRIP;

				// kill一分钟返回骑行界面
				TIMER_KillTask(TIMER_ID_MENU_QUIT);

				/********************************************************
				********************************************************/

				// 显示正常骑行界面 
				LCD_ShowRidingUI(TRUE);
			}
			

			/********************************************************
			**********************按键操作***************************
			********************************************************/

			// 注销所有按键服务
			KEYMSG_StopAllService();
			
			// 长按电源键和减键开灯
			KEYMSG_RegisterMsgService(KEY_NAME_ADD_LIGHT, KEY_MSG_HOLD, STATE_Service_LightOnOff, 0);
			// 加键加档位
			KEYMSG_RegisterMsgService(KEY_NAME_ADD, KEY_MSG_UP, STATE_Service_AssistLevelChange, TRUE);

			// 减键减档位
			KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_UP, STATE_Service_AssistLevelChange, FALSE);

			// 长按+键开关大灯
			KEYMSG_RegisterMsgService(KEY_NAME_ADD, KEY_MSG_HOLD, STATE_Service_LightOnOff, 0);
			
			// 长按减键推车助力
			KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_HOLD, STATE_Service_AssistPushOnOff, TRUE);

			// 电源键单击切换骑行界面6种显示模式
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_UP, STATE_Service_ChangeDataDisplayMode, 0);
			
 			// 速度为0时，+ - 长按1S进去菜单界面
			KEYMSG_RegisterMsgService(KEY_NAME_ADD_DEC, KEY_MSG_HOLD, STATE_Service_EnterMenu, PARAM_GetSpeed());
			
			// 电源键长按关机
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_HOLD, STATE_EnterState, STATE_SHOW_POWER_OFF);

			break;

		// 菜单页面
		case STATE_MENU_ROOT:
		
			if (stateCB.state != stateCB.preState)
			{
				
				/********************************************************
				**********************数据初始化*************************
				********************************************************/
				// 关闭全部使能
				LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);
				LCD_SetGroupOnOff(SEG_MASK_ALL, FALSE);
				LCD_SetGroupBlink(SEG_MASK_BLE, FALSE);
				
				// 全关掉
				LCD_DIS_AllState(buff);

				// 打开显示使能
				LCD_SetGroupOnOff((SEG_MASK_E)(	SEG_MASK_BLE |
											   	SEG_MASK_KMH |
												SEG_MASK_MPH |
												SEG_MASK_SPEED_NUM |
												SEG_MASK_SMALL_NUM1 |
												SEG_MASK_SMALL_NUM2 |
												SEG_MASK_SMALL_NUM3 |
												SEG_MASK_SMALL_NUM4 |
												SEG_MASK_SMALL_NUM5 |
												SEG_MASK_SMALL_NUM6),
												TRUE);


				// 大灯开启后，屏幕亮度变为最暗
				if (PARAM_GetLightSwitch())
				{
					BACK_LIGHT_Start(BACK_LIGHT_LEVEL_20_100);
				}
				// 大灯关闭后，恢复之前的背光等级
				else
				{
					BACK_LIGHT_Start((BACK_LIGHT_LEVEL_E)PARAM_GetBrightness());
				}
				
				// 注册一分钟返回骑行界面
				TIMER_AddTask(TIMER_ID_MENU_QUIT,
								TIME_WAIT_MENU_AUTO_RETURN_RIDING,
								STATE_EnterState,
								STATE_RIDING,
								1,
								ACTION_MODE_ADD_TO_QUEUE);

				// 参数初始化
				STATE_MenuParam(TRUE); 
				
				/********************************************************
				********************************************************/
												
				// 显示正常菜单界面 
				LCD_DisplayMenuRoot(Config_menu[stateCB.menu.root.menuId]);
	
			}


			/********************************************************
			**********************按键操作***************************
			********************************************************/
			
 			// 注销所有按键服务
			KEYMSG_StopAllService();

			// ADD键按下消息
			KEYMSG_RegisterMsgService(KEY_NAME_ADD, KEY_MSG_UP, STATE_MenuChange, TRUE);
			KEYMSG_RegisterMsgService(KEY_NAME_ADD, KEY_MSG_REPEAT, STATE_MenuFastChange, TRUE);

			// DEC键按下消息
			KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_UP, STATE_MenuChange, FALSE);
            KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_REPEAT, STATE_MenuFastChange, FALSE);

			// 电源键单击进入要选择的菜单
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_UP, STATE_MenuChoose,TRUE);

			// + -键长按1s释放消息，进入高级菜单界面
      		KEYMSG_RegisterMsgService(KEY_NAME_ADD_DEC, KEY_MSG_HOLD, STATE_EnterState, STATE_RIDING);
            
			// 电源键长按，关机
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_HOLD, STATE_EnterState, STATE_SHOW_POWER_OFF);
			
			break;
			

		// 高级菜单密码界面
		case STATE_ADVMENU_PASSWORD:
		
			if (stateCB.state != stateCB.preState)
			{
			
				/********************************************************
				**********************数据初始化*************************
				********************************************************/
				// 全关掉
				LCD_DIS_AllState(buff);
				
				// 关闭全部使能
				LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);
				LCD_SetGroupOnOff(SEG_MASK_ALL, FALSE);

				// 只显示速度和密码
				LCD_SetGroupOnOff(	SEG_MASK_SPEED_NUM|
									SEG_MASK_SMALL_NUM1|
									SEG_MASK_SMALL_NUM2|
									SEG_MASK_SMALL_NUM3|
									SEG_MASK_SMALL_NUM4|
									SEG_MASK_SMALL_NUM5|
									SEG_MASK_SMALL_NUM6, TRUE);

				stateCB.menu.passWord.optionsOkFlag =0;
				stateCB.menu.passWord.options = 0;
				stateCB.menu.passWord.numberOne = 0;
				stateCB.menu.passWord.numberTwo = 0;
				stateCB.menu.passWord.numberThree = 0;
				stateCB.menu.passWord.numberFour = 0;

				stateCB.menu.passWord.powerOnFrist = TRUE;

				// 闪烁初始化
				stateCB.menu.root.blinkAble =  TRUE;
		
				// 显示开机密码界面
				LCD_DisplaymenuPassword(TRUE);

			}
			
			// 注销所有按键服务
			KEYMSG_StopAllService();

			KEYMSG_RegisterMsgService(KEY_NAME_ADD, KEY_MSG_UP, STATE_menuPasswordNumChange, TRUE);

			KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_UP, STATE_menuPasswordNumChange, FALSE);

            
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_UP, STATE_menuPasswordNumEnter,TRUE);

			// 长按关机
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_HOLD, STATE_EnterState, STATE_SHOW_POWER_OFF);
			
			break;

		// 高级菜单界面
		case STATE_ADV_MENU_ROOT:
		
			if (stateCB.state != stateCB.preState)
			{	

				/********************************************************
				**********************数据初始化*************************
				********************************************************/			
				// 全关掉
				LCD_DIS_AllState(buff);
				
				// 关闭全部使能
				LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);
				LCD_SetGroupOnOff(SEG_MASK_ALL, FALSE);
		
				LCD_SetGroupOnOff( SEG_MASK_KMH|
								   SEG_MASK_MPH|
								   SEG_BIT_SPEED_NUM|
								   SEG_MASK_ASSIST_LEVEL|
								   SEG_MASK_SMALL_NUM1|
								   SEG_MASK_SMALL_NUM2|
								   SEG_MASK_SMALL_NUM3|
								   SEG_MASK_SMALL_NUM4|
								   SEG_MASK_SMALL_NUM5|
								   SEG_MASK_SMALL_NUM6, TRUE);
				
		
				// 参数初始化
				STATE_MenuParam(TRUE); 
			
				// 显示高级菜单界面 
				LCD_DisplayMenuRoot(Config_advMenu[stateCB.menu.root.menuId]);

			}

			// 注销所有按键服务
			KEYMSG_StopAllService();
				
			// ADD键按下消息
			KEYMSG_RegisterMsgService(KEY_NAME_ADD, KEY_MSG_UP, STATE_MenuChange, TRUE);
			KEYMSG_RegisterMsgService(KEY_NAME_ADD, KEY_MSG_REPEAT, STATE_MenuFastChange, TRUE);
			
			// DEC键按下消息
			KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_UP, STATE_MenuChange, FALSE);
			KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_REPEAT, STATE_MenuFastChange, FALSE);


			// 电源键按下消息确认
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_UP, STATE_MenuChoose,TRUE);
			
			// 电源键长按关机
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_HOLD, STATE_EnterState, STATE_SHOW_POWER_OFF);
			
			break;

		// 进入关机状态
		case STATE_SHOW_POWER_OFF:	

		    if (stateCB.state != stateCB.preState)
			{
				LCD_SetGroupOnOff(SEG_MASK_ALL, TRUE);
				
				// 全关掉
				LCD_DIS_AllState(buff);
			}

			// 跳转到下电状态
			TIMER_AddTask(TIMER_ID_STATE_CONTROL,
							TIME_WAIT_POWER_ON_OFF,
							STATE_EnterState,
							STATE_POWER_OFF,
							1,
							ACTION_MODE_ADD_TO_QUEUE);
					
			// 关机需要保存参数
			NVM_Save(TRUE);

			// 关闭档位
			PARAM_SetAssistLevel((ASSIST_ID_E)ASSIST_0);
			PARAM_EnablePushAssist(FALSE);

			// 重置速度相关变量
			PARAM_SetSpeed(0);	

			// 关闭背光
			BACK_LIGHT_Start(BACK_LIGHT_LEVEL_OFF);	

			// 关闭所有大灯
			PARAM_SetLightSwitch(FALSE);

			// kill掉必要任务
			TIMER_KillTask(TIMER_ID_MENU_QUIT);
			TIMER_KillTask(TIMER_ID_AUTO_POWER_OFF);
			
			// 注销所有按键服务
			KEYMSG_StopAllService();

			break;
			
		// 关机下电状态
		case STATE_POWER_OFF:

			// ■■配置按键消息服务■■
			// 注销所有按键服务
			KEYMSG_StopAllService();

			// 下电
			POWER_OFF();
			
			// 关掉背光
			BACK_LIGHT_Start(BACK_LIGHT_LEVEL_OFF);

			// 电源键长按开机，进入状态机入口状态
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_HOLD, STATE_EnterState, STATE_ENTRY);
			
			break;
			
		// ■■ 老化测试状态的入口处理 ■■ 
		case STATE_AGING_TEST:
			
			// 数码管初始化
			SEGMENT_Init();
		
			LCD_SetGroupBlink(SEG_MASK_ALL, FALSE);
			LCD_SetGroupOnOff(SEG_MASK_ALL, TRUE);
			
			// 标志大于200,则标志不合法，清零计数标志
			if (paramCB.runtime.testflag[1] > 200)
			{
				PARAM_SetRxCount(0);

				// 标志不合法说明老化次数要复位
				paramCB.runtime.testflag[1] = 0;				
				IAP_FlashWriteWordArrayWithErase(PARAM_MCU_TEST_FLAG_ADDEESS + 4, (uint32 *)&(paramCB.runtime.testflag[1]), 1);
			}
			else
			{
				PARAM_SetRxCount(paramCB.runtime.testflag[1]*10000);
			}
			
			// 背光切换回最亮模式
			BACK_LIGHT_Start(BACK_LIGHT_LEVEL_ON);
			
			// 数码管点亮循环测试
			TIMER_AddTask(TIMER_ID_FUNCTION_TEST,
							100,
							STATE_FunctionTest,
							1,
							TIMER_LOOP_FOREVER,
							ACTION_MODE_ADD_TO_QUEUE);

			// 老化命令
			TIMER_AddTask(TIMER_ID_AGE_SEND,
							100,
							UART_PROTOCOL_SendCmdAging,
							1,
							TIMER_LOOP_FOREVER,
							ACTION_MODE_ADD_TO_QUEUE);
			break;

		// ■■ 功能测试状态的入口处理 ■■ 
		case STATE_FUCTION_TEST:
			
			// 先关闭所有显示函数
			LCD_SetGroupOnOff(SEG_MASK_ALL, TRUE);
			break;

		default:
			break;
	}
}

// 每个状态下的过程处理
void STATE_Process(void)
{
	switch (stateCB.state)
	{
		// ■■ 空状态 ■■
		case STATE_NULL:
			break;

		// ■■ 入口状态过程处理 ■■
		case STATE_ENTRY:
			if (0X55AA55BB == paramCB.runtime.testflag[0])
			{
				// 老化清除要保证不断电
				POWER_ON();
					
				// 清除老化进入标志
				paramCB.runtime.testflag[0] = 0;				
				IAP_FlashWriteWordArrayWithErase(PARAM_MCU_TEST_FLAG_ADDEESS, (uint32 *)paramCB.runtime.testflag, 1);
					
				STATE_EnterState(STATE_AGING_TEST);
			}
			else
			{
				STATE_EnterState(STATE_LOAD_NVM);
			}
			break;

		// ■■ 参数加载状态过程处理 ■■
		case STATE_LOAD_NVM:
			STATE_EnterState(STATE_SHOW_POWER_ON);
			break;

		// ■■ 上电状态的过程处理 ■■
		case STATE_SHOW_POWER_ON:
			break;

		// 开机长按状态
		case STATE_POWER_KEY_KEYUP:

			// 在5s内电源键抬起则进入骑行界面
			//if (stateCB.powerOnKeyUpFlag)
			//{
				stateCB.powerOnKeyUpFlag = FALSE;

				STATE_EnterState(STATE_RIDING);
				
			//}
			
			// 高级菜单开启和屏蔽
			#if CONFIG_ADVMENU_ENABLE 

				// 达到长按门限>=5s后则进入高级菜单界面
				if (stateCB.powerOnKeyLongHoldTime > 17)      //5s  17
				 {
					 stateCB.powerOnKeyLongHoldTime = 0;

				 	if ((PARAM_GetMenuPasswordSwitch()))
					{
						STATE_EnterState(STATE_ADVMENU_PASSWORD);	
					}
					else
					{
						STATE_EnterState(STATE_ADV_MENU_ROOT);
					}
				 }
				 
			 #endif
			 
			 break;
	
		// ■■ 骑行正常界面状态的过程处理 ■■
		case STATE_RIDING:
	
			// 显示正常骑行界面 
			LCD_ShowRidingUI(FALSE);
				
			break;

		case STATE_MENU_ROOT:
		
			// 显示正常菜单界面 	 
			LCD_DisplayMenuRoot(Config_menu[stateCB.menu.root.menuId]);
			
			break;

		// 高级菜单密码界面
		case STATE_ADVMENU_PASSWORD:

			LCD_DisplaymenuPassword(FALSE);	
			
			break;	

			
		// 高级菜单界面
		case STATE_ADV_MENU_ROOT:
		
			 // 显示正常菜单界面 
			LCD_DisplayMenuRoot(Config_advMenu[stateCB.menu.root.menuId]);

			 break;
				
		// 老化测试
		case STATE_AGING_TEST:
			if (PARAM_GetRxCount() % 10000 < 10)
			{
				if ((PARAM_GetRxCount() / 10000) > paramCB.runtime.testflag[1])
				{
					paramCB.runtime.testflag[1] = PARAM_GetRxCount() / 10000;
					IAP_FlashWriteWordArrayWithErase(PARAM_MCU_TEST_FLAG_ADDEESS + 4, (uint32 *)&(paramCB.runtime.testflag[1]), 1);
				}
			}
			break;
			
		// 显示关机界面状态
		case STATE_SHOW_POWER_OFF:
			break;

		// 功能测试
		case STATE_FUCTION_TEST:			
			break;

		default:
			break;
	}	
}


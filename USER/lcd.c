#include "lcd.h"
#include "timer.h"
#include "delay.h"
#include "backlight.h"
#include "Param.h"
#include "state.h"
#include "dtconfig.h"
#include "BleProtocol.h"

SEGMENT_CB segCB;

// 显示屏刷新回调函数
void SEGMENT_CALLBACK_Refresh(uint32 param);
	
// 设置总开关
void LCD_SetGroupOnOff(SEG_MASK_E segMask, BOOL onoff);

// 速度界面
void LCD_DisplaySpeedShow(uint32 param);

// 电量界面
void LCD_DisplayBatteryShow(uint32 param);

// 速度单位显示
void LCD_DisplaySpeedUnitShow(uint32 param);

// 助力 等级[0-9]
void LCD_DisplayAssistLevel(uint32 param);

// 巡航图标显示
void LCD_DisplayCruisesShow(uint32 param);

// 蓝牙图标显示
void LCD_DisplayBlueShow(uint32 param);

// 推车助力图标显示
void LCD_DisplayAssistUp(uint32 param);

// 大灯图标显示
void LCD_DisplayLight(uint32 param);

// 错误码图标显示
void LCD_DisplayError(uint32 param);

// 单次里程图标显示
void LCD_DisplayTripShow(uint32 param);

// 总里程图标显示
void LCD_DisplayOdoShow(uint32 param);

// 平均速度图标显示
void LCD_DisplayAvgSpeedShow(uint32 param);

// 最大速度图标显示
void LCD_DisplayMaxSpeedShow(uint32 param);

// 骑行时间图标显示
void LCD_DisplayTimeShow(uint32 param);

// 卡路里图标显示
void LCD_DisplayKcalShow(uint32 param);

// 公里图标显示
void LCD_DIS_KmShow(uint32 param);

// 英里图标显示
void LCD_DisplayMileShow(uint32 param);

// 电量百分比显示 
void LCD_DisplayBattery_Percent_Show(uint32 param);

// TRIP 显示
void LCD_DisplayRidingTrip(uint32 param);

// 速度外框显示界面
void LCD_DisplaySpeedMarkShow(uint32 param);


	
// 单元测试函数
void LCD_UnitTest(void)
{	
	static uint32 test = 0;
	uint16 Allbuff[32] = {0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
						0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F};
	// 超过10万次全亮
	if (10 > paramCB.runtime.testflag[1])
	{
		LCD_DisplaySpeedShow(test%100*10);
		LCD_DisplayBatteryShow(test%100);
		LCD_DisplayBattery_Percent_Show(test%100);
		LCD_DisplayAssistLevel(test%9);
		LCD_DisplayLight(test%10);
		LCD_DisplayBlueShow(test%10);
		//LCD_DisplaySpeedMarkShow(test%100*10);
		//LCD_DisplayLineShow(test%10);
		LCD_DisplayRidingTrip(test%100*10);
		
		// 间隔一定时间就要显示老化次数，显示的时间长度为
		if (test > 120)
		{
			LCD_DisplaySpeedShow(paramCB.runtime.testflag[1]*10);
		}
		
		test ++;
		test %= 150;			
	}
	else
	{
		LCD_DIS_AllState(Allbuff);
	}
}

// 配置LCD驱动
void LCD_LcdCfg(void)
{
	LCD_InitType Init = {0};
	uint8 uidBuf[16];  

	// 帧频率必须选择在 40Hz ~100Hz 左右的范围
	Init.Divider          = LCD_DIV_25;
	Init.Prescaler        = LCD_PRESCALER_4;		
	Init.Duty             = LCD_DUTY_1_4;					// 40K/(25*2*8)=100Hz
	Init.Bias             = LCD_BIAS_1_3;
	Init.VoltageSource    = LCD_VOLTAGESOURCE_INTERNAL;		// 配置LCD使用外部电源
	Init.Contrast         = LCD_CONTRASTLEVEL_6;
	Init.DeadTime         = LCD_DEADTIME_0;
	Init.PulseOnDuration  = LCD_PULSEONDURATION_7;
	Init.HighDrive        = LCD_HIGHDRIVE_ENABLE;
	Init.HighDriveBuffer  = LCD_HIGHDRIVEBUFFER_ENABLE;
	Init.BlinkMode        = LCD_BLINKMODE_OFF;
	Init.BlinkFreq        = LCD_BLINKFREQ_DIV_8;
	Init.MuxSegment       = LCD_MUXSEGMENT_DISABLE;;

	// 判定MCU硬件版本
	GetUID(uidBuf);
	if (0x11 == uidBuf[2])
	{
		Init.MuxSegment = LCD_MUXSEGMENT_DISABLE;
	}
	else
	{
		// 当设置 MUXSEG 时，输出管脚SEG[43:40]具有与 SEG[31:28]相同的功能，而原 SEG[31:28]管脚不可用
		Init.MuxSegment = LCD_MUXSEGMENT_ENABLE;
	}

	LCD_Init(&Init);
}

void LCD_PortCfg(void)
{
	GPIO_InitType  gpioinitstruct;
	
	// Enable LCD GPIO Clocks
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA | RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOC | RCC_APB2_PERIPH_GPIOD, ENABLE);
	
	// Init GPIO init struct
	GPIO_InitStruct(&gpioinitstruct);

	// Configure peripheral GPIO output for LCD
	// Port A : 
	// PA1: SEG0	PA2: SEG1	PA3: SEG2	PA6: SEG3
	// PA7: SEG4	PA8: COM0	PA9: COM1	PA10: COM2 
	gpioinitstruct.Pin	  = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 
							| GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_15;
	gpioinitstruct.GPIO_Mode	  = GPIO_Mode_Analog;
	gpioinitstruct.GPIO_Pull	  = GPIO_No_Pull;
	gpioinitstruct.GPIO_Alternate = GPIO_AF10_LCD;
	GPIO_InitPeripheral(GPIOA, &gpioinitstruct);

	// Port B 
	// PB0: SEG5	PB1: SEG6	PB3: SEG7	
	// PB4: SEG8    PB5: SEG9 	PB8: SEG16  PB9:COM3  PB10:SEG10  
	// PB11:SEG11  PB12:SEG12  PB13:SEG13 
	// PB14:SEG14  PB15:SEG15 
	gpioinitstruct.Pin	  =   GPIO_PIN_0  | GPIO_PIN_1	| GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 
							| GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14| GPIO_PIN_15;
	GPIO_InitPeripheral(GPIOB, &gpioinitstruct);
	
	// Port C
    // PC0:SEG18 PC1:SEG19 PC2:SEG20 PC3:SEG21 PC4:SEG22 PC5:SEG23 PC6:SEG24 PC7:SEG25 PC8:SEG26 PC9:SEG27 PC10:SEG28 PC11:SEG29 PC12:SEG30
    gpioinitstruct.Pin    =   GPIO_PIN_0  | GPIO_PIN_1	| GPIO_PIN_2| GPIO_PIN_3
		| GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12; 
    GPIO_InitPeripheral(GPIOC, &gpioinitstruct);
	
	// PA15:SEG
	gpioinitstruct.Pin	= GPIO_PIN_15 ;
	gpioinitstruct.GPIO_Alternate	= GPIO_AF10_LCD;
	GPIO_InitPeripheral(GPIOA, &gpioinitstruct);
}


// 显示屏刷新请求回调函数
void SEGMENT_CALLBACK_RefreshRequest(uint32 param)
{
	segCB.refreshRequest = (BOOL)param;
}

uint8 speedtest = 0;
uint8 numbertest = 0;

// 闪烁处理回调函数，由定时器调用
void SEGMENT_CALLBACK_BlinkProcess(uint32 data)
{
	// 闪烁状态翻转
	segCB.blinkPhase = ~segCB.blinkPhase;
}

// 显示屏初始化
void SEGMENT_Init(void)
{
	uint8 i;
	
	LCD_ClockConfig(LCD_CLK_SRC_LSI);
	LCD_PortCfg();
	LCD_LcdCfg();

	// 闪烁相位
	TIMER_AddTask(TIMER_ID_LCD_BLINK,
					500,
					SEGMENT_CALLBACK_BlinkProcess,
					0,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);

	// 显示屏刷新间隔
	TIMER_AddTask(TIMER_ID_LCD_REFRESH,
					100,
					SEGMENT_CALLBACK_RefreshRequest,
					TRUE,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);

	// 显示数据结构初始化	
	segCB.blinkPhase = 0x00;

	for (i = 0; i < COM_MAX; i++)  
	{
		segCB.blinkEnable[i] = 0x00;
		segCB.state[i] = 0x00;
		segCB.onoff[i] = 0x00;
	}

	// 开启总开关
	LCD_SetGroupOnOff(SEG_MASK_ALL, TRUE);
}

void LCD_Process(void)
{
	// 检测刷新标志
	if (segCB.refreshRequest)
	{
		// 清除刷新标志
		segCB.refreshRequest = FALSE;
		
		// 根据segCB结构体 段数值、开关掩码、闪烁掩码，确定 输出段数值
		segCB.outputSeg[SEG_5] = (((segCB.blinkEnable[SEG_5] & segCB.blinkPhase) & segCB.state[SEG_5]) ^ segCB.state[SEG_5]) & segCB.onoff[SEG_5];
		segCB.outputSeg[SEG_6] = (((segCB.blinkEnable[SEG_6] & segCB.blinkPhase) & segCB.state[SEG_6]) ^ segCB.state[SEG_6]) & segCB.onoff[SEG_6];
		segCB.outputSeg[SEG_7] = (((segCB.blinkEnable[SEG_7] & segCB.blinkPhase) & segCB.state[SEG_7]) ^ segCB.state[SEG_7]) & segCB.onoff[SEG_7];
		segCB.outputSeg[SEG_8] = (((segCB.blinkEnable[SEG_8] & segCB.blinkPhase) & segCB.state[SEG_8]) ^ segCB.state[SEG_8]) & segCB.onoff[SEG_8];
		segCB.outputSeg[SEG_9] = (((segCB.blinkEnable[SEG_9] & segCB.blinkPhase) & segCB.state[SEG_9]) ^ segCB.state[SEG_9]) & segCB.onoff[SEG_9];
		segCB.outputSeg[SEG_10] = (((segCB.blinkEnable[SEG_10] & segCB.blinkPhase) & segCB.state[SEG_10]) ^ segCB.state[SEG_10]) & segCB.onoff[SEG_10];
		segCB.outputSeg[SEG_11] = (((segCB.blinkEnable[SEG_11] & segCB.blinkPhase) & segCB.state[SEG_11]) ^ segCB.state[SEG_11]) & segCB.onoff[SEG_11];
		segCB.outputSeg[SEG_12] = (((segCB.blinkEnable[SEG_12] & segCB.blinkPhase) & segCB.state[SEG_12]) ^ segCB.state[SEG_12]) & segCB.onoff[SEG_12];
		segCB.outputSeg[SEG_13] = (((segCB.blinkEnable[SEG_13] & segCB.blinkPhase) & segCB.state[SEG_13]) ^ segCB.state[SEG_13]) & segCB.onoff[SEG_13];
		segCB.outputSeg[SEG_14] = (((segCB.blinkEnable[SEG_14] & segCB.blinkPhase) & segCB.state[SEG_14]) ^ segCB.state[SEG_14]) & segCB.onoff[SEG_14];
		segCB.outputSeg[SEG_15] = (((segCB.blinkEnable[SEG_15] & segCB.blinkPhase) & segCB.state[SEG_15]) ^ segCB.state[SEG_15]) & segCB.onoff[SEG_15];
		segCB.outputSeg[SEG_16] = (((segCB.blinkEnable[SEG_16] & segCB.blinkPhase) & segCB.state[SEG_16]) ^ segCB.state[SEG_16]) & segCB.onoff[SEG_16];
		segCB.outputSeg[SEG_17] = (((segCB.blinkEnable[SEG_17] & segCB.blinkPhase) & segCB.state[SEG_17]) ^ segCB.state[SEG_17]) & segCB.onoff[SEG_17];
		segCB.outputSeg[SEG_18] = (((segCB.blinkEnable[SEG_18] & segCB.blinkPhase) & segCB.state[SEG_18]) ^ segCB.state[SEG_18]) & segCB.onoff[SEG_18];
		segCB.outputSeg[SEG_19] = (((segCB.blinkEnable[SEG_19] & segCB.blinkPhase) & segCB.state[SEG_19]) ^ segCB.state[SEG_19]) & segCB.onoff[SEG_19];
		segCB.outputSeg[SEG_20] = (((segCB.blinkEnable[SEG_20] & segCB.blinkPhase) & segCB.state[SEG_20]) ^ segCB.state[SEG_20]) & segCB.onoff[SEG_20];
		segCB.outputSeg[SEG_21] = (((segCB.blinkEnable[SEG_21] & segCB.blinkPhase) & segCB.state[SEG_21]) ^ segCB.state[SEG_21]) & segCB.onoff[SEG_21];
		segCB.outputSeg[SEG_22] = (((segCB.blinkEnable[SEG_22] & segCB.blinkPhase) & segCB.state[SEG_22]) ^ segCB.state[SEG_22]) & segCB.onoff[SEG_22];
		segCB.outputSeg[SEG_23] = (((segCB.blinkEnable[SEG_23] & segCB.blinkPhase) & segCB.state[SEG_23]) ^ segCB.state[SEG_23]) & segCB.onoff[SEG_23];
		segCB.outputSeg[SEG_24] = (((segCB.blinkEnable[SEG_24] & segCB.blinkPhase) & segCB.state[SEG_24]) ^ segCB.state[SEG_24]) & segCB.onoff[SEG_24];
		segCB.outputSeg[SEG_25] = (((segCB.blinkEnable[SEG_25] & segCB.blinkPhase) & segCB.state[SEG_25]) ^ segCB.state[SEG_25]) & segCB.onoff[SEG_25];
		segCB.outputSeg[SEG_26] = (((segCB.blinkEnable[SEG_26] & segCB.blinkPhase) & segCB.state[SEG_26]) ^ segCB.state[SEG_26]) & segCB.onoff[SEG_26];
		segCB.outputSeg[SEG_27] = (((segCB.blinkEnable[SEG_27] & segCB.blinkPhase) & segCB.state[SEG_27]) ^ segCB.state[SEG_27]) & segCB.onoff[SEG_27];
		segCB.outputSeg[SEG_28] = (((segCB.blinkEnable[SEG_28] & segCB.blinkPhase) & segCB.state[SEG_28]) ^ segCB.state[SEG_28]) & segCB.onoff[SEG_28];
		segCB.outputSeg[SEG_29] = (((segCB.blinkEnable[SEG_29] & segCB.blinkPhase) & segCB.state[SEG_29]) ^ segCB.state[SEG_29]) & segCB.onoff[SEG_29];
		segCB.outputSeg[SEG_30] = (((segCB.blinkEnable[SEG_30] & segCB.blinkPhase) & segCB.state[SEG_30]) ^ segCB.state[SEG_30]) & segCB.onoff[SEG_30];
		segCB.outputSeg[SEG_31] = (((segCB.blinkEnable[SEG_31] & segCB.blinkPhase) & segCB.state[SEG_31]) ^ segCB.state[SEG_31]) & segCB.onoff[SEG_31];

		segCB.outputSeg[SEG_32] = (((segCB.blinkEnable[SEG_32] & segCB.blinkPhase) & segCB.state[SEG_32]) ^ segCB.state[SEG_32]) & segCB.onoff[SEG_32];
		segCB.outputSeg[SEG_33] = (((segCB.blinkEnable[SEG_33] & segCB.blinkPhase) & segCB.state[SEG_33]) ^ segCB.state[SEG_33]) & segCB.onoff[SEG_33];

		segCB.outputSeg[SEG_34] = (((segCB.blinkEnable[SEG_34] & segCB.blinkPhase) & segCB.state[SEG_34]) ^ segCB.state[SEG_34]) & segCB.onoff[SEG_34];
		segCB.outputSeg[SEG_35] = (((segCB.blinkEnable[SEG_35] & segCB.blinkPhase) & segCB.state[SEG_35]) ^ segCB.state[SEG_35]) & segCB.onoff[SEG_35];			
		// 刷新显示屏
		SEGMENT_CALLBACK_Refresh(TRUE);
	}
}

// 显示屏刷新回调函数
void SEGMENT_CALLBACK_Refresh(uint32 param)
{
	uint8 i,j = 0;
	
	// 
	for (i = 0; i < COM_MAX;i++)
	{
		segCB.mapData[i] = 0x00;
		for (j = 0; j < SEG_MAX;j++)
		{
			segCB.mapData[i] |= (((segCB.outputSeg[j] & (0x01 << i))?1:0) << j);
		}
	}
	
	for (i = 0; i < COM_MAX; i++)
	{
		LCD_Write(i * 2, 0x00000000, segCB.mapData[i]);
	}
	
	LCD_UpdateDisplayRequest();
}

// 设置总开关
void LCD_SetGroupOnOff(SEG_MASK_E segMask, BOOL onoff)
{
	if (segMask & SEG_MASK_ALL)
	{
		if (onoff)
		{
			segCB.onoff[SEG_5] |= 0x0F;
			segCB.onoff[SEG_6] |= 0x0F;
			segCB.onoff[SEG_7] |= 0x0F;
			segCB.onoff[SEG_8] |= 0x0F;
			segCB.onoff[SEG_9] |= 0x0F;
			segCB.onoff[SEG_10] |= 0x0F;
			segCB.onoff[SEG_11] |= 0x0F;
			segCB.onoff[SEG_12] |= 0x0F;
			segCB.onoff[SEG_13] |= 0x0F;
			segCB.onoff[SEG_14] |= 0x0F;
			segCB.onoff[SEG_15] |= 0x0F;
			segCB.onoff[SEG_16] |= 0x0F;
			segCB.onoff[SEG_17] |= 0x0F;
			segCB.onoff[SEG_18] |= 0x0F;
			segCB.onoff[SEG_19] |= 0x0F;
			segCB.onoff[SEG_20] |= 0x0F;
			segCB.onoff[SEG_21] |= 0x0F;
			segCB.onoff[SEG_22] |= 0x0F;
			segCB.onoff[SEG_23] |= 0x0F;
			segCB.onoff[SEG_24] |= 0x0F;
			segCB.onoff[SEG_25] |= 0x0F;
			segCB.onoff[SEG_26] |= 0x0F;
			segCB.onoff[SEG_27] |= 0x0F;
			segCB.onoff[SEG_28] |= 0x0F;
			segCB.onoff[SEG_29] |= 0x0F;
			segCB.onoff[SEG_30] |= 0x0F;
			segCB.onoff[SEG_31] |= 0x0F;
			segCB.onoff[SEG_32] |= 0x0F;
			segCB.onoff[SEG_33] |= 0x0F;
			segCB.onoff[SEG_34] |= 0x0F;
			segCB.onoff[SEG_35] |= 0x0F;		
		}
		else
		{
			segCB.onoff[SEG_5] &= (~0x0F);
			segCB.onoff[SEG_6] &= (~0x0F);
			segCB.onoff[SEG_7] &= (~0x0F);
			segCB.onoff[SEG_8] &= (~0x0F);
			segCB.onoff[SEG_9] &= (~0x0F);
			segCB.onoff[SEG_10] &= (~0x0F);
			segCB.onoff[SEG_11] &= (~0x0F);
			segCB.onoff[SEG_12] &= (~0x0F);
			segCB.onoff[SEG_13] &= (~0x0F);
			segCB.onoff[SEG_14] &= (~0x0F);
			segCB.onoff[SEG_15] &= (~0x0F);
			segCB.onoff[SEG_16] &= (~0x0F);
			segCB.onoff[SEG_17] &= (~0x0F);
			segCB.onoff[SEG_18] &= (~0x0F);
			segCB.onoff[SEG_19] &= (~0x0F);
			segCB.onoff[SEG_20] &= (~0x0F);
			segCB.onoff[SEG_21] &= (~0x0F);
			segCB.onoff[SEG_22] &= (~0x0F);
			segCB.onoff[SEG_23] &= (~0x0F);
			segCB.onoff[SEG_24] &= (~0x0F);
			segCB.onoff[SEG_25] &= (~0x0F);
			segCB.onoff[SEG_26] &= (~0x0F);
			segCB.onoff[SEG_27] &= (~0x0F);
			segCB.onoff[SEG_28] &= (~0x0F);
			segCB.onoff[SEG_29] &= (~0x0F);
			segCB.onoff[SEG_30] &= (~0x0F);
			segCB.onoff[SEG_31] &= (~0x0F);
			segCB.onoff[SEG_32] &= (~0x0F);
			segCB.onoff[SEG_33] &= (~0x0F);
			segCB.onoff[SEG_34] &= (~0x0F);
			segCB.onoff[SEG_35] &= (~0x0F);
		}
		return;
	}

	// 速度数字界面
	if(segMask & SEG_MASK_SPEED_NUM)
	{
		if(onoff)
		{
			segCB.onoff[SEG_6] |= 0x0F;
			segCB.onoff[SEG_7] |= 0x0E;
			
			segCB.onoff[SEG_8] |= 0x0F;
			segCB.onoff[SEG_9] |= 0x0F;

			segCB.onoff[SEG_10] |= 0x0F;
			segCB.onoff[SEG_11] |= 0x0E;	
		}
		else
		{
			segCB.onoff[SEG_6] &= (~0x0F);
			segCB.onoff[SEG_7] &= (~0x0E);
			
			segCB.onoff[SEG_8] &= (~0x0F);
			segCB.onoff[SEG_9] &= (~0x0F);

			segCB.onoff[SEG_10] &= (~0x0F);
			segCB.onoff[SEG_11] &= (~0x0E);
			
		}
	}

	// 速度公制单位界面
	if(segMask & SEG_MASK_KMH)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_11] |= 0x01;
		}
		else
		{
			segCB.onoff[SEG_11] &= (~0x01);
		}
	}

	// 速度英制单位界面
	if(segMask & SEG_MASK_MPH)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_30] |= 0x02;
		}
		else
		{
			segCB.onoff[SEG_30] &= (~0x02);
		}
	}

	// 蓝牙图标界面
	if(segMask & SEG_MASK_BLE)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_7] |= 0x01;
		}
		else
		{
			segCB.onoff[SEG_7] &= (~0x01);
		}
	}

	// 电池图标界面
	if(segMask & SEG_MASK_BATTERY_ICON)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_30] |= 0x04;
		}
		else
		{
			segCB.onoff[SEG_30] &= (~0x04);
		}
	}

	// 电池容量界面
	if(segMask & SEG_MASK_BATTERY_QUANTITY)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_30] |= 0x08;
			segCB.onoff[SEG_31] |= 0x0F;
		}
		else
		{
			segCB.onoff[SEG_30] &= (~0x08);
			segCB.onoff[SEG_31] &= (~0x0F);
		}
	}

	// 错误图标界面
	if(segMask & SEG_MASK_ERROR)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_5] |= 0x01;
		}
		else
		{
			segCB.onoff[SEG_5] &= (~0x01);
		}
	}

	// 大灯图标界面
	if(segMask & SEG_MASK_LIGHT)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_5] |= 0x08;
		}
		else
		{
			segCB.onoff[SEG_5] &= (~0x08);
		}
	}

	// 巡航图标界面
	if(segMask & SEG_MASK_CRUISE)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_5] |= 0x02;
		}
		else
		{
			segCB.onoff[SEG_5] &= (~0x02);
		}
	}

	// 推车助力图标界面
	if(segMask & SEG_MASK_ASSIST_UP)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_5] |= 0x04;
		}
		else
		{
			segCB.onoff[SEG_5] &= (~0x04);
		}
	}

	// 助力档位界面
	if(segMask & SEG_MASK_ASSIST_LEVEL)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_25] |= 0x0F;
			segCB.onoff[SEG_26] |= 0x0F;
		}
		else
		{
			segCB.onoff[SEG_25] &= (~0x0F);
			segCB.onoff[SEG_26] &= (~0x0F);
		}
	}

	// 功率界面
	if(segMask & SEG_MASK_POWER)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_27] |= 0x0E;
			segCB.onoff[SEG_28] |= 0x0F;
			segCB.onoff[SEG_29] |= 0x0E;
			segCB.onoff[SEG_30] |= 0x01;
		}
		else
		{
			segCB.onoff[SEG_27] &= (~0x0E);
			segCB.onoff[SEG_28] &= (~0x0F);
			segCB.onoff[SEG_29] &= (~0x0E);
			segCB.onoff[SEG_30] &= (~0x01);
		}
	}

	// TRIP图标界面
	if(segMask & SEG_MASK_TRIP)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_24] |= 0x04;
		}
		else
		{
			segCB.onoff[SEG_24] &= (~0x04);
		}
	}

	// ODO图标界面
	if(segMask & SEG_MASK_ODO)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_24] |= 0x08;
		}
		else
		{
			segCB.onoff[SEG_24] &= (~0x08);
		}
	}

	// AVG图标界面
	if(segMask & SEG_MASK_SPEED_AVG)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_22] |= 0x08;
		}
		else
		{
			segCB.onoff[SEG_22] &= (~0x08);
		}
	}

	// MAX图标界面
	if(segMask & SEG_MASK_SPEED_MAX)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_20] |= 0x08;
		}
		else
		{
			segCB.onoff[SEG_20] &= (~0x08);
		}
	}

	// 时间图标界面
	if(segMask & SEG_MASK_TIME)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_24] |= 0x02;
		}
		else
		{
			segCB.onoff[SEG_24] &= (~0x02);
		}
	}

	// 公里图标界面
	if(segMask & SEG_MASK_KM)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_12] |= 0x08;
			segCB.onoff[SEG_14] |= 0x08;
		}
		else
		{
			segCB.onoff[SEG_12] &= (~0x08);
			segCB.onoff[SEG_14] &= (~0x08);
		}
	}

	// 英里图标界面
	if(segMask & SEG_MASK_MILE)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_16] |= 0x08;
			segCB.onoff[SEG_18] |= 0x08;
		}
		else
		{
			segCB.onoff[SEG_16] &= (~0x08);
			segCB.onoff[SEG_18] &= (~0x08);
		}
	}

	// 菜单数字1界面
	if(segMask & SEG_MASK_SMALL_NUM1)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_22] |= 0x07;
			segCB.onoff[SEG_23] |= 0x0F;
		}
		else
		{
			segCB.onoff[SEG_22] &= (~0x07);
			segCB.onoff[SEG_23] &= (~0x0F);
		}
	}

	// 菜单数字2界面
	if(segMask & SEG_MASK_SMALL_NUM2)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_20] |= 0x07;
			segCB.onoff[SEG_21] |= 0x0F;
			
			segCB.onoff[SEG_27] |= 0x01;
		}
		else
		{
			segCB.onoff[SEG_20] &= (~0x07);
			segCB.onoff[SEG_21] &= (~0x0F);

			segCB.onoff[SEG_27] |= 0x01;
		}
	}

	// 菜单数字3界面
	if(segMask & SEG_MASK_SMALL_NUM3)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_18] |= 0x07;
			segCB.onoff[SEG_19] |= 0x0F;
		}
		else
		{
			segCB.onoff[SEG_18] &= (~0x07);
			segCB.onoff[SEG_19] &= (~0x0F);
		}
	}

	// 菜单数字4界面
	if(segMask & SEG_MASK_SMALL_NUM4)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_16] |= 0x07;
			segCB.onoff[SEG_17] |= 0x0F;
		}
		else
		{
			segCB.onoff[SEG_16] &= (~0x07);
			segCB.onoff[SEG_17] &= (~0x0F);
		}
	}

	// 菜单数字5界面
	if(segMask & SEG_MASK_SMALL_NUM5)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_14] |= 0x07;
			segCB.onoff[SEG_15] |= 0x0F;

			segCB.onoff[SEG_29] |= 0x01;	
		}
		else
		{
			segCB.onoff[SEG_14] &= (~0x07);
			segCB.onoff[SEG_15] &= (~0x0F);

			segCB.onoff[SEG_29] &= (~0x01);
		}
	}

	// 菜单数字6界面
	if(segMask & SEG_MASK_SMALL_NUM6)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_12] |= 0x07;
			segCB.onoff[SEG_13] |= 0x0F;
		}
		else
		{
			segCB.onoff[SEG_12] &= (~0x07);
			segCB.onoff[SEG_13] &= (~0x0F);
		}
	}

		// 电量数字百分比
	if(segMask & SEG_MASK_BAT_PERCENT)
	{
		if(onoff)
		{	
			segCB.onoff[SEG_32] |= 0x0F;
			segCB.onoff[SEG_33] |= 0x0F;
			segCB.onoff[SEG_34] |= 0x0F;
			segCB.onoff[SEG_35] |= 0x0F;
		}
		else
		{
			segCB.onoff[SEG_32] &= (~0x0F); //把"1"关闭
			segCB.onoff[SEG_33] &= (~0x0F);
			segCB.onoff[SEG_34] &= (~0x0F);
			segCB.onoff[SEG_35] &= (~0x0F);
		}
	}

}

// 设置闪烁
void LCD_SetGroupBlink(SEG_MASK_E segMask, BOOL onoff)
{
	if (segMask & SEG_MASK_ALL)
	{
		if (onoff)
		{
			segCB.blinkEnable[SEG_5] |= 0x0F;
			segCB.blinkEnable[SEG_6] |= 0x0F;
			segCB.blinkEnable[SEG_7] |= 0x0F;
			segCB.blinkEnable[SEG_8] |= 0x0F;
			segCB.blinkEnable[SEG_9] |= 0x0F;
			segCB.blinkEnable[SEG_10] |= 0x0F;
			segCB.blinkEnable[SEG_11] |= 0x0F;
			segCB.blinkEnable[SEG_12] |= 0x0F;
			segCB.blinkEnable[SEG_13] |= 0x0F;
			segCB.blinkEnable[SEG_14] |= 0x0F;
			segCB.blinkEnable[SEG_15] |= 0x0F;
			segCB.blinkEnable[SEG_16] |= 0x0F;
			segCB.blinkEnable[SEG_17] |= 0x0F;
			segCB.blinkEnable[SEG_18] |= 0x0F;
			segCB.blinkEnable[SEG_19] |= 0x0F;
			segCB.blinkEnable[SEG_20] |= 0x0F;
			segCB.blinkEnable[SEG_21] |= 0x0F;
			segCB.blinkEnable[SEG_22] |= 0x0F;
			segCB.blinkEnable[SEG_23] |= 0x0F;
			segCB.blinkEnable[SEG_24] |= 0x0F;
			segCB.blinkEnable[SEG_25] |= 0x0F;
			segCB.blinkEnable[SEG_26] |= 0x0F;
			segCB.blinkEnable[SEG_27] |= 0x0F;
			segCB.blinkEnable[SEG_28] |= 0x0F;
			segCB.blinkEnable[SEG_29] |= 0x0F;
			segCB.blinkEnable[SEG_30] |= 0x0F;
			segCB.blinkEnable[SEG_31] |= 0x0F;
			segCB.blinkEnable[SEG_32] |= 0x0F;
			segCB.blinkEnable[SEG_33] |= 0x0F;
			segCB.blinkEnable[SEG_34] |= 0x0F;
			segCB.blinkEnable[SEG_35] |= 0x0F;
		}
		else
		{
			segCB.blinkEnable[SEG_5] &= (~0x0F);
			segCB.blinkEnable[SEG_6] &= (~0x0F);
			segCB.blinkEnable[SEG_7] &= (~0x0F);
			segCB.blinkEnable[SEG_8] &= (~0x0F);
			segCB.blinkEnable[SEG_9] &= (~0x0F);
			segCB.blinkEnable[SEG_10] &= (~0x0F);
			segCB.blinkEnable[SEG_11] &= (~0x0F);
			segCB.blinkEnable[SEG_12] &= (~0x0F);
			segCB.blinkEnable[SEG_13] &= (~0x0F);
			segCB.blinkEnable[SEG_14] &= (~0x0F);
			segCB.blinkEnable[SEG_15] &= (~0x0F);
			segCB.blinkEnable[SEG_16] &= (~0x0F);
			segCB.blinkEnable[SEG_17] &= (~0x0F);
			segCB.blinkEnable[SEG_18] &= (~0x0F);
			segCB.blinkEnable[SEG_19] &= (~0x0F);
			segCB.blinkEnable[SEG_20] &= (~0x0F);
			segCB.blinkEnable[SEG_21] &= (~0x0F);
			segCB.blinkEnable[SEG_22] &= (~0x0F);
			segCB.blinkEnable[SEG_23] &= (~0x0F);
			segCB.blinkEnable[SEG_24] &= (~0x0F);
			segCB.blinkEnable[SEG_25] &= (~0x0F);
			segCB.blinkEnable[SEG_26] &= (~0x0F);
			segCB.blinkEnable[SEG_27] &= (~0x0F);
			segCB.blinkEnable[SEG_28] &= (~0x0F);
			segCB.blinkEnable[SEG_29] &= (~0x0F);
			segCB.blinkEnable[SEG_30] &= (~0x0F);
			segCB.blinkEnable[SEG_31] &= (~0x0F);
			segCB.blinkEnable[SEG_32] &= (~0x0F);
			segCB.blinkEnable[SEG_33] &= (~0x0F);
			segCB.blinkEnable[SEG_34] &= (~0x0F);
			segCB.blinkEnable[SEG_35] &= (~0x0F);
		}
		return;
	}

	// 速度数字界面
	if(segMask & SEG_MASK_SPEED_NUM)
	{
		if(onoff)
		{
			segCB.blinkEnable[SEG_6] |= 0x0F;
			segCB.blinkEnable[SEG_7] |= 0x0E;
			
			segCB.blinkEnable[SEG_8] |= 0x0F;
			segCB.blinkEnable[SEG_9] |= 0x0F;

			segCB.blinkEnable[SEG_10] |= 0x0F;
			segCB.blinkEnable[SEG_11] |= 0x0E;	
		}
		else
		{
			segCB.blinkEnable[SEG_6] &= (~0x0F);
			segCB.blinkEnable[SEG_7] &= (~0x0E);
			
			segCB.blinkEnable[SEG_8] &= (~0x0F);
			segCB.blinkEnable[SEG_9] &= (~0x0F);

			segCB.blinkEnable[SEG_10] &= (~0x0F);
			segCB.blinkEnable[SEG_11] &= (~0x0E);
			
		}
	}

	// 速度公制单位界面
	if(segMask & SEG_MASK_KMH)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_11] |= 0x01;
		}
		else
		{
			segCB.blinkEnable[SEG_11] &= (~0x01);
		}
	}

	// 速度英制单位界面
	if(segMask & SEG_MASK_MPH)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_30] |= 0x02;
		}
		else
		{
			segCB.blinkEnable[SEG_30] &= (~0x02);
		}
	}

	// 蓝牙图标界面
	if(segMask & SEG_MASK_BLE)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_7] |= 0x01;
		}
		else
		{
			segCB.blinkEnable[SEG_7] &= (~0x01);
		}
	}

	// 电池图标界面
	if(segMask & SEG_MASK_BATTERY_ICON)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_30] |= 0x04;
		}
		else
		{
			segCB.blinkEnable[SEG_30] &= (~0x04);
		}
	}

	// 电池容量界面
	if(segMask & SEG_MASK_BATTERY_QUANTITY)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_30] |= 0x08;
			segCB.blinkEnable[SEG_31] |= 0x0F;
		}
		else
		{
			segCB.blinkEnable[SEG_30] &= (~0x08);
			segCB.blinkEnable[SEG_31] &= (~0x0F);
		}
	}

	// 错误图标界面
	if(segMask & SEG_MASK_ERROR)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_5] |= 0x01;
		}
		else
		{
			segCB.blinkEnable[SEG_5] &= (~0x01);
		}
	}

	// 大灯图标界面
	if(segMask & SEG_MASK_LIGHT)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_5] |= 0x08;
		}
		else
		{
			segCB.blinkEnable[SEG_5] &= (~0x08);
		}
	}

	// 巡航图标界面
	if(segMask & SEG_MASK_CRUISE)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_5] |= 0x02;
		}
		else
		{
			segCB.blinkEnable[SEG_5] &= (~0x02);
		}
	}

	// 推车助力图标界面
	if(segMask & SEG_MASK_ASSIST_UP)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_5] |= 0x04;
		}
		else
		{
			segCB.blinkEnable[SEG_5] &= (~0x04);
		}
	}

	// 助力档位界面
	if(segMask & SEG_MASK_ASSIST_LEVEL)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_25] |= 0x0F;
			segCB.blinkEnable[SEG_26] |= 0x0F;
		}
		else
		{
			segCB.blinkEnable[SEG_25] &= (~0x0F);
			segCB.blinkEnable[SEG_26] &= (~0x0F);
		}
	}

	// 功率界面
	if(segMask & SEG_MASK_POWER)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_27] |= 0x0E;
			segCB.blinkEnable[SEG_28] |= 0x0F;
			segCB.blinkEnable[SEG_29] |= 0x0E;
			segCB.blinkEnable[SEG_30] |= 0x01;
		}
		else
		{
			segCB.blinkEnable[SEG_27] &= (~0x0E);
			segCB.blinkEnable[SEG_28] &= (~0x0F);
			segCB.blinkEnable[SEG_29] &= (~0x0E);
			segCB.blinkEnable[SEG_30] &= (~0x01);
		}
	}

	// TRIP图标界面
	if(segMask & SEG_MASK_TRIP)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_24] |= 0x04;
		}
		else
		{
			segCB.blinkEnable[SEG_24] &= (~0x04);
		}
	}

	// ODO图标界面
	if(segMask & SEG_MASK_ODO)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_24] |= 0x08;
		}
		else
		{
			segCB.blinkEnable[SEG_24] &= (~0x08);
		}
	}

	// AVG图标界面
	if(segMask & SEG_MASK_SPEED_AVG)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_22] |= 0x08;
		}
		else
		{
			segCB.blinkEnable[SEG_22] &= (~0x08);
		}
	}

	// MAX图标界面
	if(segMask & SEG_MASK_SPEED_MAX)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_20] |= 0x08;
		}
		else
		{
			segCB.blinkEnable[SEG_20] &= (~0x08);
		}
	}

	// 时间图标界面
	if(segMask & SEG_MASK_TIME)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_24] |= 0x02;
		}
		else
		{
			segCB.blinkEnable[SEG_24] &= (~0x02);
		}
	}

	// 公里图标界面
	if(segMask & SEG_MASK_KM)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_12] |= 0x08;
			segCB.blinkEnable[SEG_14] |= 0x08;
		}
		else
		{
			segCB.blinkEnable[SEG_12] &= (~0x08);
			segCB.blinkEnable[SEG_14] &= (~0x08);
		}
	}

	// 英里图标界面
	if(segMask & SEG_MASK_MILE)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_16] |= 0x08;
			segCB.blinkEnable[SEG_18] |= 0x08;
		}
		else
		{
			segCB.blinkEnable[SEG_16] &= (~0x08);
			segCB.blinkEnable[SEG_18] &= (~0x08);
		}
	}

	// 菜单数字1界面
	if(segMask & SEG_MASK_SMALL_NUM1)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_22] |= 0x07;
			segCB.blinkEnable[SEG_23] |= 0x0F;
		}
		else
		{
			segCB.blinkEnable[SEG_22] &= (~0x07);
			segCB.blinkEnable[SEG_23] &= (~0x0F);
		}
	}

	// 菜单数字2界面
	if(segMask & SEG_MASK_SMALL_NUM2)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_20] |= 0x07;
			segCB.blinkEnable[SEG_21] |= 0x0F;
			
			segCB.blinkEnable[SEG_27] |= 0x01;
		}
		else
		{
			segCB.blinkEnable[SEG_20] &= (~0x07);
			segCB.blinkEnable[SEG_21] &= (~0x0F);

			segCB.blinkEnable[SEG_27] |= 0x01;
		}
	}

	// 菜单数字3界面
	if(segMask & SEG_MASK_SMALL_NUM3)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_18] |= 0x07;
			segCB.blinkEnable[SEG_19] |= 0x0F;
		}
		else
		{
			segCB.blinkEnable[SEG_18] &= (~0x07);
			segCB.blinkEnable[SEG_19] &= (~0x0F);
		}
	}

	// 菜单数字4界面
	if(segMask & SEG_MASK_SMALL_NUM4)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_16] |= 0x07;
			segCB.blinkEnable[SEG_17] |= 0x0F;
		}
		else
		{
			segCB.blinkEnable[SEG_16] &= (~0x07);
			segCB.blinkEnable[SEG_17] &= (~0x0F);
		}
	}

	// 菜单数字5界面
	if(segMask & SEG_MASK_SMALL_NUM5)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_14] |= 0x07;
			segCB.blinkEnable[SEG_15] |= 0x0F;

			segCB.blinkEnable[SEG_29] |= 0x01;	
		}
		else
		{
			segCB.blinkEnable[SEG_14] &= (~0x07);
			segCB.blinkEnable[SEG_15] &= (~0x0F);

			segCB.blinkEnable[SEG_29] &= (~0x01);
		}
	}

	// 菜单数字6界面
	if(segMask & SEG_MASK_SMALL_NUM6)
	{
		if(onoff)
		{	
			segCB.blinkEnable[SEG_12] |= 0x07;
			segCB.blinkEnable[SEG_13] |= 0x0F;
		}
		else
		{
			segCB.blinkEnable[SEG_12] &= (~0x07);
			segCB.blinkEnable[SEG_13] &= (~0x0F);
		}
	}
}

// 显示屏全部显示
void LCD_DIS_AllState(uint16 segBuff[])
{		
	// 数值置位
	segCB.state[SEG_5] = segBuff[0];
	segCB.state[SEG_6] = segBuff[1];
	segCB.state[SEG_7] = segBuff[2];
	segCB.state[SEG_8] = segBuff[3];
	segCB.state[SEG_9] = segBuff[4];
	segCB.state[SEG_10] = segBuff[5];
	segCB.state[SEG_11] = segBuff[6];
	segCB.state[SEG_12] = segBuff[7];
	segCB.state[SEG_13] = segBuff[8];
	segCB.state[SEG_14] = segBuff[9];
	segCB.state[SEG_15] = segBuff[10];
	segCB.state[SEG_16] = segBuff[11];
	segCB.state[SEG_17] = segBuff[12];
	segCB.state[SEG_18] = segBuff[13];
	segCB.state[SEG_19] = segBuff[14];
	segCB.state[SEG_20] = segBuff[15];
	segCB.state[SEG_21] = segBuff[16];
	segCB.state[SEG_22] = segBuff[17];
	segCB.state[SEG_23] = segBuff[18];
	segCB.state[SEG_24] = segBuff[19];
	segCB.state[SEG_25] = segBuff[20];
	segCB.state[SEG_26] = segBuff[21];
	segCB.state[SEG_27] = segBuff[22];
	segCB.state[SEG_28] = segBuff[23];
	segCB.state[SEG_29] = segBuff[24];
	segCB.state[SEG_30] = segBuff[25];
	segCB.state[SEG_31] = segBuff[26];
	segCB.state[SEG_32] = segBuff[27];
	segCB.state[SEG_33] = segBuff[28];
	segCB.state[SEG_34] = segBuff[29];
	segCB.state[SEG_35] = segBuff[30];
}



/**************************************************************************************
***************************************************************************************
************************************字符显示*******************************************
***************************************************************************************
***************************************************************************************/
const uint8 asciiTable[127]= 
{

	    0x00,	//  1
	    0x00,	//
	    0x00,	//	
		0x00,	//
		0x00,	//
		0x00,	//	
		0x00,	// 2
		0x00,	//
		0x00,	//	
		0x00,	//
		0x00,	//
		0x00,	//	
	    0x00,	// 3
	    0x00,	//
	    0x00,	//	
		0x00,	//
		0x00,	//
		0x00,	//	
	    0x00,	// 4
	    0x00,	//
	    0x00,	//	
		0x00,	//
		0x00,	//
		0x00,	//
		0x00,	// 5
		0x00,	//
		0x00,	//	
		0x00,	//
		0x00,	//
		0x00,	//	
	    0x00,	// 6
	    0x00,	//
	    0x00,	//	
		0x00,	//
		0x00,	//
		0x00,	//	
	    0x00,	// 7
	    0x00,	//
	    0x00,	//	
		0x00,	//
		0x00,	//
		0x00,	//	
	    0x00,	//8
	    0x00,	//
	    0x00,	//	
		0x20,	//
		0x00,	//
		0x00,	//	
		0x5f,	// 0     //48  	//FGEDCBA			
		0x06,	// 1						 
		0x3b,	// 2
		0x2f,	// 3									
		0x66,	// 4									   
		0x6d,	// 5
		0x7d,	// 6											
		0x07,	// 7 
		0x7f,	// 8 
		0x6f,	// 9     //57       
	    0x00,	//
	    0x00,	//
	    0x00,	//											A
	    0x20,	//										************
		0x00,	//										*          *
  		0x00,	//										*          *B
		0x00,	//									   F*          *
		0x77,	// A    //65							*     G    *
		0x7c,	// B									************
		0x59,	// C  									*          *
		0x3e,	// D 									*          *C
		0x79,	// E								   E*          *
		0x71,	// F									*          *
		0x6f,	// G									************
	  	0x76,	// H										 D
		0x06,	// I
		0x0e,	// J
		0x7a,	// K
		0x58,	// L
		0x35,	// M
		0x34,	// N
		0x5f,	// O
		0x73,	// P
		0x67,	// Q
		0x30,	// R
		0x6D,	// S
		0x78,	// T
		0x5E,	// U
		0x62,	// V
		0x7E,	// W
		0x56,	// X
		0x6e,	// Y
		0x3b,	// Z 90
		0x00,   // 91
		0x00,	
		0x00,
		0x00,
		0x08,  	// - 95
		0x00,  	// 96
		0x77,	// A    //97							
		0x7c,	// B								
		0x59,	// C  									
		0x3e,	// D 									
		0x79,	// E								  
		0x71,	// F									
		0x6f,	// G									
	  	0x76,	// H										
		0x06,	// I
		0x0e,	// J
		0x7a,	// K
		0x58,	// L
		0x35,	// M
		0x34,	// N
		0x5f,	// O
		0x73,	// P
		0x67,	// Q
		0x30,	// R
		0x6D,	// S
		0x78,	// T
		0x5E,	// U
		0x62,	// V
		0x7E,	// W
		0x56,	// X
		0x6e,	// Y
		0x3b,	// Z 
		0x00,   // 	

		0x00,
		0x00,
		0x00,	
};

// 16进制显示
const uint8 numberTable[16]= 
{
		0x5f,	// 0	 	//FGEDCBA			
		0x06,	// 1						 
		0x3b,	// 2
		0x2f,	// 3									
		0x66,	// 4									   
		0x6d,	// 5
		0x7d,	// 6											
		0x07,	// 7 
		0x7f,	// 8 
		0x6f,	// 9	
		0x77,	// A								
		0x7c,	// B								
		0x59,	// C									
		0x3e,	// D									
		0x79,	// E								  
		0x71,	// F	
};


// 电量百分比显示
void LCD_DisplayBatperNum1(uint32 param)
{	

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08

	//C:0x01
	//G:0x02
	//B:0x04

	uint8 value1 = 0,value2 = 0;
	
	value1 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);
	value2 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;

	segCB.state[SEG_32] = (segCB.state[SEG_32] & (~0x0F)) | (value1);
	segCB.state[SEG_33] = (segCB.state[SEG_33] & (~0x07)) | (value2);		
}

// 电量百分比显示

void LCD_DisplayBatperNum2(uint32 param)
{

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08

	//C:0x01
	//G:0x02
	//B:0x04

	uint8 value1 = 0,value2 = 0;
	
	value1 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);
	value2 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;
	
	segCB.state[SEG_34] = (segCB.state[SEG_34] & (~0x0F)) | (value1);
	segCB.state[SEG_35] = (segCB.state[SEG_35] & (~0x07)) | (value2);	
	
}





// 档位显示
void LCD_DisplayAssitNum(uint32 param)
{

	//C:0x01
	//G:0x02
	//B:0x04

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08
	
	uint8 value1 = 0,value2 = 0;

	value1 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;
	
	value2 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);

	segCB.state[SEG_25] = (segCB.state[SEG_25] & (~0x07)) | (value1);
	segCB.state[SEG_26] = (segCB.state[SEG_26] & (~0x0F)) | (value2);
	
}
// 显示速度字符
void LCD_DisplaySpeedNum1(uint32 param)
{
	//A:0x01
	//F:0x02	
	//E:0x04
	//D:0x08
	//B:0x02
	//G:0x04
	//C:0x08

	uint8 value1 = 0,value2 = 0;

	value1 = (param & 0x01)|( (param >>5)& 0x02)|((param >>2)& 0x04)|(param& 0x08) ;

	value2 = (param & 0x02)|( (param >>3)& 0x04)|((param << 1)& 0x08) ;

	segCB.state[SEG_6] = (segCB.state[SEG_6] & (~0x0F)) | (value1);
	segCB.state[SEG_7] = (segCB.state[SEG_7] & (~0x0E)) | (value2);

	
}

void LCD_DisplaySpeedNum2(uint32 param)
{
	//A:0x01
	//F:0x02	
	//E:0x04
	//D:0x08
	//B:0x02
	//G:0x04
	//C:0x08

	uint8 value1 = 0,value2 = 0;

	value1 = (param & 0x01)|( (param >>5)& 0x02)|((param >>2)& 0x04)|(param& 0x08) ;

	value2 = (param & 0x02)|( (param >>3)& 0x04)|((param << 1)& 0x08) ;

	segCB.state[SEG_8] = (segCB.state[SEG_8] & (~0x0F)) | (value1);
	segCB.state[SEG_9] = (segCB.state[SEG_9] & (~0x0E)) | (value2);
}

void LCD_DisplaySpeedNum3(uint32 param)
{
	//A:0x01
	//F:0x02	
	//E:0x04
	//D:0x08
	//B:0x02
	//G:0x04
	//C:0x08

	uint8 value1 = 0,value2 = 0;

	value1 = (param & 0x01)|( (param >>5)& 0x02)|((param >>2)& 0x04)|(param& 0x08) ;

	value2 = (param & 0x02)|( (param >>3)& 0x04)|((param << 1)& 0x08) ;

	segCB.state[SEG_10] = (segCB.state[SEG_10] & (~0x0F)) | (value1);
	segCB.state[SEG_11] = (segCB.state[SEG_11] & (~0x0E)) | (value2);
}


// 菜单栏数字1显示
void LCD_DisplayMenuNum1(uint32 param)
{

	//C:0x01
	//G:0x02
	//B:0x04

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08
	
	uint8 value1 = 0,value2 = 0;

	value1 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;
	
	value2 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);
	

	segCB.state[SEG_22] = (segCB.state[SEG_22] & (~0x07)) | (value1);
	segCB.state[SEG_23] = (segCB.state[SEG_23] & (~0x0F)) | (value2); 		
	
}

// 菜单栏数字2显示
void LCD_DisplayMenuNum2(uint32 param)
{
	//C:0x01
	//G:0x02
	//B:0x04

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08
	
	uint8 value1 = 0,value2 = 0;

	value1 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;
	
	value2 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);
	
	segCB.state[SEG_20] = (segCB.state[SEG_20] & (~0x07)) | (value1);
	segCB.state[SEG_21] = (segCB.state[SEG_21] & (~0x0F)) | (value2);
}

// 菜单栏数字3显示
void LCD_DisplayMenuNum3(uint32 param)
{
	//C:0x01
	//G:0x02
	//B:0x04

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08
	
	uint8 value1 = 0,value2 = 0;

	value1 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;
	
	value2 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);

	segCB.state[SEG_18] = (segCB.state[SEG_18] & (~0x07)) | (value1);
	segCB.state[SEG_19] = (segCB.state[SEG_19] & (~0x0F)) | (value2);
			
	
}

// 菜单栏数字4显示
void LCD_DisplayMenuNum4(uint32 param)
{
	//C:0x01
	//G:0x02
	//B:0x04

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08
	
	uint8 value1 = 0,value2 = 0;

	value1 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;
	
	value2 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);

	segCB.state[SEG_16] = (segCB.state[SEG_16] & (~0x07)) | (value1);
	segCB.state[SEG_17] = (segCB.state[SEG_17] & (~0x0F)) | (value2);
	
}

// 菜单栏数字5显示
void LCD_DisplayMenuNum5(uint32 param)
{
	//C:0x01
	//G:0x02
	//B:0x04

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08
	
	uint8 value1 = 0,value2 = 0;

	value1 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;
	
	value2 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);
	
	segCB.state[SEG_14] = (segCB.state[SEG_14] & (~0x07)) | (value1);
	segCB.state[SEG_15] = (segCB.state[SEG_15] & (~0x0F)) | (value2);

}
// 菜单栏数字6显示
void LCD_DisplayMenuNum6(uint32 param)
{
	//C:0x01
	//G:0x02
	//B:0x04

	//D:0x01
	//E:0x02
	//F:0x04
	//A:0x08
	
	uint8 value1 = 0,value2 = 0;

	value1 = ((param >>2)& 0x01)|((param >> 4)& 0x02)|((param << 1)& 0x04) ;
	
	value2 = ((param >>3)& 0x01)|((param >> 3)& 0x02)|((param >> 4)& 0x04)|((param << 3)& 0x08);
	
	segCB.state[SEG_12] = (segCB.state[SEG_12] & (~0x07)) | (value1);
	segCB.state[SEG_13] = (segCB.state[SEG_13] & (~0x0F)) | (value2);
}

// 开机数字逐位显示，count 表示目前要显示前几位
void LCD_ShowBootDigits(uint8 count)
{
	uint16 blank[32] = {0};

	// 先清掉上一帧的段数据，但保持 LCD 显示使能
	LCD_DIS_AllState(blank);
	LCD_SetGroupOnOff(SEG_MASK_ALL, TRUE);

	// 第 1 位：显示数字 1
	if (count >= 1)
	{
		LCD_DisplaySpeedNum1(numberTable[1]);
	}
	if (count >= 2)
	{
		LCD_DisplaySpeedNum2(numberTable[2]);
	}

	if (count >= 3)
	{
		LCD_DisplaySpeedNum3(numberTable[3]);
	}
	if (count >= 4)
	{
		LCD_DisplayAssitNum(numberTable[4]);
	}
	if (count >= 5)
	{
		LCD_DisplayMenuNum1(numberTable[5]);
	}
	if (count >= 6)
	{
		LCD_DisplayMenuNum2(numberTable[6]);
	}
	if (count >= 7)
	{
		LCD_DisplayMenuNum3(numberTable[7]);
	}
	if (count >= 8)
	{
		LCD_DisplayMenuNum4(numberTable[8]);
	}
	if (count >= 9)
	{
		LCD_DisplayMenuNum5(numberTable[9]);
	}
	if (count >= 10)
	{
		LCD_DisplayMenuNum6(numberTable[0]);
	}
}

//  版本标题号
void LCD_SpeedNumDisplayStr(char *str)
{
	uint8 length = strlen(str);
	char  buff[3]={0};
	if((length) > 2 ||(length == 0))
	{
		return;
	}

	for(uint8 i =0;i<length;i++)
	{
		buff[i]=str[i];
	}
	
	// 优先清零
	LCD_DisplaySpeedNum1(0x00);
	LCD_DisplaySpeedNum2(0x00);
	LCD_DisplaySpeedNum3(0x00);

	switch(length)
	{
		case 1:
			LCD_DisplaySpeedNum2(asciiTable[buff[0]]);
			break;
			
		case 2:
		
			LCD_DisplaySpeedNum1(asciiTable[buff[0]]);
			LCD_DisplaySpeedNum2(asciiTable[buff[1]]);

			break;
	}

}

//  菜单栏显示字符
void LCD_MenuNumDisplayStr(char *str)
{

	uint8 length = strlen(str);
	char  buff[6]={0};
	if((length) > 6 ||(length == 0))
	{
		return;
	}

	for(uint8 i =0;i<length;i++)
	{
		buff[i]= str[i];
	}

	// 不显示小数点
	segCB.state[SEG_24] &= ~0x01;
	// 不显示双点
	segCB.state[SEG_22] &= ~0x01;

	// 显示清零
	LCD_DisplayMenuNum1(0x00);
	LCD_DisplayMenuNum2(0x00);
	LCD_DisplayMenuNum3(0x00);
	LCD_DisplayMenuNum4(0x00);
	LCD_DisplayMenuNum5(0x00);
	LCD_DisplayMenuNum6(0x00);
	
	// 刷新显示
	switch(length)
	{
		case 1:
			LCD_DisplayMenuNum6(asciiTable[buff[0]]);
			break;
			
		case 2:
			LCD_DisplayMenuNum5(asciiTable[buff[0]]);
			LCD_DisplayMenuNum6(asciiTable[buff[1]]);
			break;

		case 3:
			LCD_DisplayMenuNum4(asciiTable[buff[0]]);
			LCD_DisplayMenuNum5(asciiTable[buff[1]]);
			LCD_DisplayMenuNum6(asciiTable[buff[2]]);
			break;
			
		case 4:
			LCD_DisplayMenuNum3(asciiTable[buff[0]]);
			LCD_DisplayMenuNum4(asciiTable[buff[1]]);
			LCD_DisplayMenuNum5(asciiTable[buff[2]]);
			LCD_DisplayMenuNum6(asciiTable[buff[3]]);
			break;
			
		case 5:
			LCD_DisplayMenuNum2(asciiTable[buff[0]]);
			LCD_DisplayMenuNum3(asciiTable[buff[1]]);
			LCD_DisplayMenuNum4(asciiTable[buff[2]]);
			LCD_DisplayMenuNum5(asciiTable[buff[3]]);
			LCD_DisplayMenuNum6(asciiTable[buff[4]]);
			break;
		case 6:
			LCD_DisplayMenuNum1(asciiTable[buff[0]]);
			LCD_DisplayMenuNum2(asciiTable[buff[1]]);
			LCD_DisplayMenuNum3(asciiTable[buff[2]]);
			LCD_DisplayMenuNum4(asciiTable[buff[3]]);
			LCD_DisplayMenuNum5(asciiTable[buff[4]]);
			LCD_DisplayMenuNum6(asciiTable[buff[5]]);
			break;

	}
		
		
}

/**************************************************************************************
***************************************************************************************
**********************************骑行界面数据*****************************************
***************************************************************************************
***************************************************************************************/
// 错误界面
void LCD_DisplayErrorShow(uint32 param)
{
	uint8 num1,num10 = 0;
	
	if (param > 0xFF)
	{
		return;
	}
	
	num1 = (param & 0x0f);
	num10 = ((param >> 4) & 0x0f);

	// 屏蔽小数点
	segCB.state[SEG_9] &= (~0x01);
	
	// 错误标识
	segCB.state[SEG_5] |= 0x01;

	LCD_DisplaySpeedNum1(numberTable[num10]);
	LCD_DisplaySpeedNum2(numberTable[num1]);
	LCD_DisplaySpeedNum3(0x00);
	
}

// 速度界面
void LCD_DisplaySpeedShow(uint32 param)
{
	uint8 num1,num10,num100= 0;
	
	if (param >= 1000)
	{
		return;
	}

	// 清除错误标识
	segCB.state[SEG_5] &= (~0x01);
	
	// 显示小数点
	segCB.state[SEG_9] &= 0xfe;
    // 显示小数点
	segCB.state[SEG_9] |= 0x01;
	
	num1 = (param % 100% 10) & 0xff;
	num10 = (param % 100 / 10) & 0xff;
	num100 = (param / 100) & 0xff;
	
	LCD_DisplaySpeedNum1((num100 > 0 ? numberTable[num100] : 0x00));
	LCD_DisplaySpeedNum2(numberTable[num10]);
	LCD_DisplaySpeedNum3(numberTable[num1]);	

}


// 助力 等级[0-9]
void LCD_DisplayAssistLevel(uint32 param)
{
	uint8 assistLevel;

	assistLevel = param;
	
	segCB.state[SEG_25] |= 0x08;

	LCD_DisplayAssitNum((assistLevel == ASSIST_P ?  asciiTable['P']:numberTable[assistLevel])); 
}


// 速度单位显示
void LCD_DisplaySpeedUnitShow(uint32 param)
{
	switch ((UNIT_E)param)
	{
		case UNIT_METRIC:
			segCB.state[SEG_30] = ((segCB.state[SEG_30] &(~0x02)) | 0x00);
			segCB.state[SEG_11] = ((segCB.state[SEG_11] &(~0x01)) | 0x01);
			break;

		case UNIT_INCH:
			segCB.state[SEG_11] = ((segCB.state[SEG_11] &(~0x01)) | 0x00);
			segCB.state[SEG_30] = ((segCB.state[SEG_30] &(~0x02)) | 0x02);
			break;
		default:
			segCB.state[SEG_11] = ((segCB.state[SEG_11] &(~0x01)) | 0x00);
			segCB.state[SEG_30] = ((segCB.state[SEG_30] &(~0x02)) | 0x00);
			break;
	}
}

// 电量界面
void LCD_DisplayBatteryShow(uint32 param)
{
	uint8 localParam;

	localParam = param;

	if (100 < localParam)
	{
		localParam = 100;
	}

	// 找出电量格数
	// 0  < x <=  5  
	// 11 < x <=  20
	// 21 < x <=  40
	// 41 < x <=  60
	// 61 < x <=  80
	// 81 < x <=  100
	if (localParam <= 5)
	{
		segCB.state[SEG_30] = ((segCB.state[SEG_30] & (~0x0C)) | 0x04);
		segCB.state[SEG_31] = ((segCB.state[SEG_31] & (~0x0F)) | 0x00);
	}
	else if (localParam <= 20)
	{
		segCB.state[SEG_30] = ((segCB.state[SEG_30] & (~0x0C)) |0x04);
		segCB.state[SEG_31] = ((segCB.state[SEG_31] & (~0x0F)) |0x08);
	}
	else if (localParam <= 40)
	{
		segCB.state[SEG_30] = ((segCB.state[SEG_30] & (~0x0C)) |0x04);
		segCB.state[SEG_31] = ((segCB.state[SEG_31] & (~0x0F)) |0x09);
	}
	else if (localParam <= 60)
	{
		segCB.state[SEG_30] = ((segCB.state[SEG_30] & (~0x0C)) |0x04);
		segCB.state[SEG_31] = ((segCB.state[SEG_31] & (~0x0F)) |0x0B);
	}
	else if (localParam <= 80)
	{
		segCB.state[SEG_30] = ((segCB.state[SEG_30] & (~0x0C)) |0x04);
		segCB.state[SEG_31] = ((segCB.state[SEG_31] & (~0x0F)) |0x0F);
	}
	else
	{
		segCB.state[SEG_30] = ((segCB.state[SEG_30] & (~0x0C)) |0x0C);
		segCB.state[SEG_31] = ((segCB.state[SEG_31] & (~0x0F)) |0x0F);
	}

	// 低电量电池框闪烁
	if (localParam <= 5)
	{
		 LCD_SetGroupBlink(SEG_MASK_BATTERY_ICON, TRUE);
	}
	else
	{
		LCD_SetGroupBlink(SEG_MASK_BATTERY_ICON, FALSE);
	}
}


// 巡航图标显示
void LCD_DisplayCruisesShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_5] |= 0x02;
	}
	else
	{
		segCB.state[SEG_5] &= (~0x02);
	}
}


// 电量图标显示
void LCD_DisplayBatteryIcon(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_30] |= 0x04;
	}
	else
	{
		segCB.state[SEG_30] &= (~0x04);
	}
}

// 蓝牙图标显示
void LCD_DisplayBlueShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_7] |= 0x01;
	}
	else
	{
		segCB.state[SEG_7] &= (~0x01);
	}
}

// 推车助力图标显示
void LCD_DisplayAssistUp(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_5] |= 0x04;
	}
	else
	{
		segCB.state[SEG_5] &= (~0x04);
	}
}

// 大灯图标显示
void LCD_DisplayLight(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_5] |= 0x08;
	}
	else
	{
		segCB.state[SEG_5] &= (~0x08);
	}
}

// 错误码图标显示
void LCD_DisplayError(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_5] |= 0x01;
	}
	else
	{
		segCB.state[SEG_5] &= (~0x01);
	}
}

// 单次里程图标显示
void LCD_DisplayTripShow(uint32 param)
{
	if ((BOOL)param)
	{	
		segCB.state[SEG_24] |= 0x04;
	}
	else
	{
		segCB.state[SEG_24] &= (~0x04);
	}
}

// 总里程图标显示
void LCD_DisplayOdoShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_24] |= 0x08;
	}
	else
	{
		segCB.state[SEG_24] &= (~0x08);
	}
}

// 平均速度图标显示
void LCD_DisplayAvgSpeedShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_22] |= 0x08;
	}
	else
	{
		segCB.state[SEG_22] &= (~0x08);
	}
}

// 最大速度图标显示
void LCD_DisplayMaxSpeedShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_20] |= 0x08;
	}
	else
	{
		segCB.state[SEG_20] &= (~0x08);
	}
}

// 骑行时间图标显示
void LCD_DisplayTimeShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_24] |= 0x02;
	}
	else
	{
		segCB.state[SEG_24] &= (~0x02);
	}
}

// 功率图标显示
void LCD_DisplayPowShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_21] |= 0x08;
		segCB.state[SEG_20] |= 0x08;
	}
	else
	{
		segCB.state[SEG_21] &= (~0x08);
		segCB.state[SEG_20] &= (~0x08);
	}
}


// 公里图标显示 KM
void LCD_DIS_KmShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_14] |= 0x08;
	}
	else
	{
		segCB.state[SEG_14] &= (~0x08);
	}
}

// 英里图标显示 MILE
void LCD_DisplayMileShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_18] |= 0x08;
	}
	else
	{
		segCB.state[SEG_18] &= (~0x08);
	}
}

// MPH
void LCD_DisplayMphShow(uint32 param)
{
	if((BOOL)param)
	{	
		segCB.state[SEG_16] |= 0x08;
	}
	else
	{
		segCB.state[SEG_16] &= (~0x08);
	}
}

// KMPH
void LCD_DIS_KmphShow(uint32 param)
{

    if((BOOL)param)
	{	
		segCB.state[SEG_12] |= 0x08;
		segCB.state[SEG_14] |= 0x08;
	}
	else
	{
		segCB.state[SEG_12] &= (~0x08);
		segCB.state[SEG_14] &= (~0x08);
	}
	
		
	
}

// 功率显示
void LCD_DisplayPowerShow(uint32 param)
{
	uint8 eId = 0;
	
	uint32 ulMaxPower =PARAM_GetPowerMax();
	uint32 ulPower = param;
		
	// 限制最大功率
	LIMIT_TO_MAX(ulPower, ulMaxPower);

	eId = ((ulPower * 10) / ulMaxPower);

	// 限制最大功率显示
	LIMIT_TO_MAX(eId, 10);
	
	segCB.state[SEG_27] &= (~0x06);
	segCB.state[SEG_28] &= (~0x0F);
	segCB.state[SEG_29] &= (~0x0E);
	segCB.state[SEG_30] &= (~0x01);

	segCB.state[SEG_27] |= 0x08;

	switch (eId)
	{
		case 0:
			break;
		case 1:
			segCB.state[SEG_27] |= 0x04;
			break;
		case 2:
			segCB.state[SEG_27] |= 0x06;
			break;
		case 3:
			segCB.state[SEG_27] |= 0x06;
			segCB.state[SEG_28] |= 0x01;
			break;
		case 4:
			segCB.state[SEG_27] |= 0x06;
			segCB.state[SEG_28] |= 0x03;
			break;	
		case 5:
			segCB.state[SEG_27] |= 0x06;
			segCB.state[SEG_28] |= 0x07;
			break;
		case 6:
			segCB.state[SEG_27] |= 0x06;
			segCB.state[SEG_28] |= 0x0F;
			break;
		case 7:
			segCB.state[SEG_27] |= 0x06;
			segCB.state[SEG_28] |= 0x0F;
			segCB.state[SEG_29] |= 0x08;
			break;
		case 8:
			segCB.state[SEG_27] |= 0x06;
			segCB.state[SEG_28] |= 0x0F;
			segCB.state[SEG_29] |= 0x0C;
			break;
		case 9:
			segCB.state[SEG_27] |= 0x06;
			segCB.state[SEG_28] |= 0x0F;
			segCB.state[SEG_29] |= 0x0E;
			break;
		case 10:
			segCB.state[SEG_27] |= 0x06;
			segCB.state[SEG_28] |= 0x0F;
			segCB.state[SEG_29] |= 0x0E;
			segCB.state[SEG_30] |= 0x01;
			break;
		default:
			break;
	}
}

// 电量百分比显示 
void LCD_DisplayBattery_Percent_Show(uint32 param)
{
	uint8 num1,num2;

	uint8 localParam;

	localParam = param;

	if (100 < localParam)
	{
		localParam = 100;
	}

	//百分比符号
	segCB.state[SEG_35] |= 0x08;
	
	// 先清除显示
	LCD_DisplayBatperNum1(0X00);
	LCD_DisplayBatperNum2(0X00);

	// 屏蔽百分比 1
	segCB.state[SEG_33] &= (~0x08);

	num1 = (uint8)((localParam % 100 / 10) & 0xFF);
	num2 = (uint8)((localParam % 100 % 10) & 0xFF);

	if(localParam == 100)
	{		
	
		segCB.state[SEG_33] |= 0x08;//开百分比 1
		
		LCD_DisplayBatperNum1(numberTable[num1]);
		LCD_DisplayBatperNum2(numberTable[num2]);

	}	
	else if (localParam < 10)
	{
		LCD_DisplayBatperNum2(numberTable[num2]);
  	}
	else
	{
		LCD_DisplayBatperNum1(numberTable[num1]);
		LCD_DisplayBatperNum2(numberTable[num2]);
	}

}

// TRIP 显示
void LCD_DisplayRidingTrip(uint32 param)
{
	uint8 num1,num2,num3,num4,num5,num6;

	if (param >= 1000000)
	{
		param = 999999;
	}
	
	num1 = (uint8)((param / 100000) & 0xFF);
	num2 = (uint8)(((param % 100000) / 10000) & 0xFF);
  	num3 = (uint8)(((param % 10000) / 1000) & 0xFF);
  	num4 = (uint8)(((param % 1000) / 100) & 0xFF);
	num5 = (uint8)(((param % 100) / 10) & 0xFF);
	num6 = (uint8)((param % 10) & 0xFF);

	// 显示小数点
	segCB.state[SEG_29] |= 0x01;
	
	// 不显示双点
	segCB.state[SEG_27] &= ~0x01;

	LCD_DisplayMenuNum6(numberTable[num6]);
	LCD_DisplayMenuNum5(numberTable[num5]);
	LCD_DisplayMenuNum4((param > 99 ? numberTable[num4]:0x00));

	LCD_DisplayMenuNum3((param > 999 ? numberTable[num3]:0x00));
	LCD_DisplayMenuNum2((param > 9999 ? numberTable[num2]:0x00));
	LCD_DisplayMenuNum1((param > 99999 ? numberTable[num1]:0x00));
	
}

// ODO 显示
void LCD_DisplayRidingOdo(uint32 param)
{
	uint8 num1,num2,num3,num4,num5,num6;

	if (param >= 1000000)
	{
		param = 999999;
	}
	
	num1 = (uint8)((param / 100000) & 0xFF);
	num2 = (uint8)(((param % 100000) / 10000) & 0xFF);
    num3 = (uint8)(((param % 10000) / 1000) & 0xFF);
    num4 = (uint8)(((param % 1000) / 100) & 0xFF);
	num5 = (uint8)(((param % 100) / 10) & 0xFF);
	num6 = (uint8)((param % 10) & 0xFF);

	// 不显示小数点
	segCB.state[SEG_29] &= ~0x01;

	// 不显示双点
	segCB.state[SEG_27] &= ~0x01;

	LCD_DisplayMenuNum6(numberTable[num6]);
	LCD_DisplayMenuNum5((param > 9 ? numberTable[num5]:0x00));
	LCD_DisplayMenuNum4((param > 99 ? numberTable[num4]:0x00));

	LCD_DisplayMenuNum3((param > 999 ? numberTable[num3]:0x00));
	LCD_DisplayMenuNum2((param > 9999 ? numberTable[num2]:0x00));
	LCD_DisplayMenuNum1((param > 99999 ? numberTable[num1]:0x00));


}

// speed 显示 单位 0.1km
void LCD_AvgMaxSpeedShow(uint32 param)
{
	uint8 num4,num5,num6;

	// 速度不能大于100KM/H
	if (999 <= param)
	{
		param = 999;
	}

    num4 = (uint8)(((param % 1000) / 100) & 0xFF);
    num5 = (uint8)(((param % 100) / 10) & 0xFF);
	num6 = (uint8)((param % 10) & 0xFF);

	// 显示小数点
	segCB.state[SEG_29] |= 0x01;
	
	// 不显示双点
	segCB.state[SEG_27] &= ~0x01;

	LCD_DisplayMenuNum6(numberTable[num6]);
	LCD_DisplayMenuNum5(numberTable[num5]);
	LCD_DisplayMenuNum4((param > 99 ? numberTable[num4]:0x00));

	LCD_DisplayMenuNum3(0x00);
	LCD_DisplayMenuNum2(0x00);
	LCD_DisplayMenuNum1(0x00);

}

// 骑行时间显示
void LCD_RidTimeShow(uint32 param)
{
	uint8 num2,num3,num4,num1;
	uint32 localBuff = param;
	uint8 usHour, usMin, usSec;

	LIMIT_TO_MAX(localBuff,359940);

	// 不显示小数点
	segCB.state[SEG_29] &= ~0x01;

	// 显示双点
	segCB.state[SEG_27] |= 0x01;

	usSec = localBuff % 60;
	localBuff /= 60;
	usMin = localBuff % 60;
	usHour = localBuff / 60;

	usHour %= 100;
	usMin %= 100;
	usSec %= 100;
	
	// 提取数字
	num1 = usHour / 10;
	num2 = usHour % 10;
	num3 = usMin / 10;
	num4 = usMin % 10;

	LCD_DisplayMenuNum6(0x00);
	LCD_DisplayMenuNum5(0x00);
	
	LCD_DisplayMenuNum4(numberTable[num4]);
	LCD_DisplayMenuNum3(numberTable[num3]);
	LCD_DisplayMenuNum2(numberTable[num2]);
	LCD_DisplayMenuNum1(numberTable[num1]);
	
}

/**************************************************************************************
***************************************************************************************
***************************************************************************************
***************************************************************************************/
// 显示正常骑行界面 
void LCD_ShowRidingUI(BOOL refreshAll)
{	

	// 显示功率条
	LCD_DisplayPowerShow(789);

	// 显示电量图标
	LCD_DisplayBatteryIcon(0);

	// 显示实时电量值
	LCD_DisplayBatteryShow(PARAM_GetBatteryPercent());

	// 显示数字百分比电量
	LCD_DisplayBattery_Percent_Show(PARAM_GetBatteryPercent());

	// 显示大灯状态
	LCD_DisplayLight(PARAM_GetLightSwitch());

	// 显示蓝牙状态
	LCD_DisplayBlueShow(PARAM_GetBleConnectState());
	
	// 显示巡航开关
	LCD_DisplayCruisesShow(PARAM_GetCruiseState());
	
	// 显示档位
	LCD_DisplayAssistLevel(PARAM_GetAssistLevel());

	if( PARAM_GetErrorCode() == 0)
	{
		// 显示速度，单位为0.1，显示屏只能显示小数
		LCD_DisplaySpeedShow(PARAM_GetSpeed());

		// 0公制 1英制
		LCD_DisplaySpeedUnitShow(1);
		
		// 关闭错误标志
		LCD_DisplayError(FALSE);
	}
	else
	{
		// 关闭单位显示
		LCD_DisplaySpeedUnitShow(UINT_COUNT);
		
		// 显示错误标志
		LCD_DisplayError(TRUE);	
		
		// 显示错误码
		LCD_DisplayErrorShow(PARAM_GetErrorCode());
		
		// 重置速度相关变量
		paramCB.runtime.speedFilter.realSpeed = 0;
		paramCB.runtime.speedFilter.proSpeed = 0;
		paramCB.runtime.speedFilter.difSpeed = 0;
		
		PARAM_SetSpeed(0);
		
	}

	// 错误码状态
	if( paramCB.runtime.lastErrorType != paramCB.runtime.errorType)
	{
		paramCB.runtime.lastErrorType = paramCB.runtime.errorType;	
		
		// 存在错误码上报蓝牙
		if(PARAM_GetBleConnectState())
		{
			// 上报系统错误信息
			TIMER_AddTask(TIMER_ID_BLE_ERROR_INFO,
							1000,
							BLE_PROTOCOL_TxErrorInfo,
							(uint32)PARAM_GetErrorCode(),
							TIMER_LOOP_FOREVER,
							ACTION_MODE_ADD_TO_QUEUE);
		}

	}	

	// 先关闭所有显示
	LCD_DisplayTripShow(FALSE);
	LCD_DisplayOdoShow(FALSE); 
	LCD_DisplayAvgSpeedShow(FALSE);
	LCD_DisplayMaxSpeedShow(FALSE);
	LCD_DisplayTimeShow(FALSE);

	LCD_DIS_KmShow(FALSE);
	LCD_DisplayMileShow(FALSE);
	LCD_DIS_KmphShow(FALSE);
	LCD_DisplayMphShow(FALSE);
	
	switch (stateCB.dataMode)
	{
		// 显示单次旅程
		case RID_DATA_MODE_TRIP:

			LCD_DisplayTripShow(TRUE); 

			LCD_DisplayRidingTrip(PARAM_GetTrip());
		
			PARAM_GetUnit() == UNIT_METRIC ? LCD_DIS_KmShow(TRUE) :LCD_DisplayMileShow(TRUE);
			 
			break;
			
		// 显示总旅程 km
		case RID_DATA_MODE_ODO:

			LCD_DisplayOdoShow(TRUE); 
			
			LCD_DisplayRidingOdo(PARAM_GetTotalDistance() / 10);
		
			PARAM_GetUnit() == UNIT_METRIC ? LCD_DIS_KmShow(TRUE) :LCD_DisplayMileShow(TRUE);

			break;
			
		// 显示平均速度 0.1
		case RID_DATA_MODE_AVG:
		
			LCD_DisplayAvgSpeedShow(TRUE); 
			
			LCD_AvgMaxSpeedShow(PARAM_GetTripAverageSpeed());

			PARAM_GetUnit() == UNIT_METRIC ? LCD_DIS_KmphShow(TRUE):LCD_DisplayMphShow(TRUE);

			break;
			
		// 显示最大速度 0.1
		case RID_DATA_MODE_MAX_SPEED:
		
			
			LCD_DisplayMaxSpeedShow(TRUE); 	
			
			LCD_AvgMaxSpeedShow(PARAM_GetTripMaxSpeed());

			PARAM_GetUnit() == UNIT_METRIC ? LCD_DIS_KmphShow(TRUE):LCD_DisplayMphShow(TRUE);

			break;
			
		// 显示骑行时间
		case RID_DATA_MODE_TIME:
		
			LCD_DisplayTimeShow(TRUE); 	
			
			LCD_RidTimeShow(PARAM_GetTripRidingTime());			

			break;
		
		default:
			break;
	}
	
}

/**************************************************************************************
***************************************************************************************
***********************************菜单库设置******************************************
***************************************************************************************
***************************************************************************************/
// 自动关机OFF的时间显示
void LCD_DisplayAutoPowerOffTime(uint32 param)
 {
	uint8 num1,num10;
	
	if (param >= 100)
	{
		return;
	}
	
	// 不显示小数点
	segCB.state[SEG_9] &= (~0x01);

	num1 =	(param % 10) & 0xff;
	num10 = (param / 10 % 10) & 0xff;

	// 数值显示
	LCD_DisplaySpeedNum1(num10 > 0 ? numberTable[num10]: 0x00);
	LCD_DisplaySpeedNum2(numberTable[num1]);
	LCD_DisplaySpeedNum3(0x00);

}

// 背光亮度设置
void LCD_DisplayLightLevel(uint32 param)
{

	if (param >= 9)
	{
		param = 9;
	}
	
	// 不显示小数点
	segCB.state[SEG_9] &= (~0x01);

	// 数值显示
	LCD_DisplaySpeedNum1(0x00);
	LCD_DisplaySpeedNum2(numberTable[param]);
	LCD_DisplaySpeedNum3(0x00);

}


// 速度界面
void LCD_DisplayMenuLimitSpeed(uint32 param)
{
	uint8 num1,num10,num100= 0;
	
	if (param >= 1000)
	{
		return;
	}

	// 清除错误标识
	segCB.state[SEG_5] &= (~0x01);
	
	// 显示小数点
	segCB.state[SEG_9] &= 0xfe;
	
	num1 = (param % 100% 10) & 0xff;
	num10 = (param % 100 / 10) & 0xff;
	num100 = (param / 100) & 0xff;
	
	LCD_DisplaySpeedNum1((num100 > 0 ? numberTable[num100] : 0x00));
	LCD_DisplaySpeedNum2(numberTable[num10]);
	LCD_DisplaySpeedNum3(0x00); 

}



// 轮径
void LCD_DisplayMenuSize(uint32 wheelSize,uint32 wheelSizeId)
{
	uint8 num1=0,num10=0,num100= 0;
	
	LIMIT_TO_MAX(wheelSize,999);

	num1 = wheelSize % 100% 10;
	num10 = wheelSize % 100 / 10;
	num100 = wheelSize / 100;

	LCD_DisplaySpeedNum1(numberTable[num100]);
	LCD_DisplaySpeedNum2(numberTable[num10]);

	if( (PARAM_WHEEL_SIZE_700C == wheelSizeId) ||(PARAM_WHEEL_SIZE_27_5_INCH == wheelSizeId))
	{
		LCD_DisplaySpeedNum3(numberTable[num1]);	
	}
	else
	{
		LCD_DisplaySpeedNum3(0x00);
	}
	
}


// 周长
void LCD_DisplayMenuPerimeter(uint32 param)
{
	uint8 num6,num3,num4,num5;

	LIMIT_TO_MAX(param,9999);

	num3 = (param % 10000) / 1000;
    num4 = (param % 1000) / 100;
    num5 = (param % 100) / 10;
	num6 =  param % 10;

	// 刷新显示
	LCD_DisplayMenuNum1(0x00);
	LCD_DisplayMenuNum2(0x00);
	LCD_DisplayMenuNum3(numberTable[num3]);
	LCD_DisplayMenuNum4(numberTable[num4]);
	LCD_DisplayMenuNum5(numberTable[num5]);
	LCD_DisplayMenuNum6(numberTable[num6]);
}

// 电压等级
void LCD_DisplayMenuVoltage(uint32 param)
{

	uint8 num1=0,num10=0;

	LIMIT_TO_MAX(param,99);

	num1  = param  % 10;
	num10 = param  / 10;
	
	LCD_DisplaySpeedNum1(numberTable[num10]);
	LCD_DisplaySpeedNum2(numberTable[num1]);
	LCD_DisplaySpeedNum3(0x00);

}


// 欠压设置
void LCD_DisplayMenuVoltage_u(uint32 param)
{

	uint8 num1=0,num10=0,num100= 0;
	
	LIMIT_TO_MAX(param,999);
	
	// 显示小数点
	segCB.state[SEG_9] |= (~0xfe);

	num1 = param % 100% 10;
	num10 = param % 100 / 10;
	num100 = param / 100;

	LCD_DisplaySpeedNum1(numberTable[num100]);
	LCD_DisplaySpeedNum2(numberTable[num10]);
	LCD_DisplaySpeedNum3(numberTable[num1]);	


}


// 限流
void LCD_DisplayMenuLimitCur(uint32 param)
{

	uint8 num1=0,num10=0;

	LIMIT_TO_MAX(param,99);

	num1  = param  % 10;
	num10 = param  / 10;
	
	LCD_DisplaySpeedNum1((num10 > 0 ? numberTable[num10]:0x00));
	LCD_DisplaySpeedNum2(numberTable[num1]);
	LCD_DisplaySpeedNum3(0x00);

}

// 显示最大档位
void LCD_DisplayMenuMaxAssit(uint32 param)
{

	LIMIT_TO_MAX(param,9);
	
	LCD_DisplaySpeedNum1(numberTable[0]);
	LCD_DisplaySpeedNum2(numberTable[param]);
	LCD_DisplaySpeedNum3(0x00);
}

// 显示档位百分比
void LCD_DisplayMenuAssitPer(uint32 param)
{

	uint8 num1=0,num10=0,num100= 0;
	
	LIMIT_TO_MAX(param,9);

	num1 = stateCB.menu.root.pwmFixed[param] % 100% 10;
	num10 = stateCB.menu.root.pwmFixed[param] % 100 / 10;
	num100 = stateCB.menu.root.pwmFixed[param] / 100;

	// 显示档位
	LCD_DisplayAssitNum(numberTable[param]);

	LCD_DisplaySpeedNum1(numberTable[num100]);
	LCD_DisplaySpeedNum2(numberTable[num10]);
	LCD_DisplaySpeedNum3(numberTable[num1]);

}


// 测试磁钢
void LCD_DisplayMenuSteel(uint32 param)
{

	uint8 num1=0,num10=0;

	LIMIT_TO_MAX(param,99);

	num1  = param  % 10;
	num10 = param  / 10;
	
	LCD_DisplaySpeedNum1((num10 > 0 ? numberTable[num10] :0x00));
	LCD_DisplaySpeedNum2(numberTable[num1]);
	LCD_DisplaySpeedNum3(0x00);

}

// 菜单库显示
void LCD_DisplayMenuRoot(MENU_ID_E menuId)
{
	//速度不为0，返回骑行界面
	if (0 != PARAM_GetSpeed())
	{
		STATE_EnterState(STATE_RIDING);
		return;
	}

	/*********************************************
	 *************关闭相关显示********************
	 *********************************************/
	// 关闭速度显示使能
	LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, FALSE);
	 
	//速度区的数字闪烁关
	LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, FALSE);
	 
	//速度区的单位闪烁关
	LCD_SetGroupBlink(SEG_MASK_MPH, FALSE);
	LCD_SetGroupBlink(SEG_MASK_KMH, FALSE);
	 
	//速度单位显示关闭
	LCD_SetGroupOnOff(SEG_MASK_KMH, FALSE);
	LCD_SetGroupOnOff(SEG_MASK_MPH, FALSE);

	// 菜单栏数字单位闪烁关
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM1, FALSE);
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM2, FALSE);
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM3, FALSE);
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM4, FALSE);
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM5, FALSE);
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM6, FALSE);

	// 档位
	LCD_SetGroupOnOff(SEG_MASK_ASSIST_LEVEL, FALSE);

	// 关速度显示小数点
	segCB.state[SEG_9] &= (~0x01);

	// 关闭蓝牙ICON显示
	LCD_DisplayBlueShow(FALSE);

	/*********************************************
	 *********************************************/
	
	 // 显示相应的具体数值
    switch (menuId)
	{


		// 清除里程界面
		case MENU_ID_CLEARTRIP:
		
			//速度区的数字闪烁开
	        LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);
	        
	        // 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);

			// 显示CLEAR
		  	LCD_MenuNumDisplayStr("Clear");
			
		  	// 显示Y/N
		  	LCD_SpeedNumDisplayStr(stateCB.menu.root.clearState == FALSE ? "N":"Y");
  
			break;

	
        // 单位
		case MENU_ID_UNIT:
		
			// 如果单位是公制亮KMPH
			if(stateCB.menu.root.unit == UNIT_METRIC) 
			{
				//速度单位显示打开
	            LCD_SetGroupOnOff(SEG_MASK_KMH, TRUE);	
	            LCD_SetGroupBlink(SEG_MASK_KMH, stateCB.menu.root.blinkAble);
			}
			else
			{
				//速度单位显示打开
				LCD_SetGroupOnOff(SEG_MASK_MPH, TRUE);
				LCD_SetGroupBlink(SEG_MASK_MPH, stateCB.menu.root.blinkAble);	
			}
		
			// 显示Unit
			LCD_MenuNumDisplayStr("Unit");
			
			// 0公制 1英制
			LCD_DisplaySpeedUnitShow(stateCB.menu.root.unit);

			break;

        //限速
		case MENU_ID_SPEED_LIMIT:
		
			//速度单位显示打开
            LCD_SetGroupOnOff(SEG_MASK_KMH, TRUE);
			LCD_SetGroupOnOff(SEG_MASK_MPH, TRUE);

			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);

            //速度区的数字闪烁开
            LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);

			// 显示Speed
			LCD_MenuNumDisplayStr("Speed");

			// 0公制 1英制
			LCD_DisplaySpeedUnitShow(PARAM_GetUnit());
			
			// 显示限速的数值
		    LCD_DisplayMenuLimitSpeed(stateCB.menu.root.speedLimit);
			break;
			
        //自动关机时间
		case MENU_ID_AUTO_POWER_OFF_TIME:
		  	
   			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);    
			
            //速度区的数字闪烁开
            LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);

            // 显示自动关机名字OFF
			LCD_MenuNumDisplayStr("OFF");

		  	// 显示自动关机时间数值
			LCD_DisplayAutoPowerOffTime(stateCB.menu.root.powerTime);
			break;


		// 背光调节
		case MENU_ID_BACKLIGHT_BRIGHTNESS:

			//速度区的数字闪烁开
            LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);
	
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);

			// 背光调节设置light的名字显示
		  	LCD_MenuNumDisplayStr("Light");

		  	// 显示背光等级
			LCD_DisplayLightLevel(stateCB.menu.root.lightLevel);  
		  	
			break;

		// 蓝牙
		case MENU_ID_BLE_BROADCAST:
	
			// 未连接开始闪烁
			if (!PARAM_GetBleConnectState())
			{
				LCD_SetGroupBlink(SEG_MASK_BLE,TRUE);
                //速度区的数字闪烁开
                LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, TRUE);     
			}
			else
			{
				LCD_SetGroupBlink(SEG_MASK_BLE,FALSE);
                //速度区的数字闪烁开
                LCD_SetGroupBlink(SEG_MASK_SPEED_NUM,FALSE);
			}
			
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			
			// 显示蓝牙绑定设置名字Ble
		  	LCD_MenuNumDisplayStr("Ble");
        
		  	// 显示蓝牙连接yes和no
       		LCD_SpeedNumDisplayStr(PARAM_GetBleConnectState() == FALSE ? "N":"Y");
       		
            //蓝牙连图标
			LCD_DisplayBlueShow(TRUE);
			
			break;	
			
		// 仪表软件版本号
		case MENU_ID_HMI_FW_VER:
		
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			
			// 名称
			LCD_SpeedNumDisplayStr("FW");
		
			LCD_MenuNumDisplayStr(stateCB.menu.root.hmiFwVer);
	
			break;
		
		// 仪表硬件版本号	
		case MENU_ID_HMI_HW_VER:

		    // 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);

			// 名称
			LCD_SpeedNumDisplayStr("HW");
	
			LCD_MenuNumDisplayStr(stateCB.menu.root.hmiHwVer);
		
			break;
			
		// 仪表控制器版本号 			
		case MENU_ID_CF_VER:

		    // 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			
			// 名称
			LCD_SpeedNumDisplayStr("CF");
		
			LCD_MenuNumDisplayStr(stateCB.menu.root.controllerVer);
		
	
			break;
		
		
		case MENU_ID_BF_VER:
		
			// 打开速度显示使能
		 	LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);

			// 名称
			LCD_SpeedNumDisplayStr("8F");
		
			LCD_MenuNumDisplayStr(stateCB.menu.root.batVer);

			break;

		/********************************************************************************
		*********************************************************************************/
		// 轮径 
		case MENU_ID_SIZE:	
		
		    // 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);
			
			// 名称
			LCD_MenuNumDisplayStr("SIZE");

		    //显示轮径
			LCD_DisplayMenuSize(stateCB.menu.root.wheelSize,stateCB.menu.root.wheelSizeId);	
			break;

		// 周长
		case MENU_ID_CY:
		
		    // 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);

			// 打开菜单栏数字闪烁
			LCD_SetGroupBlink(SEG_MASK_SMALL_NUM3, stateCB.menu.root.blinkAble);
			LCD_SetGroupBlink(SEG_MASK_SMALL_NUM4, stateCB.menu.root.blinkAble);
			LCD_SetGroupBlink(SEG_MASK_SMALL_NUM5, stateCB.menu.root.blinkAble);
			LCD_SetGroupBlink(SEG_MASK_SMALL_NUM6, stateCB.menu.root.blinkAble);		
			
			// 名称
			LCD_SpeedNumDisplayStr("CY");

            //显示周长
			LCD_DisplayMenuPerimeter(stateCB.menu.root.perimeter);
			break;
			
		// 额定电压	
		case MENU_ID_BAT: 	
		
		    // 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);

			// 名称
			LCD_MenuNumDisplayStr("Bat");	

			// 显示电压具体数值
			LCD_DisplayMenuVoltage(stateCB.menu.root.voltage);	

			break;

		// 欠压
		case MENU_ID_BAT_U:
		
		    // 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);
			// 名称
			LCD_MenuNumDisplayStr("Bat_u");	
	
			// 显示欠压
			LCD_DisplayMenuVoltage_u(stateCB.menu.root.lowVoltage);	

			break;

		// 限流
		case MENU_ID_EU_A:
		
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);

			// 名称
			LCD_MenuNumDisplayStr("EU_L");	

			// 显示限流
			LCD_DisplayMenuLimitCur(stateCB.menu.root.currentLimit);	

			break;
			
		// 最大档位	
		case MENU_ID_RANGE:
		
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);

			// 名称
			LCD_MenuNumDisplayStr("Range");	

			// 显示最大档位
			LCD_DisplayMenuMaxAssit(stateCB.menu.root.maxAssit);	

			break;
			
		// 每档助力百分比
		case MENU_ID_LEVEL_PER:
		
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);
			
			// 档位
			LCD_SetGroupOnOff(SEG_MASK_ASSIST_LEVEL,TRUE);

			// 名称
			LCD_MenuNumDisplayStr("LEVEL");	

			// 显示档位
			LCD_DisplayMenuAssitPer(stateCB.menu.root.options);	

			break;
			
		// 测试磁钢			
		case MENU_ID_STEEL:
		
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);
			
			// 名称
			LCD_MenuNumDisplayStr("Steel");	

			// 显示测试磁钢
			LCD_DisplayMenuSteel(stateCB.menu.root.speedSensor);	

			break;
			
		// 定速巡航	
		case MENU_ID_CRUIS:
		
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);
			
			// 名称
			LCD_MenuNumDisplayStr("Cruis");	

			// 显示定速巡航	
       		LCD_SpeedNumDisplayStr(stateCB.menu.root.cruiseSwitch == FALSE ? "N":"Y");

			break;
			
		// 恢复出厂		
		case MENU_ID_RESET:		
		
			// 打开速度显示使能
			LCD_SetGroupOnOff(SEG_MASK_SPEED_NUM, TRUE);
			LCD_SetGroupBlink(SEG_MASK_SPEED_NUM, stateCB.menu.root.blinkAble);
			
			// 名称
			LCD_MenuNumDisplayStr("Reset");	

			// 恢复出厂	
       		LCD_SpeedNumDisplayStr(stateCB.menu.root.reset == FALSE ? "N":"Y");

			break;
			
		/*********************************************************
		***************************预留新增内容*******************
		*********************************************************/	

		default:

			break;

	}

}

/**************************************************************************************
***************************************************************************************
*********************************高级菜单密码******************************************
***************************************************************************************
***************************************************************************************/
// 高级菜单密码的显示
void LCD_DisplayAdvMenuPasswordNum(uint32 param)
{
	uint8 num3=0,num4=0,num5=0,num6=0;

	num3 = (uint8)(((param % 10000) / 1000) & 0xFF);
    num4 = (uint8)(((param % 1000) / 100) & 0xFF);
    num5 = (uint8)(((param % 100) / 10) & 0xFF);
	num6 = (uint8)((param % 10) & 0xFF);

	LCD_DisplayMenuNum1(0x00);
	LCD_DisplayMenuNum2(0x00);
	LCD_DisplayMenuNum3(numberTable[num3]);
	LCD_DisplayMenuNum4(numberTable[num4]);
	LCD_DisplayMenuNum5(numberTable[num5]);
	LCD_DisplayMenuNum6(numberTable[num6]);

	
}

// 高级菜单密码界面
void LCD_DisplaymenuPassword(BOOL refreshAll)
{

	// 菜单密码的名字AP
	LCD_SpeedNumDisplayStr("AP");
		
	// 显示菜单密码具体数值
	 LCD_DisplayAdvMenuPasswordNum(stateCB.menu.passWord.numberParam);

	// 关闭闪烁
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM3,FALSE);
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM4,FALSE);
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM5,FALSE);
	LCD_SetGroupBlink(SEG_MASK_SMALL_NUM6,FALSE);
	
	// 密码位闪烁
	if (stateCB.menu.passWord.options == 0)
	{
		LCD_SetGroupBlink(SEG_MASK_SMALL_NUM3,stateCB.menu.root.blinkAble);
	}
	if (stateCB.menu.passWord.options == 1)
	{
		LCD_SetGroupBlink(SEG_MASK_SMALL_NUM4,stateCB.menu.root.blinkAble);
	}
	if (stateCB.menu.passWord.options == 2)
	{	
		LCD_SetGroupBlink(SEG_MASK_SMALL_NUM5,stateCB.menu.root.blinkAble);
	}
	if (stateCB.menu.passWord.options == 3)
	{
		LCD_SetGroupBlink(SEG_MASK_SMALL_NUM6,stateCB.menu.root.blinkAble);
	}

}





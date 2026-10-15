#include "common.h"
#include "system.h"
#include "delay.h"
#include "iap.h"
#include "timer.h"
#include "uartdrive.h"
#include "dtconfig.h"
#include "adc.h"
#include "key.h"
#include "keyMsgProcess.h"
#include "powerctl.h"
#include "adc.h"
#include "backlight.h"
#include "watchDog.h"
#include "param.h"
#include "state.h"
#include "LCD.h"
#include "BleUart.h"
#include "BleProtocol.h"
#include "ds1302.h"
#include "record.h"
#include "lcdTest.h"

int main(void)
{	
	// 123
	// 关闭中断
	DI();

	// 初始化系统时钟
	SYSTEM_Init();
	
	// 上电控制
	POWER_Init();
	  
	// 定时器初始化
	TIMER_Init();
	
	// IAP初始化
	IAP_Init();
	
	// 参数模块初始化
	PARAM_Init();
	
	// 协议层初始化
	PARAM_ProtocolInitSelect();

 	//亮度PWM
  	BACK_LIGHT_Init();

	// 按键初始化 
	KEY_Init();

	// AD模块初始化
	ADC_BaseInit();
  
 	// 骑行信息模块初始化
	RECORD_Init(); 
  
    
	// 状态机初始化
	STATE_Init();
	
#if	CONFIG_SYSTEM_EXISTBLE 

	// 蓝牙底层驱动初始化
	BLE_UART_Init();

	// 蓝牙协议层初始化
	BLE_PROTOCOL_Init();
	
	// DS1302初始化
	DS1302_Init();
	

	
#endif	

	// LCD初始化
	SEGMENT_Init();
			
	EI();

	while (1)
	{
	
		// 驱动层过程处理
		UART_DRIVE_Process();
		
		// 协议层过程处理
		PARAM_ProtocolProcessSelect();
		
		// 定时器任务队列处理
		TIMER_TaskQueueProcess();
		
		// 按键过程处理
		KEYMSG_Process();
		
		// 状态机模块过程处理
		STATE_Process();

		// LCD过程处理
		LCD_Process();

		// ADC过程处理
		ADC_Process();
        
        // 参数模块过程处理
		PARAM_Process();
		
		// 骑行信息模块过程处理
		RECORD_Process();	
		

#if	CONFIG_SYSTEM_EXISTBLE 

		// 蓝牙通讯驱动层过程处理
		BLE_UART_Process();

		// 蓝牙通讯模块处理
		BLE_PROTOCOL_Process();

		// DS1302过程处理
		DS1302_Process();


		
#endif	
	
	}
}



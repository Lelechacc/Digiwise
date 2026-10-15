#ifndef __TIMER_H__
#define __TIMER_H__

#include "common.h"

// TIMx选择
#define N32_TIMERx_NUM_DEF 								TIM6
#define N32_TIMERx_IRQn_DEF								TIM6_IRQn

#define TIMER_TIME										5	// 软件定时器周期，即软件定时器最小时间分辨率，单位：ms
#define TIMER_LOOP_FOREVER								-1	// 无限循环

#define TIMER_TASK_QUEUE_SIZE							TIMER_ID_MAX	// 定时器后期处理任务队列

typedef enum
{
	TIMER_ID_NULL = -1,

	TIMER_ID_STATE_CONTROL,						// 状态机模块控制

	TIMER_ID_MENU_QUIT,							// 退出菜单控制
	
	TIMER_ID_FUNCTION_TEST,						// 老化测试界面

	TIMER_ID_AGE_SEND,							// 老化命令发送定时器
	
	TIMER_ID_PROTOCOL_PARAM_TX,					// 协议层命令定时下发

	TIMER_ID_PROTOCOL_TXRX_TIME_OUT,			// 协议层通讯超时
	
	TIMER_ID_PROTOCOL_SPEED_FILTER,				// 速度平滑
	
	TIMER_ID_PARAM_CACULATETRIPDISTANCE,		// 定时计算里程

	TIMER_ID_KEY_SCAN,							// 按键扫描

	TIMER_ID_AUTO_POWER_OFF,					// 自动关机

	TIMER_ID_ADC_SCAN,							// AD转换间隔

	TIMER_ID_BLE_CONTROL,						// 查询蓝牙MAC地址
	
	TIMER_ID_LCD_BLINK,							// 显示屏闪烁
	
	TIMER_ID_LCD_REFRESH,						// 显示屏刷新

	TIMER_ID_RECOVERY_KEY_UP_ADD_DEC,			// 恢复+-键弹起功能

	TIMER_ID_RECOVERY_KEY_UP_ADD_INFO,			// 恢复+i键弹起功能

	// --------------------------------------------------------------------------
	// 2026-10-14 17:29 CST | i + DEC release recovery and full-on auto return
	TIMER_ID_RECOVERY_KEY_UP_DEC_POWER,
	TIMER_ID_FULL_ON_TEST_RETURN,
	// --------------------------------------------------------------------------

	TIMER_ID_PARAM_TIME,						// 参数层时钟

	TIMTR_ID_NVM_WRITE_REQUEST,					// NVM写数据请求
	
	TIMER_ID_BLE_START_HISTORICAL_DATA_UP,		// 蓝牙启动历史数据同步
	
	TIMER_ID_RECORD_COUNT,						// 未授时骑行记录定时器
	
	TIMER_ID_RECORD_CONTROL,					// 骑行记录
	
	TIMER_ID_RTC_READ,							// RTC时钟
	
	TIMER_ID_BLE_ERROR_INFO,					// 蓝牙上报系统错误消息
	
	TIMER_ID_BLE_CONNECT_VERIFICATE,			// 蓝牙连接权鉴验证
	
	TIMER_ID_BLE_DISCONNECT,					// 蓝牙权鉴失败断开连接

	TIMER_ID_BLE_CONNECT_STATE,

	TIMER_ID_UART_RX_TIME_OUT_CONTROL,			// 大石协议V9.2需要开启接收超时定时器

	TIMER_ID_BLE_ADV_RESET,							// 复位广播包数据


	TIMER_ID_BlinkBack,

	TIMER_ID_CLEARTRIP_BACK,
	TIMER_ID_BOOT_DIGIT, // 开机数字逐位动画
	TIMER_ID_MAX
}TIMER_ID;

typedef enum
{
	ACTION_MODE_DO_AT_ONCE = 0,	// 立即执行
	ACTION_MODE_ADD_TO_QUEUE	// 添加到任务队列中执行
}ACTION_MODE_E;

// 定时器任务
typedef struct{
	void(*action)(uint32 param);
	uint32	param;
	TIMER_ID	id;
}TIMER_TASK;

typedef struct
{
	uint32	top[TIMER_ID_MAX];
	volatile uint32  count[TIMER_ID_MAX];
	void(*action[TIMER_ID_MAX])(uint32 param);
	uint32	param[TIMER_ID_MAX];
	int32  loopCount[TIMER_ID_MAX];
	uint8  actionMode[TIMER_ID_MAX];		// 执行模式
	BOOL active[TIMER_ID_MAX];				// 是否处于运行状态

	// 定时器任务队列，当定时器任务执行时间较长时，必须将其放到中断以外执行，即将其放入任务队列中执行
	struct{
		TIMER_TASK task[TIMER_TASK_QUEUE_SIZE];
		uint8 head;
		uint8 end;
	}queue;
}TIMER_CB;


/***************************** 函数外部申明*******************************/
// 定时器模块初始化
void TIMER_Init(void);

// 添加定时器任务
BOOL TIMER_AddTask(TIMER_ID id, uint32 time, void(*action)(uint32 param), uint32 param, int32 actionTime, ACTION_MODE_E actionMode);

// 销毁定时器任务
void TIMER_KillTask(TIMER_ID id);

// 向定时器任务队列中添加一个任务
BOOL TIMER_AddToQueue(void(*action)(uint32 param), uint32 param , TIMER_ID id);

// 定时器任务队列处理，在main中调用
void TIMER_TaskQueueProcess(void);

// 定时器回调函数，在硬件定时器中断中使用
void TIMER_CALLBACK_OnTimer(void);

// 暂停指定的定时器
void TIMER_Pause(TIMER_ID id);

// 恢复指定的定时器
void TIMER_Resume(TIMER_ID id);

// 注销所有定时器
void TIMER_KillAllTimer(void);

// 复位定时器，重新开始
void TIMER_ResetTimer(TIMER_ID id);

// 改变指定定时器定时时间
BOOL TIMER_ChangeTime(TIMER_ID id, uint32 time);

#endif


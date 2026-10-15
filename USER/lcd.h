#ifndef __LCD_H__
#define __LCD_H__

#include "common.h"
#include "dtconfig.h"

// 显示屏显示所需数字
typedef enum
{
	SEG_5 = 0,
	SEG_6,
	SEG_7,
	SEG_8,
	SEG_9,
	SEG_10,
	SEG_11,
	SEG_12,
	SEG_13,
	SEG_14,
	SEG_15,
	SEG_16,
	SEG_17,
	SEG_18,
	SEG_19,
	SEG_20,
	SEG_21,
	SEG_22,
	SEG_23,
	SEG_24,
	SEG_25,
	SEG_26,
	SEG_27,
	SEG_28,
	SEG_29,
	SEG_30,
	SEG_31,
	SEG_32,
	SEG_33,	
	SEG_34,
	SEG_35,	
	
	SEG_MAX
}SEG_E;

// 结构体中每个数字对应原理图中的名称
typedef enum
{
	COM_0 = 0,	// 数字段1
	COM_1,		// 数字段2
	COM_2,		// 标识段3
	COM_3,		// 标识段4
	
	COM_MAX
}COM_E;

// 组合名称
typedef enum
{
	SEG_COMPONENT_NUM_1 = 0,
	SEG_COMPONENT_NUM_10,

}SEG_COMPONENT_E;

// 组合名称
typedef enum
{
	SEG_BIT_SPEED_NUM = 0,
//	SEG_BIT_SPEED_MARK,
	SEG_BIT_MPH,
	SEG_BIT_KMH,
	SEG_BIT_BLE,
	SEG_BIT_BATTERY_ICON,
	SEG_BIT_BATTERY_QUANTITY,	
	SEG_BIT_ERROR,
	SEG_BIT_LIGHT,
	SEG_BIT_CRUISE,
	SEG_BIT_ASSIST_UP,
	SEG_BIT_ASSIST_LEVEL,
	SEG_BIT_POWER,
	SEG_BIT_TRIP,
	SEG_BIT_ODO,
	SEG_BIT_SPEED_AVG,
	SEG_BIT_SPEED_MAX,
	SEG_BIT_TIME,
//	SEG_BIT_KCAL,
	SEG_BIT_RANGE,
	SEG_BIT_KM,
	SEG_BIT_MILE,
	SEG_BIT_SMALL_NUM1,
	SEG_BIT_SMALL_NUM2,
	SEG_BIT_SMALL_NUM3,
	SEG_BIT_SMALL_NUM4,
	SEG_BIT_SMALL_NUM5,
	SEG_BIT_SMALL_NUM6,
	SEG_BIT_BAT_PERCENT,

	SEG_BIT_ALL
}SEG_BIT_E;

typedef enum
{
	SEG_MASK_SPEED_NUM  			= 1<<SEG_BIT_SPEED_NUM,

	//SEG_MASK_SPEED_MARK  		= 1<<SEG_BIT_SPEED_MARK,
	SEG_MASK_MPH  				= 1<<SEG_BIT_MPH,
	SEG_MASK_KMH  				= 1<<SEG_BIT_KMH,
	SEG_MASK_BLE 				= 1<<SEG_BIT_BLE,
	SEG_MASK_BATTERY_ICON 		= 1<<SEG_BIT_BATTERY_ICON,
	SEG_MASK_BATTERY_QUANTITY	= 1<<SEG_BIT_BATTERY_QUANTITY,	
	SEG_MASK_ERROR				= 1<<SEG_BIT_ERROR,
	SEG_MASK_LIGHT				= 1<<SEG_BIT_LIGHT,
	SEG_MASK_CRUISE        		= 1<<SEG_BIT_CRUISE,
	SEG_MASK_ASSIST_UP			= 1<<SEG_BIT_ASSIST_UP,
	SEG_MASK_ASSIST_LEVEL		= 1<<SEG_BIT_ASSIST_LEVEL,
	SEG_MASK_POWER				= 1<<SEG_BIT_POWER,
	SEG_MASK_TRIP				= 1<<SEG_BIT_TRIP,
	SEG_MASK_ODO				= 1<<SEG_BIT_ODO,
	SEG_MASK_SPEED_AVG			= 1<<SEG_BIT_SPEED_AVG,
	SEG_MASK_SPEED_MAX			= 1<<SEG_BIT_SPEED_MAX,
	SEG_MASK_TIME				= 1<<SEG_BIT_TIME,
	//SEG_MASK_KCAL				= 1<<SEG_BIT_KCAL,
	SEG_MASK_RANGE				= 1<<SEG_BIT_RANGE,
	SEG_MASK_KM					= 1<<SEG_BIT_KM,
	SEG_MASK_MILE				= 1<<SEG_BIT_MILE,
	SEG_MASK_SMALL_NUM1			= 1<<SEG_BIT_SMALL_NUM1,
	SEG_MASK_SMALL_NUM2			= 1<<SEG_BIT_SMALL_NUM2,
	SEG_MASK_SMALL_NUM3			= 1<<SEG_BIT_SMALL_NUM3,
	SEG_MASK_SMALL_NUM4			= 1<<SEG_BIT_SMALL_NUM4,
	SEG_MASK_SMALL_NUM5			= 1<<SEG_BIT_SMALL_NUM5,
	SEG_MASK_SMALL_NUM6			= 1<<SEG_BIT_SMALL_NUM6,
	SEG_MASK_BAT_PERCENT        = 1<<SEG_BIT_BAT_PERCENT,

	
	SEG_MASK_ALL 				= 1<<SEG_BIT_ALL,
}SEG_MASK_E;

// 显示函数项枚举定义
typedef enum
{
	DISPLAY_ALL,				// 显示屏全部显示
	DISPLAY_LIGHT,				// 大灯
	DISPLAY_LOCK, 				// 锁车标识
	DISPLAY_BLE,				// 蓝牙
	DISPLAY_NUM,				// 显示屏数字显示
	DISPLAY_MPH,				// 英制速度 MPH 
	DISPLAY_KMH,				// 公制速度 KMH 
	DISPLAY_ASSIST_UP,			// 档位图标
	DISPLAY_ASSIST_LEVEL,		// 档位等级
	DISPLAY_BATTERY_FRAME,		// 电池框
	DISPLAY_BATTERY_QUANTITY,	// 电量容量

	DISPLAY_MAX
}DISPLAY_ITEM_E;

typedef struct
{
	// 数值表
	volatile uint8 state[SEG_MAX];			// 段数值
	volatile uint8 onoff[SEG_MAX];			// 开关掩码
	volatile uint8 blinkEnable[SEG_MAX];		// 闪烁掩码，在刷新时，将显示屏的数值与该值相异或后输出
	volatile uint8 blinkPhase;

	volatile uint8 outputSeg[SEG_MAX];			// 段数值
	
	volatile uint32 mapData[COM_MAX];			// 段数值

	volatile uint8 refreshRequest;				// 刷新标志

	volatile uint8 currentScanCom;				// 当前被扫描的列，取值范围0-12，代表COM1-COM13

	uint8 RefreshDuty;							// 亮度占空比调节
}SEGMENT_CB;

extern SEGMENT_CB segCB;

// 显示屏初始化
void SEGMENT_Init(void);

// 显示屏过程处理
void LCD_Process(void);

// 单元测试函数
void LCD_UnitTest(void);

// 设置总开关
void LCD_SetGroupOnOff(SEG_MASK_E segMask, BOOL onoff);

// 显示屏全部显示
void LCD_DIS_AllState(uint16 segBuff[]);

// 骑行速度显示
void LCD_AvgMaxSpeedShow(uint32 param);

void LCD_RidTimeShow(uint32 param);

// KMPH
void LCD_DIS_KmphShow(uint32 param);

// MPH
void LCD_DisplayMphShow(uint32 param);

// 骑行时间显示
void LCD_DisplayRingTime(uint32 param);

// 清除里程的状态
void LCD_DisplayClearState(uint32 param);

// 蓝牙图标显示
void LCD_DisplayBlueShow(uint32 param);

// 电量界面
void LCD_DisplayBatteryShow(uint32 param);

// 巡航图标显示
void LCD_DisplayCruisesShow(uint32 param);

// 电量图标显示
void LCD_DisplayBatteryIcon(uint32 param);

// 助力 等级[0-9]
void LCD_DisplayAssistLevel(uint32 param);

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


// 推车助力图标显示
void LCD_DisplayAssistUp(uint32 param);

// 蓝牙绑定设置Ble的名字显示
void LCD_DisplayBlueBindName(uint32 param);

// 速度单位显示
void LCD_DisplaySpeedUnitShow(uint32 param);

// 设置闪烁
void LCD_SetGroupBlink(SEG_MASK_E segMask, BOOL onoff);

void LCD_DisplayMenuLimitSpeed(uint32 param);

// 单元测试函数
void LCD_UnitTest(void);

// 开机数字逐位显示：count 为当前显示的位数，范围 1～10
void LCD_ShowBootDigits(uint8 count);

// ---------------- 2026-10-15 15:32:37 reverse digit animation ----------------
void LCD_ShowReverseDigits(uint8 count);
// --------------------------------------------------------------------------

void LCD_SpeedNumDisplayStr(char *str);

void LCD_MenuNumDisplayStr(char *str);

// 高级菜单密码界面
void LCD_DisplaymenuPassword(BOOL refreshAll);


// 高级菜单界面
void LCD_DisplayAdvMenuRoot(BOOL refreshAll);


void LCD_DisplayMenuRoot(MENU_ID_E menuId);


#endif




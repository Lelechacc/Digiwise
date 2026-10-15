#ifndef __STATE_H__
#define __STATE_H__				

#include "common.h"
#include "Param.h"
#include "dtconfig.h"

#define TIME_WAIT_AFTER_OK							1000ul		// 设置成功后、返回上级菜单前的等待时间
#define TIME_WAIT_POWER_ON_OFF						1000ul		// 开关机的等待时间
#define TIME_WAIT_MENU_AUTO_RETURN_RIDING			60000ul		// 普通菜单界面和高级菜单界面自动返回骑行界面的等待时间



// 骑行界面
typedef enum
{
	RID_DATA_MODE_TRIP = 0,			// 单次里程		
	RID_DATA_MODE_ODO,				// 总里程
	RID_DATA_MODE_AVG,				// 平均速度
	RID_DATA_MODE_MAX_SPEED,			// 最大速度
	RID_DATA_MODE_TIME,				// 骑行时间
	
	RID_DATA_MODE_MAX		
}RID_DATA_MODE_E;



// 系统状态定义
typedef enum
{
	STATE_NULL = 0,								// 掉电状态

	STATE_ENTRY,								// 入口状态
	
	STATE_LOAD_NVM,								// 参数加载状态
	
	STATE_SHOW_POWER_ON,						// 开机界面状态
	
	STATE_POWER_KEY_KEYUP,						// 开机长按状态

	STATE_RIDING,								// 骑行正常界面状态

	STATE_MENU_ROOT,							// 菜单界面

	STATE_ADVMENU_PASSWORD,						// 高级菜单密码界面

	STATE_ADV_MENU_ROOT,						// 高级菜单界面

	STATE_SHOW_POWER_OFF,						// 显示关机界面状态

	STATE_POWER_OFF,							// 关机断电

	STATE_AGING_TEST,							// 老化测试状态

	STATE_FUCTION_TEST,							// 功能测试

	STATE_BOOT_DIGIT,							// 开机数字显示状态
	// ---------------- 2026-10-15 15:32:37 reverse digit animation ----------------
	STATE_REVERSE_DIGIT, // Key-triggered reverse digit animation
	// --------------------------------------------------------------------------
	// ---------------- 2026-10-15 15:41:18 restore full-on key test ----------------
	STATE_FULL_ON_TEST,
	// --------------------------------------------------------------------------

	STATE_MAX,									// 状态数


	STATE_SERVICE_LIGHT_ON_OFF,                // 长按电源键和减键开灯
}STATE_E;

typedef struct
{	

	STATE_E state;					// 当前系统状态
	STATE_E preState;				// 上一个状态


	uint8  powerOnKeyUpFlag;
	uint16 powerOnKeyLongHoldTime;
	 
	RID_DATA_MODE_E  dataMode; 			// 骑行界面六个信息的选择

	// 菜单参数
	struct 
	{
		// 菜单密码
		struct
		{
			uint8 optionsOkFlag;
			uint8 options;
			uint8 numberOne;
			uint8 numberTwo;
			uint8 numberThree;
			uint8 numberFour;
			uint32 numberParam;
			BOOL  powerOnFrist;

		}passWord;


		struct
		{
			MENU_ID_E menuId;

			BOOL blinkAble;
			BOOL clearState;

			UNIT_E unit;
			uint16 speedLimit;
			uint16 powerTime;
			uint8  powerTimeIndex;
			uint8  lightLevel;
			uint8  lightLevelIndex;

			char  hmiFwVer[7];
			char  hmiHwVer[7];
			char  controllerVer[7];
			char  batVer[7];
			
			PARAM_WHEEL_SIZE_ID  wheelSizeId;
			uint8  wheelSizeIdIndex;
			uint16 wheelSize;
			uint16 perimeter;

			uint8 voltage;
			uint16 lowVoltage;
			uint8 voltageIndex;

			uint16 currentLimit;
			
			uint8 maxAssit;
			uint8 maxAssitIndex;
			uint8 options;
			uint8 pwmFixed[ASSIST_ID_COUNT];

			uint8 speedSensor;
			BOOL  cruiseSwitch;
		
			BOOL  reset;
			
		}root;
	
	}menu;
	
}STATE_CB;

extern STATE_CB stateCB;



// 状态机初始化
void STATE_Init(void);

// 重启自动关机计时
void STATE_ResetAutoPowerOffControl(void);

// 状态迁移
void STATE_EnterState(uint32 state);

// 状态机处理
void STATE_Process(void);

// 档位切换
void STATE_Service_AssistLevelChange(uint32 param);

// 推车助力
void STATE_Service_AssistPushOnOff(uint32 onoff);

// 显示正常骑行界面 
void LCD_ShowRidingUI(BOOL refreshAll);

// 骑行界面信息切换
void STATE_Service_ChangeDataDisplayMode(uint32 param);

// 菜单信息选择
void STATE_MenuChange(uint32 param);

// 确认要选择的菜单
void STATE_MenuConfirm(uint32 param);

// 进入菜单页面
void STATE_Service_EnterMenu(uint32 param);

#endif


#ifndef __DTCONFIG_H__
#define __DTCONFIG_H__

#include "Param.h"
#include "common.h"
#include "backlight.h"

/*****************************************************************************
******************************************************************************
************************************系统配置**********************************
******************************************************************************
******************************************************************************/
// 系统是否存在蓝牙
#define CONFIG_SYSTEM_EXISTBLE                0        



/*****************************************************************************
******************************************************************************
************************************协议配置**********************************
******************************************************************************
******************************************************************************/
// 协议类型
#define   CONFIG_PROTOCOL_TYPE_KM5S            1
#define   CONFIG_PROTOCOL_TYPE_LBNO_2          0
#define   CONFIG_PROTOCOL_TYPE_BF              0
#define   CONFIG_PROTOCOL_TYPE_J               0

/********************新增***********************
************************************************/

// H文件宏定义
#if  CONFIG_PROTOCOL_TYPE_KM5S
	#include "uartProtocol.h"

#elif CONFIG_PROTOCOL_TYPE_LBNO_2
	#include "uartProtocol.h"
	#include "uartProtocol1.h"

#elif CONFIG_PROTOCOL_TYPE_BF 
	#include "uartProtocol.h"
	#include "uartProtocol2.h"

#elif CONFIG_PROTOCOL_TYPE_J
	#include "uartProtocol.h"
	#include "uartProtocol3.h"	

/*****************新增文件宏定义****************/ 
#endif


/*****************************************************************************
******************************************************************************
************************************菜单库************************************
******************************************************************************
******************************************************************************/
// 菜单库定义
typedef enum
{
	MENU_ID_CLEARTRIP = 0, 			// 清除里程
	MENU_ID_UNIT,					// 单位
	MENU_ID_SPEED_LIMIT,			// 速度
	MENU_ID_AUTO_POWER_OFF_TIME,	// 关机时间
	MENU_ID_BACKLIGHT_BRIGHTNESS,	// 背光
	MENU_ID_BLE_BROADCAST,			// 蓝牙
	MENU_ID_HMI_FW_VER,				// 仪表软件版本号
	MENU_ID_HMI_HW_VER,				// 仪表硬件版本号
	MENU_ID_CF_VER,					// 仪表控制器版本号
	MENU_ID_BF_VER,					// 仪表电池版本号

	MENU_ID_SIZE,					// 轮径	
	MENU_ID_CY,						// 周长
	MENU_ID_BAT,					// 额定电压
	MENU_ID_BAT_U, 					// 欠压
	MENU_ID_EU_A, 					// 限流
	MENU_ID_RANGE,					// 最大档位
	MENU_ID_LEVEL_PER,				// 每档助力百分比
	MENU_ID_STEEL,					// 测试磁钢
	MENU_ID_CRUIS,					// 定速巡航
	MENU_ID_RESET,					// 恢复出厂

	/**********预留新增************/


	
	MENU_ID_MAX
	
}MENU_ID_E;

/*****************************************************************************
******************************************************************************
******************************菜单配置***************************************
******************************************************************************
******************************************************************************/
// 高级菜单开关
#define CONFIG_ADVMENU_ENABLE                         0

// 普通菜单个数
#define CONFIG_MENU_MAX_ID                            5

// 高级菜单个数
#define CONFIG_ADVMENU_MAX_ID                         10

// 限速
#define CONFIG_SPEEDLIMIT_KMH_MAX  					  320
#define CONFIG_SPEEDLIMIT_KMH_MIN 					  250

#define CONFIG_SPEEDLIMIT_MPH_MAX 					  200
#define CONFIG_SPEEDLIMIT_MPH_MIN					  155

// 关机时间个数
#define CONFIG_POWER_OFFTIME_MAX					  6

// 背光等级个数
#define CONFIG_LIGHTLEVEL_MAX   			  		  5


// 轮径数组配置
#define CONFIG_WHELLID_MAX                            4
#define CONFIG_WHELLSIZE_MAX                          2

// 默认轮径和周长(单位MM)
#define CONFIG_DEFAULT_WHELL                          160
#define CONFIG_DEFAULT_PERIMETER                      2012

// 周长调节偏差值(单位MM)
#define CONFIG_PERIMETER_PNVALUE 					  200

// 电压数组配置
#define CONFIG_VOLTLEVET_X_MAX                        6
#define CONFIG_VOLTLEVET_Y_MAX                        2

// 默认电压等级(单位V) 和欠压(单位0.1V)
#define CONFIG_VOLTLEVET_PNVALUE                      36
#define CONFIG_LOWVOLTLEVET_PNVALUE                   315


// 欠压上下限制(单位.0.1v)
#define CONFIG_VOLT_UPPERLIMIT				 		  20
#define CONFIG_VOLT_LOWERLIMIT                  	  100

// 限流值最大 最小值(单位A)
#define CONFIG_CURLIMMIT_MAX 						  50
#define CONFIG_CURLIMMIT_MIN 						  10

// 最大档位个数
#define CONFIG_MAXASSIT_NUM 						  4

// 每档最大 最小PWM值
#define CONFIG_ASSITPER_MAX 						  255
#define CONFIG_ASSITPER_MIN 						  10


// 测试磁钢最大最小数
#define CONFIG_SPPEEDSENSOR_MAX 					  12
#define CONFIG_SPPEEDSENSOR_MIN 					  1


extern  const  uint8  limitKmhToMph[100];
extern  const  uint8  limitMphToKmp[63];

extern  uint8  Config_menu[CONFIG_MENU_MAX_ID];
extern  uint8  Config_advMenu[CONFIG_ADVMENU_MAX_ID];
extern  uint8  Config_PowerOffTime[CONFIG_POWER_OFFTIME_MAX];
extern  uint8  Config_lightLevel[CONFIG_LIGHTLEVEL_MAX];
extern  uint16 Config_wheelIdSize[CONFIG_WHELLID_MAX][CONFIG_WHELLSIZE_MAX];
extern  uint16 Config_VoltageLevel[CONFIG_VOLTLEVET_X_MAX][CONFIG_VOLTLEVET_Y_MAX];
extern  uint8  Config_MaxAssit[CONFIG_MAXASSIT_NUM];

#endif












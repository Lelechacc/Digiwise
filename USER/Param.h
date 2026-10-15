/** ******************************************************************************
  * 文 件 名: Param.h
  * 版 本 号: 初版
  * 修改作者: ZJ & Workman & Fly
  * 修改日期: 2017年08月06日
  * 功能介绍: 该文件用于骑行参数的获取与计算，提供可靠的数据给上层显示或传输
  ******************************************************************************
  * 注意事项:
  *
  * 							版权归迪太科技所有.
  *
*********************************************************************************/

/****************************** 版权归迪太科技所有 *********** 文件开始 *********/
#ifndef 	__PARAM_H__
#define 	__PARAM_H__

/*******************************************************************************
 *                                  头文件	                                   *
********************************************************************************/
#include "common.h"
#include "record.h"
#include "dtconfig.h"

/*******************************************************************************
 *                                  宏定义	                                   *
********************************************************************************/
#define PARAM_MCU_PAGE_NUM					64
#define PARAM_MCU_PAGE_SIZE					2048UL

// DT SN地址
#define PARAM_MCU_DT_SN_ADDEESS				(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 0)
// 客户SN地址
#define PARAM_MCU_CUSTOMER_SN_ADDEESS		(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 32)
// 硬件版本地址
#define PARAM_MCU_HW_ADDEESS				(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 64)
// BOOT版本地址
#define PARAM_MCU_BOOT_ADDEESS				(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 96)
// APP版本地址
#define PARAM_MCU_APP_ADDEESS				(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 128)

// BLE升级标志地址
#define PARAM_MCU_BLE_UPDATA_FLAG_ADDEESS	(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 160)
// BLE升级CRC地址
#define PARAM_MCU_BLE_UPDATA_CRC_ADDEESS	(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 168)
// 二维码地址
#define PARAM_MCU_QR_ADDEESS				(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 176)

// 测试标识版本地址
#define PARAM_MCU_TEST_FLAG_ADDEESS			(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 320)

#define NVM_FORMAT_FLAG_STRING				"202505281149"
#define NVM_FORMAT_FLAG_SIZE				sizeof(NVM_FORMAT_FLAG_STRING)

// NVM参数在FLASH中的首地址
#define PARAM_NVM_START_ADDRESS				(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 2)*PARAM_MCU_PAGE_SIZE)

// NVM参数在FLASH中的备用地址
#define PARAM_NVM_BACKUP_START_ADDRESS		(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 3)*PARAM_MCU_PAGE_SIZE)

// 恢复出厂设置的备份区域
#define PARAM_NVM_FACTORY_RESET_ADDRESS		(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM - 1)*PARAM_MCU_PAGE_SIZE + 512)


// 开机密码
#define PARAM_NVM_POWERON_PASSWORD 			2020

// 菜单密码
#define PARAM_NVM_MENU_PASSWORD 			2020	

#define SET_PARAMCYCLETIME           		100

// 平滑基准时间，单位:ms
#define SMOOTH_BASE_TIME				   100		

// 显示单位定义枚举定义
typedef enum
{
	UNIT_METRIC = 0,						// 公制
	UNIT_INCH,								// 英制

	UINT_COUNT	
}UNIT_E;


#define METRIC_TO_INCH_COFF					(0.621371f)	// 1 km = 0.621371192237334 inch

// 电池电量数据源
typedef enum
{
	BATTERY_DATA_SRC_CONTROLLER = 0,		// 电压由控制器上报,仪表计算电量
	BATTERY_DATA_SRC_BMS,					// 电量直接由控制器上报
	BATTERY_DATA_SRC_ADC,					// 仪表检查电压，仪表计算电量

	BATTERY_DATA_SRC_COUNT
}BATTERY_DATA_SRC_E;


// 设置轮径ID
typedef enum
{
	PARAM_WHEEL_SIZE_12_INCH = 0,
	PARAM_WHEEL_SIZE_14_INCH,
	PARAM_WHEEL_SIZE_16_INCH,
	PARAM_WHEEL_SIZE_18_INCH,
	PARAM_WHEEL_SIZE_20_INCH,
	PARAM_WHEEL_SIZE_22_INCH,
	PARAM_WHEEL_SIZE_24_INCH,
	PARAM_WHEEL_SIZE_26_INCH,
	PARAM_WHEEL_SIZE_27_INCH,
	PARAM_WHEEL_SIZE_27_5_INCH,
	PARAM_WHEEL_SIZE_28_INCH,
	PARAM_WHEEL_SIZE_29_INCH,
	PARAM_WHEEL_SIZE_700C,
	
	PARAM_WHEEL_SIZE_MAX
}PARAM_WHEEL_SIZE_ID;


// 助力档位枚举定义
typedef enum
{
	ASSIST_0 = 0,
	ASSIST_1,
	ASSIST_2,
	ASSIST_3,
	ASSIST_4,
	ASSIST_5,
	ASSIST_6,
	ASSIST_7,
	ASSIST_8,
	ASSIST_9,
	ASSIST_P,

	ASSIST_S,	// 进入限速

	ASSIST_ID_COUNT
}ASSIST_ID_E;

// 产品类型定义
typedef enum
{
	HMI_TYPE_BC18EU = 0,				// 迪太码表BC18，TFT屏，UART通讯
	HMI_TYPE_BC18SU,					// 迪太码表BC18，TFT屏，BLE，UART通讯
	HMI_TYPE_BC28EU,					// 迪太码表BC28，TFT屏，UART通讯
	HMI_TYPE_BC28SU,					// 迪太码表BC28，TFT屏，BLE，UART通讯
	HMI_TYPE_BC28NU,					// 迪太码表BC28，TFT屏，BLE，GSM，GPS，UART通讯
	HMI_TYPE_BC18EC,					// 天腾码表BC18，TFT屏，CAN通讯
	HMI_TYPE_BC28EC,					// 天腾码表BC28，TFT屏，CAN通讯

	
	HMI_TYPE_BN400,						// 数码管仪表BN400, 数码管显示, UART通讯

	HMI_TYPE_MAX,
}HMI_TYPE_E;

// 串口通讯电平定义
typedef enum
{
	UART_LEVEL_3_3 = 0,				// 串口通讯电平3.3V
	UART_LEVEL_5_0,					// 串口通讯电平5V

	UART_LEVEL_MAX,
}UART_LEVEL_TYPE_E;


// 错误代码类型定义
typedef enum
{

	/***************************************************
	 *********************仪表错误码********************	
	 ***************************************************/
	ERROR_TYPE_NO_ERROR = 0,	
	ERROR_TYPE_COMMUNICATION_TIME_OUT = 0x30,	// 通讯超时


	/***************************************************
	 *********************5S协议错误码******************
	 ***************************************************/
	 
	ERROR_TYPE_CURRENT_ERROR = 0x21,			// 电流异常
	ERROR_TYPE_THROTTLE_ERROR = 0x22,			// 转把故障
	ERROR_TYPE_MOTOR_PHASE_ERROR = 0x23,		// 电机相线故障
	ERROR_TYPE_HALL_SENSOR_ERROR = 0x24,		// 电机霍尔信号故障
	ERROR_TYPE_BRAKE_ERROR = 0x25,				// 刹车故障
	ERROR_TYPE_LOW_VOLTAGE_ERROR = 0x26,		// 欠压故障
	

	/***************************************************
	 *********************2号协议错误码******************
	 ***************************************************/
	ERROR_TYPE_BREAK_ERROR = 0x02,					// 刹车故障
	ERROR_TYPE_BATTERY_UNDER_VOLTAGE_ERROR = 0x06,	// 电池欠压
	ERROR_TYPE_MOTOR_ERROR = 0x07,					// 电机故障
	ERROR_TYPE_TURN_ERROR = 0x08,					// 转把故障
	ERROR_TYPE_DRIVER_ERROR = 0x09,					// 控制器故障	
	ERROR_TYPE_HALL_ERROR = 0x14,					// 霍尔故障


	/***************************************************
	 *********************J协议错误码*******************
	 ***************************************************/
	ERROR_TYPE_TEMP_ERROR = 0x26,				// 温度保护
	ERROR_TYPE_MOTOR_BLOCK_ERROR = 0x27,		// 堵转故障
	ERROR_TYPE_LOW_VOLTAGE2_ERROR = 0x28,		// 欠压故障				经客户要求，屏蔽28错误码
	ERROR_TYPE_OVER_VOLTAGE_ERROR = 0x29,		// 过压故障
	

	/***************************************************
	 *********************八方协议错误码****************
	 ***************************************************/
	ERROR_TYPE_BREAKED = 0x03,								// 已刹车
	ERROR_TYPE_TURN_BAR_NOT_BACK = 0x04,					// 转把没有归位
	ERROR_TYPE_TURN_BAR_ERROR = 0x05,						// 转把故障
	ERROR_TYPE_LOW_VOLTAGE_PROTECT = 0x06,					// 低电压保护
	ERROR_TYPE_OVER_VOLTAGE_PROTECT = 0x07,					// 过电压保护
	ERROR_TYPE_MOTOR_HALL_ERROR = 0x08,						// 电机霍尔信号线故障
	ERROR_TYPE_MOTOR_PHASE_ERROR2 = 0x09,					// 电机相线故障
	ERROR_TYPE_CONTROLLER_TEMP_REACH_PROTECT_POINT = 0x10,	// 控制器温度已达到保护点
	ERROR_TYPE_MOTOR_TEMP_REACH_PROTECT_POINT = 0x11,		// 电机温度高已达到保护点
	ERROR_TYPE_CURRENT_SENSOR_ERROR = 0x12,					// 电流传感器故障
	ERROR_TYPE_BATTERY_INSIDE_TEMP_ERROR = 0x13, 			// 电池内温度故障
	ERROR_TYPE_MOTOR_INSIDE_TEMP_ERROR = 0x14,				// 电机内温度传感器故障
	ERROR_TYPE_SPEED_SENSOR_ERROR = 0x21,					// 速度传感器故障
	ERROR_TYPE_BMS_COMMUNICATION_ERROR = 0x22,				// BMS通讯故障
	ERROR_TYPE_LIGHT_ERROR = 0x23,							// 大灯故障
	ERROR_TYPE_LIGHT_SENSOR_ERROR = 0x24,					// 大灯传感器故障
	ERROR_TYPE_TORQUE_SENSOR_TORQUE_ERROR = 0x25,			// 力矩传感器力矩信号故障
	ERROR_TYPE_TORQUE_SENSOR_SPEED_ERROR = 0x26,			// 力矩传感器速度故障


	
}ERROR_TYPE_E;



// ■■系统注册表，掉电保存在NVM中■■
typedef struct
{
	// 系统参数
	struct
	{
		PARAM_WHEEL_SIZE_ID wheelSizeID;	// 轮径ID
		uint16 newWheelSizeInch;
		uint16 showWheelSizeInch;			// 车辆信息显示轮径，与实际协议下发无关
		uint16 newPerimeter;
		
		uint16 powerMax;					// 最大电机功率,单位:W
	
		uint8 pwmFixed[ASSIST_ID_COUNT];	// PWM档位固定值

		uint8 eBikeName;					// 车名
		uint16 motorSys;					// 电机功率
		uint16 batteryCap;					// 电池功率
		uint8 tiresSize;					// 轮宽
		uint8 carModel[2];					// 车型
		uint16 menuPassword;				// 进入菜单界面密码
		uint16 powerOnPassword;				// 开机界面密码
		uint8 powerPasswordSwitch;			// 开机密码开关
		uint8 menuPasswordSwitch;			// 菜单密码开关	

		BOOL beepSwitch;					// 蜂鸣器开关
		uint8 limitSpeedBeepOn;				// 超速蜂鸣器提醒

		BOOL resFactorySet;					// 恢复到出厂设置
		// 驱动器设置参数
		struct
		{
			uint8 steelNumOfStartAssist;		// 助力开始的磁钢数
			uint8 steelNumOfSpeedSensor;		// 测试传感器一圈磁钢数(即 测速磁钢)
			BOOL assistDirectionForward;		// 助力正反
			uint8 assistPercent;				// 助力比例
			uint8 slowStart; 					// 缓启动参数
			BOOL turnBarSpeed6kmphLimit;		// 转把限速6km/h
			BOOL turnBarLevel;					// 转把分档
			uint32 currentLimit; 				// 电流门限，单位:mA
			uint16 lowVoltageThreshold;			// 欠压门限，单位:mV

			uint8 controlMode;					// 驱动器控制模式
			BOOL zeroStartOrNot;				// 零启动或非零启动
			BOOL switchCruiseWay;				// 切换巡航的方式
			BOOL switchCruiseMode;				// 巡航切换模式
			uint8 assistSensitivity;			// 助力灵敏度
			uint8 assistStartIntensity;			// 助力启动强度
			uint8 assistSteelType;				// 助力磁钢类型
			uint8 reversalHolzerSteelNum;		// 限速电机换向霍尔磁钢数
			uint8 turnBarSpeedLimit;			// 转把限速值,单位:Km/h
			uint8 motorFuture;					// 电机特性参数			
			uint8 turnBarFunction;				// 转把功能
			uint8 motorPulseCode;				// 电机相位编码
			uint8 assistSensorSignalNum;		// 助力传感器信号数
			uint8 assistTrim;					// 助力微调
			uint8 absBrake;						// ABS刹车强度
			uint8 hmiProperty;					// 仪表属性
			BOOL assistSwitch;					// 助力开关
			BOOL speedLimitSwitch;				// 限速位开关
			BOOL cruiseSwitch;					// 定速巡航功能开关
		}driver;
	}protocol;

	// 公共参数
	struct
	{
		HMI_TYPE_E hmiType;					// 码表类型
		UNIT_E unit;					// 单位, 公制与英制 	
		ASSIST_ID_E assistMax;				// 最大助力档位 
		uint32 busAliveTime;				// 通讯故障超时时间，单位:ms
		uint8  brightness;					// 背光亮度
		uint8  powerOffTime; 				// 自动关机时间，单位:分钟
		uint8  workAssistRatio;				// 机器与人做功助力比

		BOOL existBle;						// 是否存在蓝牙
		uint8 uartLevel;					// 串口通信电平
		BATTERY_DATA_SRC_E percentageMethod;// 电量计算方式
		uint8 uartProtocol;					// 协议选择 0：KM5S	1：锂电2号		2：八方	 3：J协议
		uint8 pushSpeedSwitch;				// 推车助力功能  0无推车助力功能，1为有推车助力功能
		uint8 defaultAssist;				// 默认档位	取值范围0~9
		uint8 logoMenu;						// LOGO项	0为迪太界面，1为中性界面，2为客户界面

		// 速度类
		struct
		{
			uint16 limitVal;				// 限速门限，单位:0.1Km/h
			uint8  filterLevel;				// 速度平滑度
		}speed;

		// 电池类
		struct
		{
			uint8 voltage;					// 电池供电电压，单位:V
			uint8 filterLevel;				// 电量变化时间，单位:秒
		}battery;
		
		struct
		{
			// 自出厂以来的统计数据
			struct
			{
				uint32 distance;			// 出厂以来总里程，单位:0.1Km				
				uint32 preDistance;			// 出厂以来总里程，单位:0.1Km
				uint16 maxTripOf24hrs;		// 个人记录，单位:0.1Km
				uint32 ridingTime;			// 骑行时间，单位:s
				uint32 calories;			// 出厂以来总卡路里，单位:KCal
			}total;

			// 自Trip清零以来的统计数据
			struct
			{
				uint32 distance;			// Trip，单位:0.1Km
				uint32 calories;			// 卡路里，单位:KCal
				uint16 speedMax;			// 最大速度，单位:0.1Km/h
				uint16 speedAverage;		// 自Trip清零以来的平均速度，单位:0.1Km/h
				uint32 ridingTime;			// 总骑行时间秒
			}trip;

			// 自0点以来的统计数据
			struct
			{
				uint16 trip;				// 今日里程，单位:0.1Km 	
				uint16 calories;			// 今日卡路里，单位:KCal

				uint32 ridingTime;			// 今日骑行累计时间，单位:s

				uint16 year;				// 今日参数对应的日期
				uint8 month; 				// 今日参数对应的日期
				uint8 day;					// 今日参数对应的日期
			}today;
		}record;

		// 放到最后，可以保证只有前面的值都写完之后才开始写格式化标识字符串
		uint8 nvmFormat[NVM_FORMAT_FLAG_SIZE];			// NVM格式化标识
	}common;	
}NVM_CB;

// NVM区尺寸
#define PARAM_NVM_DATA_SIZE						sizeof(NVM_CB)

// ■■运行过程中数据，掉电不保存■■
typedef struct
{
	
	// DT SN号
	struct
	{
		uint8 snLenth;
		uint8 snStr[30+1];
	}SnCode;

	// LIME SN号
	struct
	{
		uint8 LimeSnLenth;
		uint8 LimeSnStr[30+1];
	}LimeSnCode;

	// 硬件版本号
	struct
	{
		uint8 hwVersionLenth;
		uint8 hwVersion[30+1];
	}hwVersion;

	// BOOT版本号
	struct
	{
		uint8 bootVersionLenth;
		uint8 bootVersion[30+1];
	}bootVersion;

	// APP版本号
	struct
	{
		uint8 appVersionLenth;
		uint8 appVersion[30+1];
	}appVersion;
	
	// 仪表唯一序列号
	volatile struct
	{
		uint32 sn0;
		uint32 sn1;
		uint32 sn2;
		uint8 snStr[24+1];
	}watchUid;	

	// 测试标志
	uint32 testflag[16];

	uint32 ageTestFlag;
	uint8  pucTestFlag;
	uint32 segContrl;

	// 电池参数
	struct
	{
		uint16 voltage;							// 当前电池电压，单位:mV
		uint16 current; 						// 当前电池电流，单位:mA
		uint8 lowVoltageAlarm;					// 电池容量低告警
		uint8 percent;							// 电池百分比
		BATTERY_DATA_SRC_E batteryDataSrc;		// 电池容量数据源
		uint8 state;							// 电池状态
		uint8 preState;
		uint32 ver;
	}battery;
	
	struct
	{
		uint16 ver;

	}contor;
	

	struct
	{
		BOOL  ageTestEnable;			 
		uint32 txCount;
		uint32 rxCount;
		uint8 ageTestResoult;
	}ageTest;
	
	struct
	{
		uint8 keyNowValue;
	}keyValue;

	// 其他骑行参数，用于码表显示与公式计算
	// 实时时间
	TIME_E rtc;
	BOOL cruiseEnabe;				// 巡航开关，0:表示关闭;1:表示打开 
	BOOL cruiseState;				// 当前巡航状态，0:静止;1:表示进入巡航状态 
	BOOL pushAssistOn; 				// 6km/h推行功能，0:表示关闭;1:表示打开
	BOOL PushAssistState;			// 当前6km/h推车助力状态
	BOOL isHavePushAssist;			// 是否有推行功能
	BOOL lightSwitch; 				// 大灯开关，0:表示关闭;1:表示打开 

	uint16 speed;					// 速度，单位:0.1Km/h
	
	// 平滑算法数据结构
	struct{
		uint16 realSpeed;
		uint16 proSpeed;
		uint16 difSpeed;
	}speedFilter;
	
	uint8 cadence;					// 踏频，单位:rpm
	uint32 pulseNum; 				// 脉冲个数
	uint8 pwm;						// 发送给控制器的pwm值

	ERROR_TYPE_E errorType; 		// 当前错误代码
	ERROR_TYPE_E lastErrorType; 		// 当前错误代码

	BOOL overSpeedAlarm;			// 超速告警，TRUE:表示超速 	
	BOOL underVoltageAlarm;			// 欠压告警，0:表示不欠压；1:表示欠压

	uint8 torque;					// 力矩,单位:N.m
 

	ASSIST_ID_E assist;				// 助力档位 
	uint16 recordCount;				// 骑行统计数据条数
	uint8 ringStatus;				// 来电提醒
	uint8 warningStatus;			// 设防/撤防

	uint16 power;					// 电机功率,单位:W
	

	uint8 motorTemperature;			// 电机运行温度
	BOOL bleConnectState;			// 蓝牙是否连接的状态

	struct
	{
		uint16 moveKeepTime;		// 运动持续时间
		uint16 stopKeepTime;		// 静止持续时间
		uint16 userMaxSpeed;		// 自用户清零以来的最大速度
	}record;
	
	uint8 devVersion[3];				// 仪表型号
	uint8 fwBinNum;					// 固件版本编号
	struct
	{
		uint8 devVersionRemain;				// 设备发布版本号(需求版本号)
		uint8 devReleaseVersion;			// 软件升级更改发布版本号(产品定义版本号)
		uint8 fwUpdateVersion;				// 调试发布版本号(方案版本号)
		uint8 fwDebugReleaseVersion;		// 调试临时版本号
	}fwBinVersion;
	

	
	uint32 uiVersion;				// UI版本
	uint32 btCapacity;				// 电池容量
	uint32 fwVer;				// 软件版本
	uint32 hwVer;				// 硬件版本
}RUNTIME_DATA_CB;

/*******************************************************************************
 *                                  结构体	                                   *
********************************************************************************/
// ■■ 码表骑行参数 : 分为两大类:保存NVM与不保存NVM ■■
typedef struct
{
	RUNTIME_DATA_CB runtime;

	union
	{
  		NVM_CB param;

		uint8 array[PARAM_NVM_DATA_SIZE];
	}nvm;
	uint8 preValue[PARAM_NVM_DATA_SIZE];

	BOOL nvmWriteRequest;			// NVM更新请求
	BOOL nvmWriteEnable;			// NVM更新使能

	// 参数计算标志
	BOOL recaculateRequest;		// 参数计算请求标志，TRUE:请求计算；FALSE:不需要计算
}PARAM_CB;

#define PARAM_SIZE	(sizeof(PARAM_CB))

extern PARAM_CB paramCB;		
extern const uint32 PARAM_MCU_VERSION_ADDRESS[];

/******************************************************************************
* 【外部接口宏】
******************************************************************************/
// 设置巡航开关，0:表示关闭；1:表示打开
#define PARAM_SetCruiseSwitch(enable)		paramCB.runtime.cruiseEnabe=enable
// 获取巡航开关，0:表示关闭；1:表示打开
#define PARAM_GetCruiseSwitch()				(paramCB.runtime.cruiseEnabe)
	
	
// 设置巡航状态，0:静止；1:正在巡航
#define PARAM_SetCruiseState(enable)		paramCB.runtime.cruiseState=enable
// 获取巡航状态，0:静止；1:正在巡航
#define PARAM_GetCruiseState()				(paramCB.runtime.cruiseState)
	
	
// 打开或关闭推车助力
#define PARAM_EnablePushAssist(enable)		paramCB.runtime.pushAssistOn = enable;
													
// 获取推车助力功能状态
#define PARAM_IsPushAssistOn()				(paramCB.runtime.pushAssistOn) 
	
	
// 设置6km推车助力状态
#define PARAM_SetPushAssistState(a)			paramCB.runtime.PushAssistState=a
// 获取6km推车助力状态
#define PARAM_GetPushAssistState()			(paramCB.runtime.PushAssistState)
	
	
	
// 设置是否有推车助力功能
#define PARAM_SetIsHavePushAssist(a)		paramCB.runtime.isHavePushAssist=a
// 获取是否有推车助力功能
#define PARAM_GetIsHavePushAssist()			(paramCB.runtime.isHavePushAssist)
	
	
// 设置大灯开关 
#define PARAM_SetLightSwitch(enable)		paramCB.runtime.lightSwitch=enable;
													
																			
										
// 获取大灯开关
#define PARAM_GetLightSwitch()				(paramCB.runtime.lightSwitch)
	
#define PARAM_SetTxCount(a)						paramCB.runtime.ageTest.txCount = a
#define PARAM_GetTxCount()						(paramCB.runtime.ageTest.txCount)

#define PARAM_SetRxCount(a)						paramCB.runtime.ageTest.rxCount = a
#define PARAM_GetRxCount()						(paramCB.runtime.ageTest.rxCount)

// 设置电池电压，单位:mV
#define PARAM_SetBatteryVoltage(mV)			paramCB.runtime.battery.voltage=mV
// 获取电池电压，单位:mV
#define PARAM_GetBatteryVoltage()			(paramCB.runtime.battery.voltage)
	
// 设置电池电流，单位:mA
#define PARAM_SetBatteryCurrent(mA)			paramCB.runtime.battery.current=mA
// 获取电流，单位:mA
#define PARAM_GetBatteryCurrent()			(paramCB.runtime.battery.current)
	
// 设置速度，单位:0.1Km/h
#define PARAM_SetSpeed(a)					paramCB.runtime.speed=a
// 获取速度，单位:0.1Km/h
//#define PARAM_GetSpeed()					(paramCB.runtime.speed)
	
// 设置踏频
#define PARAM_SetCadence(a)					paramCB.runtime.cadence=a
// 获取踏频
#define PARAM_GetCadence()					(paramCB.runtime.cadence)
	
// 设置脉冲个数
#define PARAM_SetPulseNum(a)				paramCB.runtime.pulseNum=a
// 获取脉冲个数
#define PARAM_GetPulseNum()					(paramCB.runtime.pulseNum)
	
// 设置错误代码
#define PARAM_SetErrorCode(a)				paramCB.runtime.errorType=a
// 获取错误代码
#define PARAM_GetErrorCode()				(paramCB.runtime.errorType)
	
	
	
// 设置超速告警，1:表示超速
#define PARAM_SetOverSpeedAlarm(a)			paramCB.runtime.overSpeedAlarm=a
// 获取超速告警，1:表示超速
#define PARAM_GetOverSpeedAlarm()			(paramCB.runtime.overSpeedAlarm)
	
// 设置力矩
#define PARAM_SetTorque(a)					paramCB.runtime.torque=a
// 获取力矩
#define PARAM_GetTorque()					(paramCB.runtime.torque)
	
// 设置欠压告警，0:表示不欠压；1:表示欠压
#define PARAM_SetUnderVoltageAlarm(a)		paramCB.runtime.underVoltageAlarm=a
// 获取欠压告警，0:表示不欠压；1:表示欠压
#define PARAM_GetUnderVoltageAlarm()		(paramCB.runtime.underVoltageAlarm)
	
// 设置电池容量低告警
#define PARAM_SetBatteryLowAlarm(a)			paramCB.runtime.battery.lowVoltageAlarm=a
// 获取电池容量低告警
#define PARAM_GetBatteryLowAlarm()			(paramCB.runtime.battery.lowVoltageAlarm)
	 
	
// 设置日期时间
#define PARAM_SetRTC(YY,MM,DD,hh,mm,ss)		do{\
													paramCB.runtime.rtc.year=YY;\
													paramCB.runtime.rtc.month=MM;\
													paramCB.runtime.rtc.day=DD;\
													paramCB.runtime.rtc.hour=hh;\
													paramCB.runtime.rtc.minute=mm;\
													paramCB.runtime.rtc.second=ss;\
												}while(0)
// 设置系统时间秒
#define PARAM_SetRTCSecond(a)				paramCB.runtime.rtc.second=a
// 获取系统时间秒
#define PARAM_GetRTCSecond()				(paramCB.runtime.rtc.second)
	
// 设置系统时间分
#define PARAM_SetRTCMin(a)					paramCB.runtime.rtc.minute=a
// 获取系统时间分
#define PARAM_GetRTCMin()					(paramCB.runtime.rtc.minute)
	
// 设置系统时间时
#define PARAM_SetRTCHour(a)					paramCB.runtime.rtc.hour=a
// 获取系统时间时
#define PARAM_GetRTCHour()					(paramCB.runtime.rtc.hour)
	
// 设置系统时间日
#define PARAM_SetRTCDay(a)					paramCB.runtime.rtc.day=a
// 获取系统时间日
#define PARAM_GetRTCDay()					(paramCB.runtime.rtc.day)
	
// 设置系统时间月
#define PARAM_SetRTCMonth(a)				paramCB.runtime.rtc.month=a
// 获取系统时间月
#define PARAM_GetRTCMonth()					(paramCB.runtime.rtc.month)
	
// 设置系统时间年
#define PARAM_SetRTCYear(a)					paramCB.runtime.rtc.year=a
// 获取系统时间年
#define PARAM_GetRTCYear()					(paramCB.runtime.rtc.year)
	
// 设置电池百分比
#define PARAM_SetBatteryPercent(a)			paramCB.runtime.battery.percent=a
// 获取电池百分比
#define PARAM_GetBatteryPercent()			(paramCB.runtime.battery.percent)	
	
// 设置电池电量数据源
#define PARAM_SetBatteryDataSrc(a)			paramCB.runtime.battery.batteryDataSrc=a
// 获取电池电量数据源
#define PARAM_GetBatteryDataSrc()			(paramCB.runtime.battery.batteryDataSrc)	
	
#define PARAM_SetPercentageMethod(a)		paramCB.nvm.param.common.percentageMethod=a
#define PARAM_GetPercentageMethod()			(paramCB.nvm.param.common.percentageMethod)

// 设置助力档位
#define PARAM_SetAssistLevel(a)				paramCB.runtime.assist=a;

// 获取助力档位
//#define PARAM_GetAssistLevel()			(paramCB.runtime.assist)
	
// 设置骑行记录条数
#define PARAM_SetRecordCount(count)			paramCB.runtime.recordCount=count
// 获取骑行记录条数
#define PARAM_GetRecordCount()				(paramCB.runtime.recordCount)
	
// 设置来电提醒
#define PARAM_SetRingStatus(a)				paramCB.runtime.ringStatus=a
// 获取来电提醒
#define PARAM_GetRingStatus()				(paramCB.runtime.ringStatus)
	
// 设置设防/撤防
#define PARAM_SetWarningStatus(a)			paramCB.runtime.warningStatus=a
// 获取设防/撤防
#define PARAM_GetWarningStatus()			(paramCB.runtime.warningStatus)	
	
// 设置电机功率,单位:W
#define PARAM_SetPower(a)					paramCB.runtime.power=a
// 获取电机功率,单位:W
#define PARAM_GetPower()					(paramCB.runtime.power)	

// 设置定速巡航功能开关
#define PARAM_SetCruiseEnableSwitch(a)		paramCB.nvm.param.protocol.driver.cruiseSwitch=a
// 获取定速巡航功能开关
#define PARAM_GetCruiseEnableSwitch()		paramCB.nvm.param.protocol.driver.cruiseSwitch

// 设置最大电机功率,单位:W
#define PARAM_SetPowerMax(a)				paramCB.nvm.param.protocol.powerMax=a
// 获取最大电机功率,单位:W
#define PARAM_GetPowerMax()					(paramCB.nvm.param.protocol.powerMax)
	
// 设置电池电压等级,单位:V
#define PARAM_SetBatteryVoltageLevel(a)		paramCB.nvm.param.common.battery.voltage=a
// 获取电池电压等级,单位:V
#define PARAM_GetBatteryVoltageLevel()		(paramCB.nvm.param.common.battery.voltage)
	
// 设置助力开始的磁钢数
#define PARAM_SetAssitStartOfSteelNum(a)	paramCB.nvm.param.protocol.driver.steelNumOfStartAssist=a
// 获取助力开始的磁钢数
#define PARAM_GetAssitStartOfSteelNum()		(paramCB.nvm.param.protocol.driver.steelNumOfStartAssist)
	
// 设置一圈磁钢数，即测速磁钢
#define PARAM_SetCycleOfSteelNum(a)			paramCB.nvm.param.protocol.driver.steelNumOfSpeedSensor=a
// 获取一圈磁钢数，即测速磁钢
#define PARAM_GetCycleOfSteelNum()			(paramCB.nvm.param.protocol.driver.steelNumOfSpeedSensor)
	
// 设置助力正反
#define PARAM_SetAssistDirection(a)			paramCB.nvm.param.protocol.driver.assistDirectionForward=a
// 获取助力正反
#define PARAM_GetAssistDirection()			(paramCB.nvm.param.protocol.driver.assistDirectionForward)
	
// 设置电机助力比例
#define PARAM_SetAssistPercent(a)			paramCB.nvm.param.protocol.driver.assistPercent=a
// 获取电机助力比例
#define PARAM_GetAssistPercent()			(paramCB.nvm.param.protocol.driver.assistPercent)
	
// 设置缓启动参数
#define PARAM_SetSlowStart(a)				paramCB.nvm.param.protocol.driver.slowStart=a
// 获取缓启动参数
#define PARAM_GetSlowStart()				(paramCB.nvm.param.protocol.driver.slowStart)
	
// 设置转把限速6km/h
#define PARAM_SetTurnBarSpeed6kmphLimit(a)	paramCB.nvm.param.protocol.driver.turnBarSpeed6kmphLimit=a
// 获取转把限速6km/h
#define PARAM_GetTurnBarSpeed6kmphLimit()	(paramCB.nvm.param.protocol.driver.turnBarSpeed6kmphLimit)
	
// 设置转把分档
#define PARAM_SetTurnbarLevel(a)			paramCB.nvm.param.protocol.driver.turnBarLevel=a
// 获取转把分档
#define PARAM_GetTurnbarLevel()				(paramCB.nvm.param.protocol.driver.turnBarLevel)
	
// 设置转把限速
#define PARAM_SetTurnBarSpeedLimit(a)		paramCB.nvm.param.protocol.driver.turnBarSpeedLimit=a
// 获取转把限速
#define PARAM_GetTurnBarSpeedLimit()		(paramCB.nvm.param.protocol.driver.turnBarSpeedLimit)
	
// 设置电流门限，单位:mA
#define PARAM_SetCurrentLimit(a)			paramCB.nvm.param.protocol.driver.currentLimit=a
// 获取电流门限，单位:mA
#define PARAM_GetCurrentLimit()				(paramCB.nvm.param.protocol.driver.currentLimit)
	
// 设置欠压门限，单位:mV
#define PARAM_SetLowVoltageThreshold(a)		paramCB.nvm.param.protocol.driver.lowVoltageThreshold=a
// 获取欠压门限，单位:mV
#define PARAM_GetLowVoltageThreshold()		(paramCB.nvm.param.protocol.driver.lowVoltageThreshold)
	
// 设置驱动器控制模式
#define PARAM_SetDriverControlMode(a)		paramCB.nvm.param.protocol.driver.controlMode=a
// 获取驱动器控制模式
#define PARAM_GetDriverControlMode()		(paramCB.nvm.param.protocol.driver.controlMode)

// 设置电机特性参数
#define PARAM_SetMotorFuture(a)				paramCB.nvm.param.protocol.driver.motorFuture=a
// 获取电机特性参数
#define PARAM_GetMotorFuture()				(paramCB.nvm.param.protocol.driver.motorFuture)
	
// 设置零启动或非零启动
#define PARAM_SetZeroStartOrNot(a)			paramCB.nvm.param.protocol.driver.zeroStartOrNot=a
// 获取零启动或非零启动
#define PARAM_GetZeroStartOrNot()			(paramCB.nvm.param.protocol.driver.zeroStartOrNot)
	
// 设置切换巡航的方式
#define PARAM_SetSwitchCruiseWay(a)			paramCB.nvm.param.protocol.driver.switchCruiseWay=a
// 获取切换巡航的方式
#define PARAM_GetSwitchCruiseWay()			(paramCB.nvm.param.protocol.driver.switchCruiseWay)
	
// 设置巡航切换模式
#define PARAM_SetSwitchCruiseMode(a)		paramCB.nvm.param.protocol.driver.switchCruiseMode=a
// 获取巡航切换模式
#define PARAM_GetSwitchCruiseMode()			(paramCB.nvm.param.protocol.driver.switchCruiseMode)
	
// 设置助力灵敏度
#define PARAM_SetAssistSensitivity(a)		paramCB.nvm.param.protocol.driver.assistSensitivity=a
// 获取助力灵敏度
#define PARAM_GetAssistSensitivity()		(paramCB.nvm.param.protocol.driver.assistSensitivity)
	
// 设置助力启动强度
#define PARAM_SetAssistStartIntensity(a)	paramCB.nvm.param.protocol.driver.assistStartIntensity=a
// 获取助力启动强度
#define PARAM_GetAssistStartIntensity()		(paramCB.nvm.param.protocol.driver.assistStartIntensity)
	
// 设置助力磁钢类型
#define PARAM_SetSteelType(a)				paramCB.nvm.param.protocol.driver.assistSteelType=a
// 获取助力磁钢类型
#define PARAM_GetSteelType()				(paramCB.nvm.param.protocol.driver.assistSteelType)
	
// 设置限速电机换向霍尔磁钢数
#define PARAM_SetReversalHolzerSteelNum(a)	paramCB.nvm.param.protocol.driver.reversalHolzerSteelNum=a
// 获取限速电机换向霍尔磁钢数
#define PARAM_GetReversalHolzerSteelNum()	(paramCB.nvm.param.protocol.driver.reversalHolzerSteelNum)
	
// 设置转把功能
#define PARAM_SetTurnBarFunction(a)			paramCB.nvm.param.protocol.driver.turnBarFunction=a
// 获取转把功能
#define PARAM_GetTurnBarFunction()			(paramCB.nvm.param.protocol.driver.turnBarFunction)
	
// 设置电机相位编码
#define PARAM_SetMotorPulseCode(a)			paramCB.nvm.param.protocol.driver.motorPulseCode=a
// 获取电机相位编码
#define PARAM_GetMotorPulseCode()			(paramCB.nvm.param.protocol.driver.motorPulseCode)
	
// 设置助力传感器信号数
#define PARAM_SetAssistSensorSignalNum(a)	paramCB.nvm.param.protocol.driver.assistSensorSignalNum=a
// 获取助力传感器信号数
#define PARAM_GetAssistSensorSignalNum()	(paramCB.nvm.param.protocol.driver.assistSensorSignalNum)
	
// 设置助力微调
#define PARAM_SetAssistTrim(a)				paramCB.nvm.param.protocol.driver.assistTrim=a
// 获取助力微调
#define PARAM_GetAssistTrim()				(paramCB.nvm.param.protocol.driver.assistTrim)
	
// 设置abs刹车强度
#define PARAM_SetAbsBrake(a)				paramCB.nvm.param.protocol.driver.absBrake=a
// 获取abs刹车强度
#define PARAM_GetAbsBrake()					(paramCB.nvm.param.protocol.driver.absBrake)
	
// 设置仪表属性
#define PARAM_SetHmiProperty(a)				paramCB.nvm.param.protocol.driver.hmiProperty=a
// 获取仪表属性
#define PARAM_GetHmiProperty()				(paramCB.nvm.param.protocol.driver.hmiProperty)
	
// 设置助力开关
#define PARAM_SetAssistSwitch(a)			paramCB.nvm.param.protocol.driver.assistSwitch=a
// 获取助力开关
#define PARAM_GetAssistSwitch()				(paramCB.nvm.param.protocol.driver.assistSwitch)
	
// 设置限速开关
#define PARAM_SetSpeedLimitSwitch(a)		paramCB.nvm.param.protocol.driver.speedLimitSwitch=a
// 获取限速开关
#define PARAM_GetSpeedLimitSwitch()			(paramCB.nvm.param.protocol.driver.speedLimitSwitch)
	
// 设置限速门限，单位:0.1Km/h	
#define PARAM_SetSpeedLimit(a)				paramCB.nvm.param.common.speed.limitVal=a
// 获取限速门限，单位:0.1Km/h	
//#define PARAM_GetSpeedLimit() 			(paramCB.nvm.param.common.speed.limitVal)
	
// 设置轮径ID
#define PARAM_SetWheelSizeID(a)				paramCB.nvm.param.protocol.wheelSizeID=a
// 获取轮径ID
#define PARAM_GetWheelSizeID()				(paramCB.nvm.param.protocol.wheelSizeID)

// 设置蜂鸣器开关
#define PARAM_SetBeepSwitch(a)				paramCB.nvm.param.protocol.beepSwitch=a
// 获取蜂鸣器开关
#define PARAM_GetBeepSwitch()				(paramCB.nvm.param.protocol.beepSwitch)

// 设置高速蜂鸣器提醒
#define PARAM_SetLimitSpeedBeep(a)				paramCB.nvm.param.protocol.limitSpeedBeepOn=a
// 获取高速蜂鸣器提醒
#define PARAM_GetLimitSpeedBeep()				(paramCB.nvm.param.protocol.limitSpeedBeepOn)

// 设置恢复到出厂设置
#define PARAM_SetResFactorySet(a)				paramCB.nvm.param.protocol.resFactorySet=a
// 获取恢复到出厂设置
#define PARAM_GetResFactorySet()				(paramCB.nvm.param.protocol.resFactorySet)

#define PARAM_SetShowWheelSizeInch(a)			paramCB.nvm.param.protocol.showWheelSizeInch=a
#define PARAM_GetShowWheelSizeInch()			(paramCB.nvm.param.protocol.showWheelSizeInch)
#define PARAM_SetNewWheelSizeInch(a)			paramCB.nvm.param.protocol.newWheelSizeInch=a
#define PARAM_GetNewWheelSizeInch()				(paramCB.nvm.param.protocol.newWheelSizeInch)

#define PARAM_SetNewperimeter(a)				paramCB.nvm.param.protocol.newPerimeter=a
#define PARAM_GetNewperimeter()					(paramCB.nvm.param.protocol.newPerimeter)

#define PARAM_SetRunProtocol(a)					paramCB.nvm.param.protocol.runProtocol=a
#define PARAM_GetRunProtocol()					(paramCB.nvm.param.protocol.runProtocol)

	
// 设置总骑行时间秒
#define PARAM_SetTripRidingTime(a)			paramCB.nvm.param.common.record.trip.ridingTime=a
// 获取总骑行时间秒
#define PARAM_GetTripRidingTime()			(paramCB.nvm.param.common.record.trip.ridingTime)
	
// 设置Trip，单位:0.1Km
#define PARAM_SetTrip(a)					paramCB.nvm.param.common.record.trip.distance=a
// 获取Trip，单位:0.1Km
//#define PARAM_GetTrip()					(paramCB.nvm.param.common.record.trip.distance)
	
// 设置个人记录，单位:0.1Km
#define PARAM_SetPersonalBestTrip(a)		paramCB.nvm.param.common.record.total.maxTripOf24hrs=a
// 获取个人记录，单位:0.1Km
//#define PARAM_GetPersonalBestTrip()		(paramCB.nvm.param.common.record.total.maxTripOf24hrs)
		
// 设置卡路里，单位:KCal
#define PARAM_SetTripCalories(a)			paramCB.nvm.param.common.record.trip.calories=a
// 获取卡路里，单位:KCal
#define PARAM_GetTripCalories()				(paramCB.nvm.param.common.record.trip.calories)

// 设置总里程，单位:0.1Km
#define PARAM_SetTotalDistance(a)			paramCB.nvm.param.common.record.total.distance=a
// 获取总里程，单位:0.1Km
//#define PARAM_GetTotalDistance()			(paramCB.nvm.param.common.record.total.distance)
	
// 获取出厂以来的总骑行时间，单位:s
#define PARAM_GetTotalRidingTime()			(paramCB.nvm.param.common.record.total.ridingTime)
// 获取出厂以来的总卡路里，单位:KCal
#define PARAM_GetTotalCalories()			(paramCB.nvm.param.common.record.total.calories)
	
// 设置平均速度，单位:0.1Km/h
#define PARAM_SetTripAverageSpeed(a)		paramCB.runtime.trip.speedAverage=a
// 获取平均速度，单位:0.1Km/h
//#define PARAM_GetTripAverageSpeed()		(paramCB.runtime.trip.speedAverage)
		
// 设置最大速度，单位:0.1Km/h
#define PARAM_SetTripMaxSpeed(a)			paramCB.nvm.param.common.record.trip.speedMax=a
// 获取最大速度，单位:0.1Km/h
//#define PARAM_GetTripMaxSpeed()			(paramCB.nvm.param.common.record.trip.speedMax)
	
// 设置今日里程，单位:0.1Km
#define PARAM_SetToadyTrip(a)				paramCB.nvm.param.common.record.today.trip=a
// 获取今日里程，单位:0.1Km
//#define PARAM_GetTodayTrip()				(paramCB.nvm.param.common.record.today.trip)
	
// 设置今日卡路里，单位:KCal
#define PARAM_SetTodayCalories(a)			paramCB.nvm.param.common.record.today.calories=a
// 获取今日卡路里，单位:KCal
#define PARAM_GetTodayCalories()			(paramCB.nvm.param.common.record.today.calories)
	
// 设置单位, 公制与英制 
#define PARAM_SetUnit(a)					paramCB.nvm.param.common.unit=a
// 获取单位, 公制与英制 
#define PARAM_GetUnit()						(paramCB.nvm.param.common.unit)
	
// 设置背光亮度
#define PARAM_SetBrightness(a)				paramCB.nvm.param.common.brightness=a
// 获取背光亮度
#define PARAM_GetBrightness()				(paramCB.nvm.param.common.brightness)
		
// 设置速度变化平滑度
#define PARAM_SetSpeedFilterLevel(a)		paramCB.nvm.param.common.speed.filterLevel=a
// 获取速度变化平滑度
#define PARAM_GetSpeedFilterLevel()			(paramCB.nvm.param.common.speed.filterLevel)
	
// 设置自动关机时间，单位:分钟；>=0min
#define PARAM_SetPowerOffTime(a)			paramCB.nvm.param.common.powerOffTime=a
// 获取自动关机时间，单位:分钟；>=0min
#define PARAM_GetPowerOffTime()				(paramCB.nvm.param.common.powerOffTime)
	
// 设置系统做功助力比
#define PARAM_SetWorkAssistRatio(a)			paramCB.nvm.param.common.workAssistRatio=a
// 获取系统做功助力比
#define PARAM_GetWorkAssistRatio()			(paramCB.nvm.param.common.workAssistRatio)
	
// 设置今日骑行时间秒
#define PARAM_SetTodayRidingTime(a)			paramCB.nvm.param.common.record.today.ridingTime=a
// 获取今日骑行时间秒
#define PARAM_GetTodayRidingTime()			(paramCB.nvm.param.common.record.today.ridingTime)
	
// 设置最大档位
#define PARAM_SetMaxAssist(a)				do{\
													paramCB.nvm.param.common.assistMax=a;\
													NVM_SetPWMLimitValue();\
												}while(0)
// 获取最大档位
#define PARAM_GetMaxAssist()				(paramCB.nvm.param.common.assistMax)

// 设置协议
#define PARAM_SetUartProtocol(a)			paramCB.nvm.param.common.uartProtocol=a
// 获取协议
#define PARAM_GetUartProtocol()				(paramCB.nvm.param.common.uartProtocol)
	
// 设置推车助力功能
#define PARAM_SetPushSpeedSwitch(a)			paramCB.nvm.param.common.pushSpeedSwitch=a
// 获取推车助力功能
#define PARAM_GetPushSpeedSwitch()			(paramCB.nvm.param.common.pushSpeedSwitch)
	
// 设置默认档位
#define PARAM_SetDefaultAssist(a)			paramCB.nvm.param.common.defaultAssist=a
// 获取默认档位
#define PARAM_GetDefaultAssist()			(paramCB.nvm.param.common.defaultAssist)
	
// 设置LOGO界面
#define PARAM_SetlogoMenu(a)				paramCB.nvm.param.common.logoMenu=a
// 获取LOGO界面
#define PARAM_GetlogoMenu()					(paramCB.nvm.param.common.logoMenu)
	
// 设置串口通讯电平
#define PARAM_SetUartLevel(a)				paramCB.nvm.param.common.uartLevel=a
// 获取串口通讯电平
#define PARAM_GetUartLevel()				(paramCB.nvm.param.common.uartLevel)
	
// 设置总线通讯故障超时时间，单位:ms
#define PARAM_SetBusAliveTime(a)			paramCB.nvm.param.common.busAliveTime=a
// 获取总线通讯故障超时时间，单位:ms
#define PARAM_GetBusAliveTime()				(paramCB.nvm.param.common.busAliveTime)
	
// 设置电池电量变化时间，单位:s
#define PARAM_SetBatteryCapVaryTime(a)		paramCB.nvm.param.common.battery.filterLevel=a
// 获取电池电量变化时间，单位:s
#define PARAM_GetBatteryCapVaryTime()		(paramCB.nvm.param.common.battery.filterLevel)
	
// 设置运动状态保持时间，单位:s
#define PARAM_SetMoveTimeKeepTime(a)		paramCB.runtime.record.moveKeepTime=a
// 获取运动状态保持时间，单位:s
#define PARAM_GetMoveTimeKeepTime()			(paramCB.runtime.record.moveKeepTime)
	
// 设置静止状态保持时间，单位:s
#define PARAM_SetStopTimeKeepTime(a)		paramCB.runtime.record.stopKeepTime=a
// 获取静止状态保持时间，单位:s
#define PARAM_GetStopTimeKeepTime()			(paramCB.runtime.record.stopKeepTime)
	
// 用户最大速度清零
#define PARAM_ClearUserMaxSpeed()			paramCB.runtime.record.userMaxSpeed=0
// 获取用户最大速度
#define PARAM_GetUserMaxSpeed()				(paramCB.runtime.record.userMaxSpeed)
	
// 设置电机温度
#define PARAM_SetMotorTemperature(a)		paramCB.runtime.motorTemperature=a
// 获取电机温度
#define PARAM_GetMotorTemperature()			(paramCB.runtime.motorTemperature)
	
// 设置电池状态
#define PARAM_SetBatteryState(s)			paramCB.runtime.battery.state=s
// 获取电池状态
#define PARAM_GetBatteryState()				(paramCB.runtime.battery.state)
	
// 获取FW版本号
#define PARAM_GetFWVersion()				(paramCB.runtime.fwVersion)
	
// 获取硬件版本号
#define PARAM_GetHWVersion()				(paramCB.runtime.hwVersion)
	
// 获取电池容量
#define PARAM_GetBCVersion()				(paramCB.runtime.btCapacity)

// 设置测试按键值
#define PARAM_SetKeyValue(a)				paramCB.runtime.keyValue.keyNowValue=a

// 获取测试按键值
#define PARAM_GetKeyValue()					(paramCB.runtime.keyValue.keyNowValue)

// 设置是否有蓝牙
#define PARAM_SetExistBle(a)				paramCB.nvm.param.common.existBle=a
// 获取是否有蓝牙
#define PARAM_GetExistBle()					(paramCB.nvm.param.common.existBle)

// 菜单界面密码
#define	PARAM_SetMenuPassword(a)			paramCB.nvm.param.protocol.menuPassword=a
#define	PARAM_GetMenuPassword()				(paramCB.nvm.param.protocol.menuPassword)

// 开机界面密码
#define	PARAM_SetPowerOnPassword(a)			paramCB.nvm.param.protocol.powerOnPassword=a
#define	PARAM_GetPowerOnPassword()			(paramCB.nvm.param.protocol.powerOnPassword)

// 判断是否有开机密码
#define	PARAM_SetPowerPasswordSwitch(a)		paramCB.nvm.param.protocol.powerPasswordSwitch=a
#define	PARAM_GetPowerPasswordSwitch()		(paramCB.nvm.param.protocol.powerPasswordSwitch)

// 判断是否有菜单密码
#define	PARAM_SetMenuPasswordSwitch(a)		paramCB.nvm.param.protocol.menuPasswordSwitch=a
#define	PARAM_GetMenuPasswordSwitch()		(paramCB.nvm.param.protocol.menuPasswordSwitch)

#define PARAM_SeteBikeName(a)					paramCB.nvm.param.protocol.eBikeName=a
#define PARAM_GeteBikeName()					(paramCB.nvm.param.protocol.eBikeName)

#define PARAM_SetbatteryCap(a)					paramCB.nvm.param.protocol.batteryCap=a
#define PARAM_GetbatteryCap()					(paramCB.nvm.param.protocol.batteryCap)

#define PARAM_SettiresSize(a)					paramCB.nvm.param.protocol.tiresSize=a
#define PARAM_GettiresSize()					(paramCB.nvm.param.protocol.tiresSize)

// 设置蓝牙是否连接状态
#define	PARAM_SetBleConnectState(a)			paramCB.runtime.bleConnectState=a
// 获取蓝牙是否连接状态
#define	PARAM_GetBleConnectState()			(paramCB.runtime.bleConnectState)

// 获取固件版本编号
#define PARAM_GetFwBinNum()					(paramCB.runtime.fwBinNum)

// 获取固件版本号保留位
#define PARAM_GetDevVersionRemain()			(paramCB.runtime.fwBinVersion.devVersionRemain)


// 获取固件发布版本号
#define PARAM_Get_DevReleaseVersion()		(paramCB.runtime.fwBinVersion.devReleaseVersion)


// 获取固件需求升级版本号
#define PARAM_Get_FwUpdateVersion()			(paramCB.runtime.fwBinVersion.fwUpdateVersion)


// 获取固件修复bug升级版本号
#define PARAM_Get_FwDebugReleaseVersion()	(paramCB.runtime.fwBinVersion.fwDebugReleaseVersion)

/******************************************************************************
* 【外部接口声明】
******************************************************************************/
// 模块初始化，完成模块初始化
void PARAM_Init(void);

// 模块过程处理
void PARAM_Process(void);

// NVM更新，传入参数指明是否立即写入
void NVM_SetDirtyFlag(BOOL writeAtOnce);
void NVM_Save(BOOL saveAll);

uint16 PARAM_GetSpeed(void);
uint32 PARAM_GetTrip(void);
uint32 PARAM_GetTotalDistance(void);
uint16 PARAM_GetTripAverageSpeed(void);
uint16 PARAM_GetTripMaxSpeed(void);
ASSIST_ID_E  PARAM_GetAssistLevel(void);

uint8 PARAM_GetPwmFixed(uint8 asist);
void NVM_SetPWMLimitValue(void);

// 返回出厂设置
void PARAM_FactoryReset(void);

// 保存初始化设置参数
void NVM_Save_FactoryReset(BOOL saveAll);
	
// Trip清零
void PARAM_ClearTrip(void);

// 协议初始化选择
void PARAM_ProtocolInitSelect(void);

// 运行协议选择
void PARAM_ProtocolProcessSelect(void);

void PARAM_SpeedFilterDevValue(uint32 param);

#endif



#ifndef __RECORD_H__
#define __RECORD_H__

#include "common.h"
#include "ds1302.h"
#include "iap.h"

// 单元测试开关
#define RECORD_UNIT_TEST_ON						0

// 是否带蓝牙模块，有(1)则开启骑行数据记录功能，无(0)则被忽略
#define RECORD_WITH_BLE_MODULE					1

// 骑行数据启动和停止时间阈值
#define RECORD_MOVE_TIME_THRESHOLD				8		// 单位:s
#define RECORD_STOP_TIME_THRESHOLD				10		// 单位:s

// 非授时存储数据条数 间隔5min存储一条，一天可以存288条记录
#define RECORD_RIDING_COUNT						288
// 非授时情况下存储数据时间间隔 5min
#define RECORD_RIDING_TIME						300

// NVM格式化标识
#define RECORD_NVM_FORMAT_FLAG					"2020000807134122"//"20171012111700"
#define RECORD_NVM_FORMAT_FLAG_SIZE				sizeof(RECORD_NVM_FORMAT_FLAG)

// ■■E2中的参数和数据结构定义■■	

// 总记录条数的保存地址
#define RECORD_ADDR_TOTAL						(IAP_MCU_RECORD_ADDR_BASE+RECORD_NVM_FORMAT_FLAG_SIZE)

// 已发送的记录条数
#define RECORD_ADDR_SEND_COUNT					(RECORD_ADDR_TOTAL+2)

// 首条记录的保存位置
#define RECORD_ADDR_DATA_START					(RECORD_ADDR_SEND_COUNT+2)

// 末条记录的保存位置
#define RECORD_ADDR_DATA_END					(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM-3)*PARAM_MCU_PAGE_SIZE)	// 预留5K做系统参数记录使用

// BLE同步时间的Flag位置
#define RECORD_ADDR_BLE_CHECK_TIME_START		(RECORD_ADDR_DATA_START+sizeof(TRACK_TIME)+2+2+2+2+1)

// 骑行记录条数上限
#define RECORD_COUNT_MAX						((RECORD_ADDR_DATA_END-RECORD_ADDR_DATA_START)/sizeof(RECORD_PARAM_CB))

// 骑行记录数据地址
#define IAP_MCU_RECORD_ADDR_BASE				(IAP_N32_FLASH_BASE+(PARAM_MCU_PAGE_NUM-5)*PARAM_MCU_PAGE_SIZE)

// 各参数在DS1302片内RAM中保存地址
enum
{
	RECORD_PARAM_ADDR_RTC_VALID_FLAG = 0,
	RECORD_PARAM_ADDR_DIRTY_FLAG,
	RECORD_PARAM_ADDR_DATE_YEAR,
	RECORD_PARAM_ADDR_DATE_MONTH,
	RECORD_PARAM_ADDR_DATE_DAY,
	RECORD_PARAM_ADDR_DATE_HOUR,
	RECORD_PARAM_ADDR_DATE_MINUTE,
	RECORD_PARAM_ADDR_DATE_SECOND,
	RECODE_PARAM_ADDR_TRIPTIMER_H,
	RECODE_PARAM_ADDR_TRIPTIMER_L,
	RECORD_PARAM_ADDR_MAXSPEED_H,
	RECORD_PARAM_ADDR_MAXSPEED_L,
	RECORD_PARAM_ADDR_KCAL_H,
	RECORD_PARAM_ADDR_KCAL_L,
	RECORD_PARAM_ADDR_TRIP_H,
	RECORD_PARAM_ADDR_TRIP_L,
	RECODE_PARAM_ADDR_TIMESLOT,
	
	RECORD_PARAM_ADDR_IS_CHECK_BLE_TIME,
	RECORD_PARAM_ADDR_CHECK,
	RECORD_PARAM_ADDR_MAX
};

// 时间参数结构体
typedef struct
{
	uint8 year;
	uint8 month;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
}TRACK_TIME;

// 一字节对齐
#pragma pack(1)
// 参数
typedef struct
{
	TRACK_TIME dateTime;
	
	// 骑行数据
	uint16 tripTimer;			// 运动时长，单位秒
	uint16 speedMax;			// 最大速度，单位:0.1Km/h
	uint16 calories;			// 卡路里，单位:KCal
	uint16 trip; 				// Trip，单位:0.1Km
	uint8 timeSlot;				// 所属时间段
	
	uint8 isTimeCheck;			// 蓝牙同时时间标志位, 
								// 1:同步校准；
								// 0:上电时判断为0时，强制为0xFF；非上电则该数据临时有效，未校准；
								// FF:该条数据无效
	uint8_t packCheck;			// 数据包校验位
}RECORD_PARAM_CB;
#pragma pack()

// 骑行记录数据结构
typedef union
{
	RECORD_PARAM_CB param;

	uint8 array[sizeof(RECORD_PARAM_CB)];
}RECORD_ITEM_CB;

typedef struct 
{
	uint8 hour;					// 运动所属时间段
	uint8 calories;				// 卡路里
	uint8 trip;					// 骑行距离
	uint16 speedMax;			// 最大速度
	uint16 min;					// 运动分钟
	uint16 ridingTime;			// 骑行时间
}RECORD_PARAM_TMP;

typedef union
{
	RECORD_PARAM_TMP recordparam;
	
	uint8 array[sizeof(RECORD_PARAM_TMP)];
}RECORD_ITEM_TMP;

// 未授时保存的数据结构
typedef struct 
{
	uint8 tripFlag;								// 未授时骑行标志

	uint16 tirpNum;								// 未授时骑行记录次数

	uint32 tirpTime;							// 未授时骑行时间，单位s
	
	RECORD_ITEM_TMP recordItemTmp[RECORD_RIDING_COUNT];			// 未授时骑行记录，掉电不保存的

}RECORD_ITEM_TMP_CB;

extern RECORD_ITEM_TMP_CB recordTmpCB;

// 系统状态定义
typedef enum
{
	RECORD_STATE_NULL = 0,							// 空状态

	RECORD_STATE_STANDBY,							// 待机状态

	RECORD_STATE_RECORDING,							// 持续记录状态

	RECORD_STATE_MAX								// 状态数

}RECORD_STATE_E;


// 骑行信息总控结构体
typedef struct
{
	uint8 nvmFormat[RECORD_NVM_FORMAT_FLAG_SIZE];	// NVM格式化标识

	RECORD_STATE_E state;				// 当前系统状态
	RECORD_STATE_E preState;			// 上一个状态

	// 起始参数
	struct
	{
		uint32 ridingTime;
		uint32 distance;
		uint32 calories;
	}start;

	uint8 ds1302Addr[RECORD_PARAM_ADDR_MAX];
}RECORD_CB;

extern RECORD_CB recordCB;


// 骑行信息模块上电初始化准备
void RECORD_Init(void);

// 清空数据记录
void RECORD_ClearRecords(void);

// 写入一个骑行记录点，如果当前缓冲区内无数据，则更新开始时间；如果缓冲区已有数据，则只更新结束时间
void RECORD_CALLBACK_DoBackup(uint32 param);

// 将当前缓存写入NVM
void RECORD_SaveToNVM(void);

// 读取一条记录
BOOL RECORD_ReadFromNVM(RECORD_ITEM_CB* pTrack);

// 获取骑行记录条数
uint16 RECORD_GetRecordCount(void);

// 获取已发送记录条数
uint16 RECORD_GetRecordSendCount(void);

// 状态迁移
void RECORD_EnterState(uint32 state);

// 状态机处理
void RECORD_Process(void);

// 获取记录状态
RECORD_STATE_E RECORD_GetState(void);

// 确认历史数据发送成功之后，刷新已发送历史数据，即已发送历史数据加一
BOOL RECORD_RefreashSendCount(void);

// 查询是否有未同步历史数据
BOOL RECORD_IsHistoricalData(void);

// 刷新已发送历史数据条数
void RECORD_RefreashSaveCount(void);

// 骑行记录校验
uint8_t RECORD_Check(uint8_t *ptr, uint8_t len);

#endif




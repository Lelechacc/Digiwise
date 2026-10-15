#ifndef 	__DS1302_H__
#define 	__DS1302_H__

#include "common.h"

#define	DS1302_REFRESH_TIME				1000		// 时间更新周期，单位:ms
#define	MINUTE_COUNT_OF_24_HOURS		(24*60)		// 24小时的分钟数

// DS1302单元测试开关
#define DS1302_UNIT_TEST				0

#define DS1302_START_YEAR				1970

// DS1302驱动I2C端口定义(用户根据自己端口进行修改)
#define DS1302_CE_DIR_OUT()				
#define DS1302_CE_DIR_IN()				
#define DS1302_SCLK_DIR_OUT()			
#define DS1302_SCLK_DIR_IN()			
#define DS1302_SDA_DIR_OUT()			
#define DS1302_SDA_DIR_IN() 			DS1302_SDA_HIGH()	// 开漏模式，输出1即可读取外部电平

#define DS1302_CE_HIGH()				
#define DS1302_CE_LOW() 				
#define DS1302_SCLK_HIGH()				
#define DS1302_SCLK_LOW()				
#define DS1302_SDA_HIGH()				
#define DS1302_SDA_LOW()				
#define DS1302_SDA_GET()				
#define DS1302_SDA_SET(i)				

// 延时设置(用户根据自己需要进行调整)
#define	DS1302_DELAY_SWITCH 			1		// 延时方式选择0:简短方式延时NOP()	 1:子函数延时DS1302_Delayus(i)
#if DS1302_DELAY_SWITCH
#define	DS1302_DELAY()					do{\
											DS1302_Delayus(5);\
										}while(0)											
#else
#define	DS1302_DELAY()					do{\
											NOP();\
											NOP();\
										}while(0)											
#endif

// ■■■■■■■ 以下部分不能随意修改 ■■■■■■■■■■■■■■■
#define	DS1302_REG_SECOND				0x80	// 秒的写地址
#define	DS1302_REG_MINUTE				0x82	// 分钟的写地址
#define	DS1302_REG_HOUR					0x84	// 小时的写地址
#define	DS1302_REG_DATE					0x86	// 日的写地址
#define	DS1302_REG_MONTH				0x88	// 月的写地址
#define	DS1302_REG_WEEK 				0x8A	// 星期的写地址
#define	DS1302_REG_YEAR					0x8C	// 年的写地址
#define	DS1302_REG_WP					0x8E	// 写保护
#define	DS1302_REG_CHARGE				0x90	// 充电
#define DS1302_RAM(X)					(0xC0+(X)*2)// 用于计算 DS1302_RAM 地址的宏

#define	DS1302_REG_MULTIPLE				0xBE	// 多字节RTC写命令
#define	DS1302_RAM_MULTIPLE				0xFE	// 多字节RAM写命令
#define DS1302_CHARGE_ENABLE			0xA0	// 使能充电
#define DS1302_CHARGE_DISABLE			0x00	// 禁止充电
#define DS1302_DIODE_NONE				0x00	// 无二极管接入
#define DS1302_DIODE_ONE				0x04	// 一个二极管接入
#define DS1302_DIODE_TWO				0x08	// 两个二极管接入
#define DS1302_RES_NONE					0x00	// 无电阻接入
#define DS1302_RES_2K					0x01	// 2K电阻接入
#define DS1302_RES_4K					0x02	// 4K电阻接入
#define DS1302_RES_8K					0x03	// 8K电阻接入

#define DS1302_PROTECT_ON				0x80		// 保护使能
#define DS1302_PROTECT_OFF				0x00		// 保护禁止
#define DS1302_START(X)					(X&0x7F)	// 启动时钟
#define DS1302_STOP(X)					(X|0x80)	// 停止时钟
#define DS1302_HOUR_MODE_12(X)			(X|0x80)	// 12小时制
#define DS1302_HOUR_MODE_24(X)			(X&0x7F)	// 24小时制
#define DS1302_RD(X)					(X|0x01)	// 读取操作
#define DS1302_WR(X)					(X&0xFE)	// 写入操作

#define	DS1302_MASK_YEAR				0xFF	// 年
#define	DS1302_MASK_WEEK				0x07	// 星期
#define	DS1302_MASK_MONTH				0x1F	// 月
#define	DS1302_MASK_DATE				0x3F	// 日
#define	DS1302_MASK_HOUR				0x3F	// 时
#define	DS1302_MASK_MINUTE				0x7F	// 分
#define	DS1302_MASK_SECOND				0x7F	// 秒

typedef unsigned int time_t;
typedef struct 
{
	int tm_year; // 年份，其值等于实际年份减去1900
	int tm_mon;	 // 月份（从一月开始，0代表一月） - 取值区间为[0,11]
	int tm_mday; // 一个月中的日期 - 取值区间为[1,31]
	int tm_hour; // 时 - 取值区间为[0,23]
	int tm_min;	 // 分 - 取值区间为[0,59]
	int tm_sec;	 // 秒 – 取值区间为[0,59]
}TM_T;

typedef struct
{
	uint16 year;
	uint8 month;
	uint8 day;
	uint8 hour;
	uint8 minute;
	uint8 second;
}TIME_E;

typedef struct
{
	TIME_E rtc;						// 实时时间

}DS1302_CB;

extern DS1302_CB ds1302CB;

// DS1302初始化
void DS1302_Init(void);

// RTC处理，在main中调用
void DS1302_Process(void);

// 调整时间
void DS1302_SetTime(uint8 hour, uint8 minute, uint8 second);

// 调整日期
uint32 DS1302_SetDate(uint16 year, uint8 month, uint8 day, uint8 hour, uint8 minute, uint8 second);

#if 0
// 调整日期
void DS1302_SetDateCB(TIME_E* time);
#endif

// 获取日期
void DS1302_GetDateCB(TIME_E* time);

// 将一个字节写入RAM
void DS1302_WriteRAM(uint8 addr, uint8 data);

// 从RAM读取一个字节
uint8 DS1302_ReadRAM(uint8 addr);

// 日期转时间戳
uint32 DS1302_GetTimeStampFromDateTime(TIME_E dataTime);

// 时间戳转换为日期
// second是从1900年1月1日开始计算的秒
void DS1302_TimeStampToDateTime(uint32 second, TIME_E* dateTime);

#endif


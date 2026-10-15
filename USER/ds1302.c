#include "common.h"
#include "ds1302.h"
#include "timer.h"
#include "param.h"
#include "record.h"

DS1302_CB ds1302CB;

// 简单延时
void DS1302_Delayus(uint8 i);

// 简单延时
void DS1302_Delayms(uint8 i);

// 单元测试
void DS1302_UnitTest(void);

// 多字节写DS1302的8个RTC寄存器或RAM单元
void DS1302_BurstWrite(uint8 *pBuf, uint8 Size, uint8 RegFlag);

// 多字节读DS1302的RTC寄存器或RAM单元
void DS1302_BurstRead(uint8 *pBuf, uint8 Size, uint8 RegFlag);

// 回调函数，用于周期性地设置更新请求
void DS1302_CALLBACK_SetUpdateRequest(uint32 param);

// 16进制转压缩BCD码
uint8 DS1302_HexToBCD(uint8 data);

// 压缩BCD码转16进制
uint8 DS1302_BCDToHex(uint8 data);

// 闰年判断函数
int isleap(int year);

// 时间生成时间戳
time_t DS1302_mktime(TM_T dt);

// 时间戳转换成时间
void DS1302_localtime(time_t time, TM_T*t);

/*********************************************************
* 函数定义区
*********************************************************/
// 简单延时
void DS1302_Delayus(volatile uint8 i)
{	 
	// 内核72MHz，1us
	while (i--) 
	{
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();	// 10
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();	// 10
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();	// 10
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();	// 6
	}
}

// 简单延时
void DS1302_Delayms(volatile uint8 i)
{	 
	volatile uint16 x;
	volatile uint8 y;

	// 内核72MHz，1ms
	while (i--)
	{
		x = 700; 
		while (x--)
		{
			y = 10;
			while (y--)
			{
				NOP(); 
			} 
		}
	}
}

// 单元测试
void DS1302_UnitTest(void)
{
#if DS1302_UNIT_TEST
	
	//DS1302_SetDate(2019,12,04,15,03,00);
	//DS1302_SetDate(2099,12,31,23,59,59);
	//DS1302_SetDate(2000,1,1,0,0,0);
	//DS1302_SetDate(2019,2,29,0,0,0);
	//DS1302_SetDate(2100,2,28,0,0,0);
	//while (1)
	{
		TIME_E date1 = {2000,1,1,0,0,0};
		TIME_E date2 = {2019,12,06,12,12,12};
		TIME_E date3 = {2099,12,30,1,1,1};
		uint32 time1,time2,time3;
		
		time1 = DS1302_GetTimeStampFromDateTime(date1);
		time2 = DS1302_GetTimeStampFromDateTime(date2);
		time3 = DS1302_GetTimeStampFromDateTime(date3);

		DS1302_TimeStampToDateTime(time1, &date3);
		DS1302_TimeStampToDateTime(time2, &date2);
		DS1302_TimeStampToDateTime(time3, &date1);
	}	
	
#endif
}

// DS1302初始化
void DS1302_Init(void)
{
	// 上电初始化时间
	ds1302CB.rtc.year = DS1302_START_YEAR;	
	ds1302CB.rtc.month = 0x01;
	ds1302CB.rtc.day = 0x01;
	ds1302CB.rtc.hour = 0x00;	
	ds1302CB.rtc.minute = 0x00;
	ds1302CB.rtc.second = 0x00;
	
	// 启动定时器任务，周期性地计算时间	
	TIMER_AddTask(TIMER_ID_RTC_READ,
				  DS1302_REFRESH_TIME,
				  DS1302_CALLBACK_SetUpdateRequest,
				  0,
				  TIMER_LOOP_FOREVER,
				  ACTION_MODE_DO_AT_ONCE);

#if DS1302_UNIT_TEST
	DS1302_UnitTest();
#endif
}

// RTC处理，在main中调用
void DS1302_Process(void)
{
	// 将当前日期时间添加到参数层
	PARAM_SetRTC(ds1302CB.rtc.year,
				ds1302CB.rtc.month,
				ds1302CB.rtc.day,
				ds1302CB.rtc.hour,
				ds1302CB.rtc.minute,
				ds1302CB.rtc.second);
}

// 调整时间
void DS1302_SetTime(uint8 hour, uint8 minute,  uint8 second)
{
    // 时间格式不对
    if ((hour > 23) || (minute > 59) || (second > 59))
    {
        return;
    }
    
	ds1302CB.rtc.hour = hour;
	ds1302CB.rtc.minute = minute;
	ds1302CB.rtc.second = second;

	/*
	// 写入新的时间
	DS1302_WriteRegister(DS1302_REG_WP, DS1302_PROTECT_OFF);	// Open Write Protect bit
	
	DS1302_WriteRegister(DS1302_REG_SECOND, DS1302_START(DS1302_HexToBCD(second) & DS1302_MASK_SECOND));	    // 写秒
	
	DS1302_WriteRegister(DS1302_REG_MINUTE, DS1302_HexToBCD(minute) & DS1302_MASK_MINUTE);	            		// 写分钟
	
	DS1302_WriteRegister(DS1302_REG_HOUR, DS1302_HOUR_MODE_24(DS1302_HexToBCD(hour)) & DS1302_MASK_HOUR);   	// 写小时(24小时制)
	
	DS1302_WriteRegister(DS1302_REG_WP, DS1302_PROTECT_ON);		// Close Write Protect bit
	*/
}

// 调整日期
uint32 DS1302_SetDate(uint16 year, uint8 month, uint8 day, uint8 hour, uint8 minute, uint8 second)
{
	uint8 dayCompare = 31;
	
    // 时间格式不对
    if ((year > 2099 ) || (month > 12) || (day > 31) || (hour > 23) || (minute > 59) || (second > 59))
    {
        return UINT32_NULL;
    }

	
	 // 根据不同的月份判断设置的天数是否符合要求
	if (2 == month)
	{
		// 闰年2月29天
		if ((0 == (year % 400)) 
			|| ((0 == (year % 4)) && (0 != (year % 100))))
		{
			dayCompare = 29;
		}
		else
		{
			dayCompare = 28;
		}
	}
	else if ((4 == month) 
			|| (6 == month)
			|| (9 == month)
			|| (11 == month))
	{
		dayCompare = 30;
	}
	else
	{
		dayCompare = 31;
	}

	// 天数格式不对
	if (day > dayCompare)
	{
		return UINT32_NULL;
	}

    ds1302CB.rtc.year = year;
	ds1302CB.rtc.month = month;
	ds1302CB.rtc.day = day;
	ds1302CB.rtc.hour = hour;
	ds1302CB.rtc.minute = minute;
	ds1302CB.rtc.second = second;

	return 0;
}

// 获取日期
void DS1302_GetDateCB(TIME_E* time)
{
	// 入参检验
	if(NULL == time)
	{
		return;
	}

	time->year = ds1302CB.rtc.year;
	time->month = ds1302CB.rtc.month;
	time->day = ds1302CB.rtc.day;
	time->hour = ds1302CB.rtc.hour;
	time->minute = ds1302CB.rtc.minute;
	time->second = ds1302CB.rtc.second;
}

// 回调函数，用于周期性地设置更新请求，定时器调用
void DS1302_CALLBACK_SetUpdateRequest(uint32 param)
{
	uint8 monthDays = 0;
	
	// 计算分钟
	if (60 == (++ds1302CB.rtc.second))
	{
	   ds1302CB.rtc.second = 0;
	   ds1302CB.rtc.minute ++;
	}
	else
	{
		return;
	}

	// 计算小时
	if (60 == ds1302CB.rtc.minute)
	{
		ds1302CB.rtc.minute = 0;
		ds1302CB.rtc.hour ++;
	}

	// 计算天
	if (24 == ds1302CB.rtc.hour)
	{
		ds1302CB.rtc.hour = 0;
		ds1302CB.rtc.day ++;
	}

	 // 计算月
	if (2 == ds1302CB.rtc.month)
	{
		// 闰年2月29天
		if ((0 == (ds1302CB.rtc.year % 400)) 
			|| ((0 == (ds1302CB.rtc.year % 4)) && (0 != (ds1302CB.rtc.year % 100))))
		{
			monthDays = 29;
		}
		else
		{
			monthDays = 28;
		}
	}
	else if ((4 == ds1302CB.rtc.month) 
			|| (6 == ds1302CB.rtc.month)
			|| (9 == ds1302CB.rtc.month)
			|| (11 == ds1302CB.rtc.month))
	{
		monthDays = 30;
	}
	else
	{
		monthDays = 31;
	}
	
	if (ds1302CB.rtc.day == (monthDays + 1))
	{
		ds1302CB.rtc.day = 1;
		ds1302CB.rtc.month ++;
	}

	// 计算年 
	if (13 == ds1302CB.rtc.month)
	{
		ds1302CB.rtc.month = 1;
		ds1302CB.rtc.year ++;
	}

}

// 16进制转压缩BCD码
uint8 DS1302_HexToBCD(uint8 data)
{
	uint8 bcd;

	bcd = (data/10)<<4;
	bcd += data%10;

	return bcd;
}

// 压缩BCD码转16进制
uint8 DS1302_BCDToHex(uint8 data)
{
	uint8 hex;

	hex = (data>>4)*10;
	hex += data&0x0F;

	return hex;
}

// 将一个字节写入RAM
void DS1302_WriteRAM(uint8 addr, uint8 data)
{
	// 地址合法性判断，合法的地址范围是0-30，共31个字节，因此超过30就判定非法值
	if(addr > RECORD_PARAM_ADDR_MAX)
	{
		return;
	}

	recordCB.ds1302Addr[addr] = data;
}

// 从RAM读取一个字节
uint8 DS1302_ReadRAM(uint8 addr)
{
	// 地址合法性判断，合法的地址范围是0-30，共31个字节，因此超过30就判定非法值
	if(addr > RECORD_PARAM_ADDR_MAX)
	{
		return 0;
	}

	//return DS1302_ReadRegister(DS1302_RAM(addr));

	return recordCB.ds1302Addr[addr];
}

// 根据剩余天数获取本年的月份与天数
void DS1302_GetMonthAndDay(uint32 nDays, BOOL IsLeapYear, uint8* nMonth, uint8* nDay)
{
	uint8 DayOfMon[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};	// 平年每个月对应的天数
	int i = 0, nTempDay = 0;

	// 参数判断
	if ((NULL == nMonth) || (NULL == nDay))
	{
		return;
	}

	//循环减去12个月中每个月的天数，直到剩余天数小于等于0，就找到了对应的月份
	for (i = 0; i < 12; i++)
	{
		// 如果是润年，2月份特殊处理		
		if ((IsLeapYear) &&(1 == i))
		{
			DayOfMon[i] = 29; 
		}
		
		// nTemp <= 0，证明已经是当月了，计算当月月份与天数
		nTempDay = nDays - DayOfMon[i];
		if (nTempDay <= 0)
		{
			// 计算当月月份
			nMonth[0] = i + 1;

			// 计算当月天数
			if (0 == nTempDay)			//表示刚好是这个月的最后一天，那么天数就是这个月的总天数了
			{
				nDay[0] = DayOfMon[i];
			}
			else
			{
				nDay[0] = nDays;
			}
			
			break;
		}
		
		nDays = nTempDay;
	}
}

// 时间戳转换为日期
// second从DS1302_START_YEAR年1月1日开始计算的秒
void DS1302_TimeStampToDateTime(uint32 second, TIME_E* dateTime)
{

	TM_T timeTemp;
	uint32 secTemp = second;
	DS1302_localtime(secTemp,&timeTemp);

	dateTime->year = (uint16)timeTemp.tm_year;
	dateTime->month = (uint8)timeTemp.tm_mon;
	dateTime->day = (uint8)timeTemp.tm_mday;
	dateTime->hour = (uint8)timeTemp.tm_hour;
	dateTime->minute = (uint8)timeTemp.tm_min;
	dateTime->second = (uint8)timeTemp.tm_sec;
	
}
 

// 日期转换为时间戳
// 从DS1302_START_YEAR/1/1/0/0/0到现在的时间戳
// 返回UINT32_NULL为无效值
uint32 DS1302_GetTimeStampFromDateTime(TIME_E dataTime)
{
	TM_T timeTemp;
	timeTemp.tm_year = (int)dataTime.year;
	timeTemp.tm_mon = (int)dataTime.month;
	timeTemp.tm_mday = (int)dataTime.day;
	timeTemp.tm_hour = (int)dataTime.hour;
	timeTemp.tm_min = (int)dataTime.minute;
	timeTemp.tm_sec = (int)dataTime.second;

	return DS1302_mktime(timeTemp);
}

int isleap(int year)
{
	return (year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0);
}

time_t DS1302_mktime(TM_T dt)
{
	time_t mon_yday[2][12] =
	{
		{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
		{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
	};

	time_t result;
	int i = 0;
	
	// 以平年时间计算的秒数
	result = (dt.tm_year - 1970) * 365 * 24 * 3600 +
			 (mon_yday[isleap(dt.tm_year)][dt.tm_mon - 1] + dt.tm_mday - 1) * 24 * 3600 +
			 dt.tm_hour * 3600 + dt.tm_min * 60 + dt.tm_sec;
	// 加上闰年的秒数
	for (i = 1970; i < dt.tm_year; i++)
	{
		if (isleap(i))
		{
			result += 24 * 3600;
		}
	}
	return (result);
}


void DS1302_localtime(time_t time, TM_T*t)
{
	char Days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	unsigned int Pass4year;
	int hours_per_year;
	
	//取秒时间
	t->tm_sec = (int)(time % 60);
	time /= 60;
	//取分钟时间
	t->tm_min = (int)(time % 60);
	time /= 60;
	//取过去多少个四年，每四年有 1461*24 小时
	Pass4year = ((unsigned int)time / (1461L * 24L));
	//计算年份
	t->tm_year = (Pass4year << 2) + 1970;
	//四年中剩下的小时数
	time %= 1461L * 24L;
	//校正闰年影响的年份，计算一年中剩下的小时数
	
	for (;;)
	{
		//一年的小时数
		hours_per_year = 365 * 24;
		//判断闰年
		if ((t->tm_year & 3) == 0)
		{
			//是闰年，一年则多24小时，即一天
			hours_per_year += 24;
		}
		if (time < hours_per_year)
		{
			break;
		}
		t->tm_year++;
		time -= hours_per_year;
	}
	
	//小时数
	t->tm_hour = (int)(time % 24);
	//一年中剩下的天数
	time /= 24;
	//假定为闰年
	time++;
	
	//校正闰年的误差，计算月份，日期
	if ((t->tm_year & 3) == 0)
	{
		if (time > 60)
		{
			time--;
		}
		else
		{
			if (time == 60)
			{
				t->tm_mon = 2;
				t->tm_mday = 29;
				return;
			}
		}
	}
	
	//计算月日
	for (t->tm_mon = 0; Days[t->tm_mon] < time; t->tm_mon++)
	{
		time -= Days[t->tm_mon];
	}
	t->tm_mon += 1;
	t->tm_mday = (int)(time);
	return;
}





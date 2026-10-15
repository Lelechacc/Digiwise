/***************************************************************************************************************************
*  【说明】
*          删除了轨迹记录的功能后，不再需要向SPI Flash中频繁写入轨迹点数据，而是启用了DS1302的RAM来保存频繁修改的数据，
*      一次骑行结束后再写入Flash
*  DS1302片内RAM使用定义
*  [ 0] - 时钟运行标识
*  [ 1] - 脏标识，为0x55说明当前数据未写入E2PROM或Flash，其它值说明当前数据已写入
*  [ 2] - 时间:年
*  [ 3] - 时间:月
*  [ 4] - 时间:日
*  [ 5] - 时间:时
*  [ 6] - 时间:分
*  [ 7] - 时间:秒
*  [ 8] - 运动时长(min)H
*  [ 9] - 运动时长(min)L
*  [10] - 最高速度H
*  [11] - 最高速度L
*  [12] - 卡路里(KCal)H
*  [13] - 卡路里(KCal)L
*  [14] - 里程H
*  [15] - 里程L
*  [16] - 运动时间段
*  [17] - 蓝牙校准时间标志
*  [18] - 
*  [19] - 
*  [20] - 
*  [21] - 
*  [22] - 
*  [23] - 
*  [24] - 
*  [25] - 
*  [26] - 
*  [27] - 
*  [28] - 
*  [29] - 
*  [30] - 
***************************************************************************************************************************/
#include "timer.h"
#include "delay.h"
#include "record.h"
#include "BleProtocol.h"
#include "ds1302.h"
#include "param.h"
#include "iap.h"

RECORD_CB recordCB;

RECORD_ITEM_TMP_CB recordTmpCB = {0};

// 返回出厂设置的密钥
const uint8 constFormatFlag[] = RECORD_NVM_FORMAT_FLAG;

// 设置脏标识
void RECORD_SetDirtyFlag(void)
{
	// 写统计数据后，设置脏标识
	DS1302_WriteRAM(RECORD_PARAM_ADDR_DIRTY_FLAG, 0x55);
}

// 清除脏标识
void RECORD_ClearDirtyFlag(void)
{
	// 清除脏标识
	DS1302_WriteRAM(RECORD_PARAM_ADDR_DIRTY_FLAG, 0);
}

// 读取脏标识是否有效
BOOL RECORD_IsDirty(void)
{
	if(0x55 == DS1302_ReadRAM(RECORD_PARAM_ADDR_DIRTY_FLAG))
	{
		return TRUE;
	}
	
	return FALSE;
}

// 获取骑行记录条数，包括未同步时间的记录
uint16 RECORD_GetRecordCount(void)
{
	uint8 bBuff[2];
	
	IAP_FlashReadByteArray(RECORD_ADDR_TOTAL, bBuff, 2);
	return (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
}

// 获取已发送记录条数
uint16 RECORD_GetRecordSendCount(void)
{
	uint8 bBuff[2];
	
	IAP_FlashReadByteArray(RECORD_ADDR_SEND_COUNT, bBuff, 2);
	return (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
}

// 骑行记录校验
uint8_t RECORD_Check(uint8_t *ptr, uint8_t len)
{
	uint8_t i;
	uint8_t cc = 0;
	
	for(i=0; i<len; i++)
	{
		cc ^= ptr[i];
	}
	return (~cc) ;
}

// 从缓冲区中读取一个过程统计数据
void RECORD_ReadBuffer(RECORD_ITEM_CB* pTrackCB)
{
	uint16 temp;
	
	if(NULL == pTrackCB)
	{
		return;
	}

	// 将当前记录写入缓存
	pTrackCB->param.dateTime.year = DS1302_ReadRAM(RECORD_PARAM_ADDR_DATE_YEAR);			// 年份只保留十位和个位
	pTrackCB->param.dateTime.month = DS1302_ReadRAM(RECORD_PARAM_ADDR_DATE_MONTH);
	pTrackCB->param.dateTime.day = DS1302_ReadRAM(RECORD_PARAM_ADDR_DATE_DAY);
	pTrackCB->param.dateTime.hour = DS1302_ReadRAM(RECORD_PARAM_ADDR_DATE_HOUR);
	pTrackCB->param.dateTime.min = DS1302_ReadRAM(RECORD_PARAM_ADDR_DATE_MINUTE);
	pTrackCB->param.dateTime.sec = DS1302_ReadRAM(RECORD_PARAM_ADDR_DATE_SECOND);

	temp = DS1302_ReadRAM(RECODE_PARAM_ADDR_TRIPTIMER_H);
	temp <<= 8;
	temp += DS1302_ReadRAM(RECODE_PARAM_ADDR_TRIPTIMER_L);
	pTrackCB->param.tripTimer = temp;

	temp = DS1302_ReadRAM(RECORD_PARAM_ADDR_MAXSPEED_H);
	temp <<= 8;
	temp += DS1302_ReadRAM(RECORD_PARAM_ADDR_MAXSPEED_L);
	pTrackCB->param.speedMax = temp;

	temp = DS1302_ReadRAM(RECORD_PARAM_ADDR_KCAL_H);
	temp <<= 8;
	temp += DS1302_ReadRAM(RECORD_PARAM_ADDR_KCAL_L);
	pTrackCB->param.calories = temp;

	temp = DS1302_ReadRAM(RECORD_PARAM_ADDR_TRIP_H);
	temp <<= 8;
	temp += DS1302_ReadRAM(RECORD_PARAM_ADDR_TRIP_L);
	pTrackCB->param.trip = temp;

	pTrackCB->param.timeSlot = DS1302_ReadRAM(RECODE_PARAM_ADDR_TIMESLOT);

	pTrackCB->param.isTimeCheck = DS1302_ReadRAM(RECORD_PARAM_ADDR_IS_CHECK_BLE_TIME);
	
	pTrackCB->param.packCheck = RECORD_Check(pTrackCB->array, sizeof(RECORD_ITEM_CB) - 1);
}

// 判断NVM是否已格式化
BOOL RECORD_IsFormatOK(void)
{
	uint8 i;
	//uint32 bBuff[1];
	
	// 读取格式化标志
	IAP_FlashReadWordArray(IAP_MCU_RECORD_ADDR_BASE, (uint32*)recordCB.nvmFormat, RECORD_NVM_FORMAT_FLAG_SIZE/4);

	// 逐个比较，只要发现一个不同，即认为未格式化
	for (i = 0; i < RECORD_NVM_FORMAT_FLAG_SIZE; i++)
	{
		if(constFormatFlag[i] != recordCB.nvmFormat[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}

// 格式化NVM
void RECORD_FormatNVM(void)
{
	uint8 i;
	uint8 bBuff[2] = {0};

	// 初始化单元参数
	for (i = 0; i < RECORD_NVM_FORMAT_FLAG_SIZE; i++)
	{
		recordCB.nvmFormat[i] = constFormatFlag[i];
	}

	// 写格式化密匙
	IAP_FlashWriteWordArrayWithErase(IAP_MCU_RECORD_ADDR_BASE, (uint32*)recordCB.nvmFormat, RECORD_NVM_FORMAT_FLAG_SIZE/4);

	// 初始化记录条为0
	IAP_FLASH_WriteByteWithErase(RECORD_ADDR_TOTAL, bBuff ,2);

	// 已读路线编号为0
	IAP_FLASH_WriteByteWithErase(RECORD_ADDR_SEND_COUNT, bBuff ,2);

	// 参数层记录的数据记录条数清零
	PARAM_SetRecordCount(0);
}

// 清空数据记录
void RECORD_ClearRecords(void)
{
	uint8 bBuff[2] = {0};
	
	// 初始化记录条为0
	IAP_FLASH_WriteByteWithErase(RECORD_ADDR_TOTAL, bBuff ,2);

	// 已读路线编号为0
	IAP_FLASH_WriteByteWithErase(RECORD_ADDR_SEND_COUNT, bBuff ,2);

	// 清除脏标识
	RECORD_ClearDirtyFlag();

	// 参数层记录的数据记录条数清零
	PARAM_SetRecordCount(0);
}

// 写入一个骑行记录点，如果当前缓冲区内无数据，则更新开始时间；如果缓冲区已有数据，则只更新结束时间
void RECORD_CALLBACK_DoBackup(uint32 param)
{
	TIME_E time;
	uint32 temp1, temp2;

	DS1302_GetDateCB(&time);
	
	// 当前无数据，则写入开始时间
	if(!RECORD_IsDirty())
	{
		// 清除参数层保存的用户最大速度
		PARAM_ClearUserMaxSpeed();

		// 保存起始参数
		recordCB.start.ridingTime = PARAM_GetTotalRidingTime();
		recordCB.start.distance = paramCB.nvm.param.common.record.total.distance;
		recordCB.start.calories = PARAM_GetTotalCalories();
		
		// 判断蓝牙是否校准过时间
		// 蓝牙校准过，则数据第0个字节保存为1；
		// 蓝牙校没准过，则数据第0个字节保存为0；
		// 注意上电需要排查该校验位，非1则改写为0xFF；
		DS1302_WriteRAM(RECORD_PARAM_ADDR_IS_CHECK_BLE_TIME, (uint8)bleProtocolCB.isTimeCheck);
		
		DS1302_WriteRAM(RECORD_PARAM_ADDR_DATE_YEAR, time.year%100);			// 年份只保留十位和个位
		DS1302_WriteRAM(RECORD_PARAM_ADDR_DATE_MONTH, time.month);
		DS1302_WriteRAM(RECORD_PARAM_ADDR_DATE_DAY, time.day);
		DS1302_WriteRAM(RECORD_PARAM_ADDR_DATE_HOUR, time.hour);
		DS1302_WriteRAM(RECORD_PARAM_ADDR_DATE_MINUTE, time.minute);
		DS1302_WriteRAM(RECORD_PARAM_ADDR_DATE_SECOND, time.second);

		// 保存所属时间段
		DS1302_WriteRAM(RECODE_PARAM_ADDR_TIMESLOT, time.hour);

		// 写入开始时间后，设置脏标识
		RECORD_SetDirtyFlag();
	}


	// 到目前为止的骑行时间，单位:s
	temp2 = PARAM_GetTotalRidingTime() - recordCB.start.ridingTime;
	
	DS1302_WriteRAM(RECODE_PARAM_ADDR_TRIPTIMER_H, temp2>>8);
	DS1302_WriteRAM(RECODE_PARAM_ADDR_TRIPTIMER_L, temp2&0x00FF);
	
	// 保存最大速度
	DS1302_WriteRAM(RECORD_PARAM_ADDR_MAXSPEED_H, PARAM_GetUserMaxSpeed()>>8);
	DS1302_WriteRAM(RECORD_PARAM_ADDR_MAXSPEED_L, PARAM_GetUserMaxSpeed()&0x00FF);

	// 保存到目前为止的卡路里消耗
	temp1 = PARAM_GetTotalCalories() - recordCB.start.calories;
	DS1302_WriteRAM(RECORD_PARAM_ADDR_KCAL_H, temp1>>8);
	DS1302_WriteRAM(RECORD_PARAM_ADDR_KCAL_L, temp1&0x00FF);

	// 保存到目前为止的骑行距离
	temp1 = paramCB.nvm.param.common.record.total.distance - recordCB.start.distance;
	DS1302_WriteRAM(RECORD_PARAM_ADDR_TRIP_H, temp1>>8);
	DS1302_WriteRAM(RECORD_PARAM_ADDR_TRIP_L, temp1&0x00FF);
}

// 判断是否需要备份与更新
void RECORD_CALLBACK_IsDoBackup(uint32 param)
{	
	RECORD_CALLBACK_DoBackup(0);
}

// 未授时骑行记录更新
void RECORD_Update(uint32 param)
{
	static uint8 updateFlag;
	//static uint32 ridingTime;
	static uint32 distance;
	static uint32 calories;
	static uint16 tripTimeTmp;
	
	recordTmpCB.tirpTime++;
	tripTimeTmp++;

	// 首次进来备份当前信息
	if (updateFlag == 0)
	{
		// 清除参数层保存的用户最大速度
		PARAM_ClearUserMaxSpeed();
	
		//ridingTime = PARAM_GetTotalRidingTime();
		distance = paramCB.nvm.param.common.record.total.distance;
		calories = PARAM_GetTotalCalories();
		
		updateFlag = 1;
		
		tripTimeTmp = 1;
	}else if ((param == 1) && (updateFlag == 1))
	{
		updateFlag = 0;

		if (recordTmpCB.tirpNum >= RECORD_RIDING_COUNT)
		{
			return;
		}
		
		// 骑行时间段时间,此时间段内的里程为0，则不更新骑行时间
		if (paramCB.nvm.param.common.record.total.distance - distance != 0)
		{
			recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.ridingTime += tripTimeTmp;
		}
		
		// 骑行累计分钟时刻
		recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.min = (uint16)(recordTmpCB.tirpTime/60);
		
		// 骑行最大速度
		if (recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.speedMax < PARAM_GetUserMaxSpeed())
		{
			recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.speedMax = PARAM_GetUserMaxSpeed();
		}
		
		// 骑行卡路里
		recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.calories += PARAM_GetTotalCalories() - calories;;
		
		// 骑行里程
		recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.trip += paramCB.nvm.param.common.record.total.distance - distance;
	}
	
	// 未授时情况下，一旦开始骑行就开始每5min记录一次数据
	if (recordTmpCB.tirpTime%RECORD_RIDING_TIME == 0)
	{
		updateFlag = 0;

		if (recordTmpCB.tirpNum >= RECORD_RIDING_COUNT)
		{
			return;
		}
		
		// 骑行时间段时间,此时间段内的里程为0，则不更新骑行时间
		if (paramCB.nvm.param.common.record.total.distance - distance != 0)
		{
			recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.ridingTime += tripTimeTmp;
		}
		
		// 骑行累计分钟时刻
		recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.min = (uint16)(recordTmpCB.tirpTime/60);
		
		// 骑行最大速度
		if (recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.speedMax < PARAM_GetUserMaxSpeed())
		{
			recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.speedMax = PARAM_GetUserMaxSpeed();
		}
		
		// 骑行卡路里
		recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.calories += PARAM_GetTotalCalories() - calories;;
		
		// 骑行里程
		recordTmpCB.recordItemTmp[recordTmpCB.tirpNum].recordparam.trip += paramCB.nvm.param.common.record.total.distance - distance;

		recordTmpCB.tirpNum++;
	}
}

// 将当前缓存写入NVM
void RECORD_SaveToNVM(void)
{
	uint16 count;
	uint32 addr;
	RECORD_ITEM_CB track;
	uint8 bBuff[2];
	
	// 当前无缓存，退出
	if(!RECORD_IsDirty())
	{
		return;
	}

	// 已有记录条数达到或超过允许数量，退出
	IAP_FlashReadByteArray(RECORD_ADDR_TOTAL, bBuff, 2);
	count = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
	if(count >= (uint16)RECORD_COUNT_MAX)
	{
		return;
	}

	// 读取缓存
	RECORD_ReadBuffer(&track);

	// 计算待写入位置的首地址
	addr = sizeof(RECORD_PARAM_CB);
	addr *= count;
	addr += RECORD_ADDR_DATA_START;

	// 写入数据
	IAP_FLASH_WriteByteWithErase(addr, (uint8*)track.array, sizeof(RECORD_PARAM_CB));	

	// 记录条数累加
	bBuff[0] = ((uint8)(count+1));
	bBuff[1] = ((uint8)(count+1) >> 8);
	IAP_FLASH_WriteByteWithErase(RECORD_ADDR_TOTAL, bBuff, 2);
	
	// 清除缓存
	RECORD_ClearDirtyFlag();

	// 更新参数层记录的数据记录条数
	PARAM_SetRecordCount(count+1);
}

// 读取最早的一条的记录
BOOL RECORD_ReadFromNVM(RECORD_ITEM_CB* pTrack)
{
	uint16 totalCount, readCount;
	uint32 addr;
	uint8 bBuff[2];

	if(NULL == pTrack)
	{
		return FALSE;
	}

	// 读取已有记录条数和已读出条数
	IAP_FlashReadByteArray(RECORD_ADDR_TOTAL, bBuff, 2);
	totalCount = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
	IAP_FlashReadByteArray(RECORD_ADDR_SEND_COUNT, bBuff, 2);
	readCount = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
	
	// 已读出数达到或超出总条数，退出，清除历史记录
	if(readCount >= totalCount)
	{
		RECORD_ClearRecords();
		return FALSE;
	}

	// 计算读取的首地址
	addr = sizeof(RECORD_PARAM_CB);
	addr *= readCount;
	addr += RECORD_ADDR_DATA_START;

	// 读取
	IAP_FlashReadByteArray(addr, (uint8*)pTrack->array, sizeof(RECORD_PARAM_CB));
	// 判断校验位是否是正确的数据
	if (pTrack->param.packCheck != RECORD_Check(pTrack->array, sizeof(RECORD_ITEM_CB) - 1))
	{
		return FALSE;
	}

	// 判断是否是蓝牙校准之后的数据，如果不是，则屏蔽掉这条数据上传并更新发送数据条数
	if (0x01 != pTrack->param.isTimeCheck)
	{
		// 更新已发送历史条数
		bBuff[0] = (uint8)(readCount+1);
		bBuff[1] = (uint8)((readCount+1) >> 8);
		IAP_FLASH_WriteByteWithErase(RECORD_ADDR_SEND_COUNT, bBuff, 2);
		return FALSE;
	}


	// 更新已读取计数
	//SPI_FLASH_WriteHalfWord(RECORD_ADDR_SEND_COUNT, readCount+1);	 
	
	return TRUE;
}

// 刷新已发送历史数据条数
BOOL RECORD_RefreashSendCount(void)
{
	uint16 totalCount,readCount;
	uint8 bBuff[2];
	
		// 读取已有记录条数和已读出条数
	IAP_FlashReadByteArray(RECORD_ADDR_TOTAL, bBuff, 2);
	totalCount = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
	IAP_FlashReadByteArray(RECORD_ADDR_SEND_COUNT, bBuff, 2);
	readCount = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
	
	// 已读出数达到或超出总条数，退出
	if(readCount >= totalCount)
	{
		return FALSE;
	}

	// 更新已发送历史条数
	bBuff[0] = (uint8)(readCount+1);
	bBuff[1] = (uint8)((readCount+1) >> 8);
	IAP_FLASH_WriteByteWithErase(RECORD_ADDR_SEND_COUNT, bBuff, 2);
	
	return TRUE;
}

void RECORD_UnitTest(void)
{
	
	uint8 testWriteBuf[50] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
	uint8 testReadBuf[50] = {0x00};
	
	// 写入数据
	IAP_FLASH_WriteByteWithErase(RECORD_ADDR_DATA_START, testWriteBuf, 32);	
	// 读出数据
	IAP_FlashReadByteArray(RECORD_ADDR_DATA_START, testReadBuf, 32);
}

// 刷新写入写入骑行历史数据条数
void RECORD_RefreashSaveCount(void)
{
	uint16 totalCount;
	uint8 bBuff[2];
	
	IAP_FlashReadByteArray(RECORD_ADDR_TOTAL, bBuff, 2);
	totalCount = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
	
	bBuff[0] = (uint8)(totalCount+1);
	bBuff[1] = (uint8)((totalCount+1) >> 8);
	
	IAP_FLASH_WriteByteWithErase(RECORD_ADDR_TOTAL, bBuff, 2);
}

// 骑行信息模块上电初始化准备
void RECORD_Init(void)
{
	// 未格式化，则格式化
	if (!RECORD_IsFormatOK())
	{
		RECORD_FormatNVM();
	}
	else
	{
	
	}

#if RECORD_UNIT_TEST_ON
	RECORD_UnitTest();
#endif

	// 默认状态为空
	recordCB.state = RECORD_STATE_NULL;
	recordCB.preState = RECORD_STATE_NULL;

	// 将记录条数传给参数层
	PARAM_SetRecordCount(RECORD_GetRecordCount());

	// 指定时间后进入待机
	TIMER_AddTask(TIMER_ID_RECORD_CONTROL,
					500UL,
					RECORD_EnterState,
					RECORD_STATE_STANDBY,
					1,
					ACTION_MODE_ADD_TO_QUEUE);
}

// 每个状态的入口处理
void RECORD_EnterState(uint32 state)
{
	// 让当前的状态成为历史
	recordCB.preState = recordCB.state;

	// 设置新的状态
	recordCB.state = (RECORD_STATE_E)state;

	// 无蓝牙操作跳出
	if (!PARAM_GetExistBle())
	{
		return;
	}
	
	// 各状态的入口设定
	switch(state)
	{
		// ■■ 空状态 ■■
		case RECORD_STATE_NULL:
			break;

		// ■■ 待机状态入口处理 ■■
		case RECORD_STATE_STANDBY:
		
			// 停止记录定时器
			TIMER_KillTask(TIMER_ID_RECORD_CONTROL);

			if (bleProtocolCB.isTimeCheck == FALSE)
			{
				RECORD_Update(1);
			}
			else
			{
				// 每次进入待机状态，都保存一次
				RECORD_SaveToNVM();
			}

			// 判断蓝牙是否连接，是否有历史数据
			if((PARAM_GetBleConnectState()) && (RECORD_IsHistoricalData()) && (PARAM_GetSpeed() == 0))
			{

				// 如果没有接收到应答，五秒中循环发送请求同步数据
				TIMER_AddTask(TIMER_ID_BLE_START_HISTORICAL_DATA_UP,
								5000,
								BLE_PROTOCOL_TxStartHistoryData,
								UINT32_NULL,
								TIMER_LOOP_FOREVER,
								ACTION_MODE_ADD_TO_QUEUE);
			}

			break;

		// ■■ 记录中状态入口处理 ■■
		case RECORD_STATE_RECORDING:
		
			// 开始进入骑行记录,上电只能进来一次
			if (bleProtocolCB.isTimeCheck == FALSE)
			{
				if (recordTmpCB.tripFlag == 0)
				{
					recordTmpCB.tripFlag = 1;
					
					TIMER_AddTask(TIMER_ID_RECORD_COUNT,
									1000UL,
									RECORD_Update,
									0,
									TIMER_LOOP_FOREVER,
									ACTION_MODE_ADD_TO_QUEUE);
				}
			}
			else
			{
				// 每次进入此状态，都记录一次数据
				RECORD_CALLBACK_DoBackup(0);

				// 首次进入此状态，开启定时器任务，每秒记录一次(不要随意改，否则出错)
				if(recordCB.preState != recordCB.state)
				{
					TIMER_AddTask(TIMER_ID_RECORD_CONTROL,
									1000UL,
									RECORD_CALLBACK_IsDoBackup,
									0,
									TIMER_LOOP_FOREVER,
									ACTION_MODE_ADD_TO_QUEUE);
				}
			}

			break;

		default:
			break;
	}
}

// 每个状态下的过程处理
void RECORD_Process(void)
{
	TIME_E time;
	static uint8 lastSecond;
	
	switch(recordCB.state)
	{
		// ■■ 空状态 ■■
		case RECORD_STATE_NULL:
			break;

		// ■■ 待机状态过程处理 ■■
		case RECORD_STATE_STANDBY:

			// 运动时间达到或超过门限，开始记录
			if(PARAM_GetMoveTimeKeepTime() >= RECORD_MOVE_TIME_THRESHOLD)
			{
				// 进入记录中状态
				RECORD_EnterState(RECORD_STATE_RECORDING);
			}
			break;

		// ■■ 记录中状态过程处理 ■■
		case RECORD_STATE_RECORDING:

			// 静止时间达到或超过门限，停止记录
			if(PARAM_GetStopTimeKeepTime() >= RECORD_STOP_TIME_THRESHOLD)
			{
				// 进入待机状态
				RECORD_EnterState(RECORD_STATE_STANDBY);

				// 蓝牙上报状态消息
				if(PARAM_GetBleConnectState())
				{
					BLE_PROTOCOL_SendCmdStatusAck();
				}
			}

			// 无蓝牙操作跳出
			if (!PARAM_GetExistBle())
			{
				break;
			}

			// 骑行状态过程中，整点存储一条数据
			DS1302_GetDateCB(&time);
			if((0 == time.minute) && (0 == time.second) && (59 == lastSecond))
			{
				// 每次进入整点状态都保存一次
				RECORD_SaveToNVM();
			}

			// 更新上一次秒数，重复进入整点记录数据
			lastSecond = time.second;
			break;

		default:
			break;
	}
}

// 获取记录状态
RECORD_STATE_E RECORD_GetState(void)
{
	return recordCB.state;
}

// 查询是否有未同步历史数据
BOOL RECORD_IsHistoricalData(void)
{
	uint16 totalCount, readCount;
	uint8 bBuff[2];
	
	// 读取已有记录条数和已读出条数
	IAP_FlashReadByteArray(RECORD_ADDR_TOTAL, bBuff, 2);
	totalCount = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
	IAP_FlashReadByteArray(RECORD_ADDR_SEND_COUNT, bBuff, 2);
	readCount = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];

	if(readCount < totalCount)
	{
		return TRUE;  
	}
	else
	{
		return FALSE;
	}
}



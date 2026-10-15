#include "common.h"
#include "timer.h"
#include "delay.h"
#include "uartDrive.h"
#include "uartProtocol2.h"
#include "param.h"
#include "state.h"

/******************************************************************************
* 【内部接口声明】
******************************************************************************/

// 数据结构初始化
void UART_PROTOCOL2_DataStructInit(UART_PROTOCOL2_CB* pCB);

// UART报文接收处理函数(注意根据具体模块修改)
void UART_PROTOCOL2_MacProcess(uint16 standarID, uint8* pData, uint16 length);

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void UART_PROTOCOL2_RxFIFOProcess(UART_PROTOCOL2_CB* pCB);

// UART命令帧缓冲区处理
void UART_PROTOCOL2_CmdFrameProcess(UART_PROTOCOL2_CB* pCB);

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL UART_PROTOCOL2_ConfirmTempCmdFrameBuff(UART_PROTOCOL2_CB* pCB);

// 通讯超时处理-单向
void UART_PROTOCOL2_CALLBACK_RxTimeOut(uint32 param);

// 停止RX通讯超时检测任务
void UART_PROTOCOL2_StopRxTimeOutCheck(void);

// 协议层发送处理过程
void UART_PROTOCOL2_TxStateProcess(void);

// UART协议层向驱动层注册数据发送接口
void UART_PROTOCOL2_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length));

//===============================================================================================================
// 码表设置周期下发数据请求标志
void UART_PROTOCOL2_CALLBACK_SetTxPeriodRequest(uint32 param);

// 码表发送数据通讯时序处理
void UART_PROTOCOL2_DataCommunicationSequenceProcess(void);

// 启动通讯超时判断任务
void UART_PROTOCOL2_StartTimeoutCheckTask(void);

// UART总线超时错误处理
void UART_PROTOCOL2_CALLBACK_UartBusError(uint32 param);

//=============================================================================
void UART_PROTOCOL2_ReadProtocolVersion(uint8 param);
void UART_PROTOCOL2_ReadDriverStatus(uint8 param);
void UART_PROTOCOL2_ReadSysWorkStatus(uint8 param);
void UART_PROTOCOL2_ReadInstantaneousCurrent(uint8 param);
void UART_PROTOCOL2_ReadBatteryCapacity(uint8 param);
void UART_PROTOCOL2_ReadRPM(uint8 param);
void UART_PROTOCOL2_ReadLightSwitch(uint8 param);
void UART_PROTOCOL2_ReadBatteryInfo1(uint8 param);
void UART_PROTOCOL2_ReadBatteryInfo2(uint8 param);
void UART_PROTOCOL2_ReadRemainderMileage(uint8 param);
void UART_PROTOCOL2_ReadDriverErrorCode(uint8 param);
void UART_PROTOCOL2_ReadCadence(uint8 param);
void UART_PROTOCOL2_ReadTQKG(uint8 param);
void UART_PROTOCOL2_WriteLimitSpeed(uint16 param);
void UART_PROTOCOL2_WriteAssistLevel(uint8 maxAssistLevel, uint8 assistLevel);
void UART_PROTOCOL2_WriteLightSwitch(uint8 param);
void UART_PROTOCOL2_WriteClearDriverErrorCode(uint8 param);
// 和控制器数据交互步骤处理
void UART_PROTOCOL2_DataInteractionProcess(uint32 param);

// 全局变量定义
UART_PROTOCOL2_CB uartProtocolCB2;

// ■■函数定义区■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// 启动通讯超时判断任务
void UART_PROTOCOL2_StartTimeoutCheckTask(void)
{
	// 清除通讯异常
	if(ERROR_TYPE_COMMUNICATION_TIME_OUT == PARAM_GetErrorCode())
	{
		PARAM_SetErrorCode(ERROR_TYPE_NO_ERROR);
	}

	// 开启UART通讯超时定时器
#if __SYSTEM_NO_TIME_OUT_ERROR__		// 研发调试时不启用
#else
	TIMER_AddTask(TIMER_ID_PROTOCOL_TXRX_TIME_OUT,
					PARAM_GetBusAliveTime(),
					UART_PROTOCOL2_CALLBACK_UartBusError,
					TRUE,
					1,
					ACTION_MODE_ADD_TO_QUEUE);
#endif
}

// 协议初始化
void UART_PROTOCOL2_Init(void)
{
	// 协议层数据结构初始化
	UART_PROTOCOL2_DataStructInit(&uartProtocolCB2);

	// 向驱动层注册数据接收接口
	UART_DRIVE_RegisterDataSendService(UART_PROTOCOL2_MacProcess);

	// 向驱动层注册数据发送接口
	UART_PROTOCOL2_RegisterDataSendService(UART_DRIVE_AddTxArray);

	// 注册间隔参数设置时间
	TIMER_AddTask(TIMER_ID_PROTOCOL_PARAM_TX,
					UART_PROTOCOL2_CMD_SEND_TIME,
					UART_PROTOCOL2_CALLBACK_SetTxPeriodRequest,
					TRUE,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);

 
	// 启动UART总线通讯超时判断
	UART_PROTOCOL2_StartTimeoutCheckTask();
}

// UART协议层过程处理
void UART_PROTOCOL2_Process(void)
{
	// UART接收FIFO缓冲区处理
	UART_PROTOCOL2_RxFIFOProcess(&uartProtocolCB2);

	// UART接收命令缓冲区处理
	UART_PROTOCOL2_CmdFrameProcess(&uartProtocolCB2);
	
	// UART协议层发送处理过程
	UART_PROTOCOL2_TxStateProcess();

	// 码表发送数据通讯时序处理
	UART_PROTOCOL2_DataCommunicationSequenceProcess();

//=========================================================
	// 在八方协议中嵌入KM5S协议
	UART_PROTOCOL_Process();
}

// 向发送命令帧队列中添加数据
void UART_PROTOCOL2_TxAddData(uint8 data)
{
	uint16 head = uartProtocolCB2.tx.head;
	uint16 end =  uartProtocolCB2.tx.end;
	UART_PROTOCOL2_TX_CMD_FRAME* pCmdFrame = &uartProtocolCB2.tx.cmdQueue[uartProtocolCB2.tx.end];

	// 发送缓冲区已满，不予接收
	if((end + 1) % UART_PROTOCOL2_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 队尾命令帧已满，退出
	if(pCmdFrame->length >= UART_PROTOCOL2_TX_CMD_FRAME_LENGTH_MAX)
	{
		return;
	}

	// 数据添加到帧末尾，并更新帧长度
	pCmdFrame->buff[pCmdFrame->length] = data;
	pCmdFrame->length ++;
}

// 确认添加命令帧，即承认之前填充的数据为命令帧，将其添加到发送队列中，由main进行调度发送，本函数内会自动校正命令长度，并添加校验码
void UART_PROTOCOL2_TxAddFrame(void)
{
	uint16 head = uartProtocolCB2.tx.head;
	uint16 end  = uartProtocolCB2.tx.end;
	UART_PROTOCOL2_TX_CMD_FRAME* pCmdFrame = &uartProtocolCB2.tx.cmdQueue[uartProtocolCB2.tx.end];
	uint16 length = pCmdFrame->length;

	// 发送缓冲区已满，不予接收
	if((end + 1) % UART_PROTOCOL2_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 命令帧长度不足，清除已填充的数据，退出
	if(length < UART_PROTOCOL2_TX_CMD_FRAME_LENGTH_MIN)
	{
		pCmdFrame->length = 0;
		
		return;
	}
	// 队尾命令帧已满，退出
	if(length > UART_PROTOCOL2_TX_CMD_FRAME_LENGTH_MAX)
	{
		return;
	}
	
	uartProtocolCB2.tx.end ++;
	uartProtocolCB2.tx.end %= UART_PROTOCOL2_TX_QUEUE_SIZE;
}

// 数据结构初始化
void UART_PROTOCOL2_DataStructInit(UART_PROTOCOL2_CB* pCB)
{
	uint16 i;
	
	// 参数合法性检验
	if (NULL == pCB)
	{
		return;
	}

	pCB->tx.txBusy = FALSE;
	pCB->tx.index = 0;
	pCB->tx.head = 0;
	pCB->tx.end = 0;
	for(i = 0; i < UART_PROTOCOL2_TX_QUEUE_SIZE; i++)
	{
		pCB->tx.cmdQueue[i].length = 0;
	}

	pCB->rxFIFO.head = 0;
	pCB->rxFIFO.end = 0;

	pCB->rx.head = 0;
	pCB->rx.end  = 0;
	for(i=0; i<UART_PROTOCOL2_RX_QUEUE_SIZE; i++)
	{
		pCB->rx.cmdQueue[i].length = 0;
	}
}

// UART报文接收处理函数(注意根据具体模块修改)
void UART_PROTOCOL2_MacProcess(uint16 standarID, uint8* pData, uint16 length)
{
	uint16 end = uartProtocolCB2.rxFIFO.end;
	uint16 head = uartProtocolCB2.rxFIFO.head;
	uint8 rxdata = 0x00;
	
	// 接收数据
	rxdata = *pData;

	// 一级缓冲区已满，不予接收
	if((end + 1)%UART_PROTOCOL2_RX_FIFO_SIZE == head)
	{
		return;
	}
	// 一级缓冲区未满，接收 
	else
	{
		// 将接收到的数据放到临时缓冲区中
		uartProtocolCB2.rxFIFO.buff[end] = rxdata;
		uartProtocolCB2.rxFIFO.end ++;
		uartProtocolCB2.rxFIFO.end %= UART_PROTOCOL2_RX_FIFO_SIZE;
	}	

//====================================================================
	// 借用KM5S协议解析
	UART_PROTOCOL_MacProcess(standarID, pData, length);
}

// UART协议层向驱动层注册数据发送接口
void UART_PROTOCOL2_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length))
{		
	uartProtocolCB2.sendDataThrowService = service;
}

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL UART_PROTOCOL2_ConfirmTempCmdFrameBuff(UART_PROTOCOL2_CB* pCB)
{
	UART_PROTOCOL2_RX_CMD_FRAME* pCmdFrame = NULL;
	
	// 参数合法性检验
	if(NULL == pCB)
	{
		return FALSE;
	}

	// 临时缓冲区为空，不予添加
	pCmdFrame = &pCB->rx.cmdQueue[pCB->rx.end];
	if(0 == pCmdFrame->length)
	{
		return FALSE;
	}

	// 添加
	pCB->rx.end ++;
	pCB->rx.end %= UART_PROTOCOL2_RX_QUEUE_SIZE;
	pCB->rx.cmdQueue[pCB->rx.end].length = 0;	// 该行的作用是将新的添加位置有效数据个数清零，以便将这个位置当做临时帧缓冲区
	
	return TRUE;
}

// 协议层发送处理过程
void UART_PROTOCOL2_TxStateProcess(void)
{
	uint16 head = uartProtocolCB2.tx.head;
	uint16 end =  uartProtocolCB2.tx.end;
	uint16 length = uartProtocolCB2.tx.cmdQueue[head].length;
	uint8* pCmd = uartProtocolCB2.tx.cmdQueue[head].buff;
	uint16 localDeviceID = uartProtocolCB2.tx.cmdQueue[head].deviceID;

	// 发送缓冲区为空，说明无数据
	if (head == end)
	{
		return;
	}

	// 发送函数没有注册直接返回
	if (NULL == uartProtocolCB2.sendDataThrowService)
	{
		return;
	}

	// 协议层有数据需要发送到驱动层		
	if (!(*uartProtocolCB2.sendDataThrowService)(localDeviceID, pCmd, length))
	{
		return;
	}

	// 发送环形队列更新位置
	uartProtocolCB2.tx.cmdQueue[head].length = 0;
	uartProtocolCB2.tx.head ++;
	uartProtocolCB2.tx.head %= UART_PROTOCOL2_TX_QUEUE_SIZE;
}

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void UART_PROTOCOL2_RxFIFOProcess(UART_PROTOCOL2_CB* pCB)
{
	uint16 end = pCB->rxFIFO.end;
	uint16 head = pCB->rxFIFO.head;
	UART_PROTOCOL2_RX_CMD_FRAME* pCmdFrame = NULL;
	
	// 参数合法性检验
	if(NULL == pCB)
	{
		return;
	}
	
	// 一级缓冲区为空，退出
	if(head == end)
	{
		return;
	}

	// 无临时缓冲区可用，退出
	if((pCB->rx.end + 1)%UART_PROTOCOL2_RX_QUEUE_SIZE == pCB->rx.head)
	{
		return;
	}
	
	pCmdFrame = &pCB->rx.cmdQueue[pCB->rx.end];

	// 将数据添加到临时缓冲区中
	pCmdFrame->buff[pCmdFrame->length++] = pCB->rxFIFO.buff[head];

	// 正在接收数据
	uartProtocolCB2.rxFIFO.rxFIFOBusy = TRUE;

	// 添加成功，从一级缓冲区中删除当前数据
	pCB->rxFIFO.head ++;
	pCB->rxFIFO.head %= UART_PROTOCOL2_RX_FIFO_SIZE;

	// 接收到的数据是否达到了期望长度
	if(pCmdFrame->length < pCB->rx.expectLength)
	{
		return;
	}

	// 数据已经接收完毕
	uartProtocolCB2.rxFIFO.rxFIFOBusy = FALSE;

	// 数据长度已经满足，说明接收到了一个完整并且正确的命令帧，此时需要将其扶正
	UART_PROTOCOL2_ConfirmTempCmdFrameBuff(pCB);
}

// UART命令帧缓冲区处理
void UART_PROTOCOL2_CmdFrameProcess(UART_PROTOCOL2_CB* pCB)
{
	UART_PROTOCOL2_CMD cmd = UART_PROTOCOL2_CMD_NULL;
	UART_PROTOCOL2_RX_CMD_FRAME* pCmdFrame = NULL;
	//uint16 i;
	uint32 temp;

	// 参数合法性检验
	if(NULL == pCB)
	{
		return;
	}

	// 命令帧缓冲区为空，退出
	if(pCB->rx.head == pCB->rx.end)
	{
		return;
	}

	// 获取当前要处理的命令帧指针
	pCmdFrame = &pCB->rx.cmdQueue[pCB->rx.head];	

	cmd = pCB->cmdType;
	
	// 执行命令帧
	switch(cmd)
	{
		// 空命令，不予执行
		case UART_PROTOCOL2_CMD_NULL:
			break;

		// 读协议版本
		case UART_PROTOCOL2_CMD_READ_PROTOCOL_VERSION:
			break;

		// 写限速
		case UART_PROTOCOL2_CMD_WRITE_LIMIT_SPEED:
			break;
//============================================================================================
//============================================================================================
		// 读驱动器状态
		case UART_PROTOCOL2_CMD_READ_DRIVER_STATUS:

			// 嘉宏屏蔽掉刹车故障
			if (0x03 == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX])
			{
				break;
			}
			
			PARAM_SetErrorCode((ERROR_TYPE_E)pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX]);

			// 为了通用各种协议，将八方的无错误0x01转成0x00
			if (0x01 == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX])
			{
				
				PARAM_SetErrorCode(ERROR_TYPE_NO_ERROR);
			}
			
			break;

		// 读工作状态
		case UART_PROTOCOL2_CMD_READ_WORK_STATUS:
			if (pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX])
			{
				// 不工作状态
				if (0x30 == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX])
				{
					//PARAM_SetCruiseSwitch(FALSE);
				}
				// 工作状态
				else if (0x31 == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX])
				{
					//PARAM_SetCruiseSwitch(TRUE);
				}
			}			
			break;

		// 读瞬时电流
		case UART_PROTOCOL2_CMD_READ_INSTANTANEOUS_CURRENT:
			if (pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX])
			{
				temp = pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] * 0.5f * 1000;	// 上报的单位为0.5A
				PARAM_SetBatteryCurrent(temp);
			}

			break;

		// 读电池容量
		case UART_PROTOCOL2_CMD_READ_BATTERY_CAPACITY:
			//if (pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX])
			//{
			//	PARAM_SetBatteryPercent(pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX]);
			//}

			if (BATTERY_DATA_SRC_CONTROLLER == PARAM_GetBatteryDataSrc())
			{
				// 没有电压上报的字段，故为了兼容人为采用固定电压
				temp = PARAM_GetBatteryVoltageLevel();
				temp *= 1000;											// 单位转换: 1V转换为1000mV	
				
				PARAM_SetBatteryVoltage(temp);
			}
			else if (BATTERY_DATA_SRC_BMS == PARAM_GetBatteryDataSrc())
			{
				// 电量
				if (pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX])
				{
					PARAM_SetBatteryPercent(pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX]);
				}
			}
			
			break;

		// 读转速
		case UART_PROTOCOL2_CMD_READ_RPM:
			if (((pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] + pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX] + 0x20)&0xFF) == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA3_INDEX])
			{
				double speed;
				
				temp = pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX];
				temp <<= 8;
				temp += pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX];

				//speed = 0.00006f * temp * PARAM_GetPerimeter(PARAM_GetWheelSizeID());
				speed = 0.00006f * temp * PARAM_GetNewperimeter();
				LIMIT_TO_MAX(speed, 99.9);
				PARAM_SetSpeed((uint16)(speed*10));	// 转换为0.1KM/H，传入到参数池

				// 速度不为0，重置定时关机任务
				if(PARAM_GetSpeed() != 0)
				{
					STATE_ResetAutoPowerOffControl();
				}
			}

			break;

		// 读大灯开启值
		case UART_PROTOCOL2_CMD_READ_LIGHT_SWITCH:
			break;
		// 读电池信息1
		case UART_PROTOCOL2_CMD_READ_BATTERY_INFO1:
			break;
		// 读电池信息2
		case UART_PROTOCOL2_CMD_READ_BATTERY_INFO2:
			break;

		// 读剩余里程
		case UART_PROTOCOL2_CMD_READ_REMAINDER_MILEAGE:
			if (pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] + pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX] == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA3_INDEX])
			{
				temp = pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX];
				temp <<= 8;
				temp += pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX];

				#warning "参数层需要增加剩余里程变量"
			}						
			
			break;

		// 读驱动器错误代码
		case UART_PROTOCOL2_CMD_READ_DRIVER_ERROR_CODE:
			break;

		// 读踏频
		case UART_PROTOCOL2_CMD_READ_CADENCE:
			if (pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] + pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX] == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA3_INDEX])
			{
				temp = pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX];
				temp <<= 8;
				temp += pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX];
				
				PARAM_SetCadence(temp);
			}
			break;

		// 读脚踏半圈力矩最大值
		case UART_PROTOCOL2_CMD_READ_TQKG:
			if (pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX] == pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA2_INDEX])
			{
				PARAM_SetTorque(pCmdFrame->buff[UART_PROTOCOL2_CMD_DATA1_INDEX]);
			}
			break;
		
//===================================================
//===================================================
		// 写档位
		case UART_PROTOCOL2_CMD_WRITE_ASSIST_LEVEL:
			break;

		// 写大灯开关
		case UART_PROTOCOL2_CMD_WRITE_LIGHT_SWITCH:
			break;

		// 写清除控制器错误码
		case UART_PROTOCOL2_CMD_WRITE_CLEAR_DRIVER_ERROR_CODE:
			break;
					
		default:
			break;
	}

	// 启动UART总线通讯超时判断
	UART_PROTOCOL2_StartTimeoutCheckTask();
	
	// 删除命令帧
	pCB->rx.head ++;
	pCB->rx.head %= UART_PROTOCOL2_RX_QUEUE_SIZE;
}

// RX通讯超时处理-单向
#if UART_PROTOCOL2_RX_TIME_OUT_CHECK_ENABLE
void UART_PROTOCOL2_CALLBACK_RxTimeOut(uint32 param)
{
	UART_PROTOCOL2_RX_CMD_FRAME* pCmdFrame = NULL;

	pCmdFrame = &uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end];

	// 超时错误，将命令帧长度清零，即认为抛弃该命令帧
	pCmdFrame->length = 0;	// 2016.1.6增加
	// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
	uartProtocolCB2.rxFIFO.head ++;
	uartProtocolCB2.rxFIFO.head %= UART_PROTOCOL2_RX_FIFO_SIZE;
	uartProtocolCB2.rxFIFO.currentProcessIndex = uartProtocolCB2.rxFIFO.head;
}

// 停止Rx通讯超时检测任务
void UART_PROTOCOL2_StopRxTimeOutCheck(void)
{
	TIMER_KillTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL);
}
#endif

// 码表设置立刻下发数据请求标志
void UART_PROTOCOL2_SetTxAtOnceRequest(uint32 param)
{
	uartProtocolCB2.txAtOnceRequest = (BOOL)param;
}

// 码表设置立刻下发命令类型
void UART_PROTOCOL2_SetTxAtOnceCmdType(UART_PROTOCOL2_TX_AT_ONCE_CMD_TYPE type)
{
	uartProtocolCB2.txAtOnceCmdType = type;
}

// 码表设置周期下发数据请求标志
void UART_PROTOCOL2_CALLBACK_SetTxPeriodRequest(uint32 param)
{
	uartProtocolCB2.txPeriodRequest = (BOOL)param;
}

// 	UART总线超时错误处理
void UART_PROTOCOL2_CALLBACK_UartBusError(uint32 param)
{
	PARAM_SetErrorCode(ERROR_TYPE_COMMUNICATION_TIME_OUT);
}

// 码表发送数据通讯时序处理
void UART_PROTOCOL2_DataCommunicationSequenceProcess(void)
{
	// 如果正在接收数据，不允许中途打断
	//if (uartProtocolCB2.rxFIFO.rxFIFOBusy)
	//{
	//	return;
	//}
	
	// 立即发送请求
	if (uartProtocolCB2.txAtOnceRequest)
	{
		// 重新复位参数下发定时器
		TIMER_ResetTimer(TIMER_ID_PROTOCOL_PARAM_TX);

		uartProtocolCB2.txAtOnceRequest = FALSE;
#if 0
		if (UART_PROTOCOL2_TX_CMD_TYPE_ASSIST == uartProtocolCB2.txAtOnceCmdType)
		{
			// 写档位
			UART_PROTOCOL2_WriteAssistLevel(PARAM_GetMaxAssist(), PARAM_GetAssistLevel());
		}
		else if (UART_PROTOCOL2_TX_CMD_TYPE_LIGHT == uartProtocolCB2.txAtOnceCmdType)
		{
			// 写大灯
			UART_PROTOCOL2_WriteLightSwitch(PARAM_GetLightSwitch());
		}
#endif
		// 写档位
		UART_PROTOCOL2_WriteAssistLevel(PARAM_GetMaxAssist(), PARAM_GetAssistLevel());
		// 写大灯状态
		UART_PROTOCOL2_WriteLightSwitch(PARAM_GetLightSwitch());
		// 写限速
		UART_PROTOCOL2_WriteLimitSpeed(paramCB.nvm.param.common.speed.limitVal);

		return;
	}

	// 周期性发送请求
	if (uartProtocolCB2.txPeriodRequest)
	{
		uartProtocolCB2.txPeriodRequest = FALSE;

		UART_PROTOCOL2_DataInteractionProcess(TRUE);		
	}	
}

// 和控制器数据交互步骤处理
void UART_PROTOCOL2_DataInteractionProcess(uint32 param)
{
	switch (uartProtocolCB2.step)
	{
		case 0:
			// 读取协议版本
			UART_PROTOCOL2_ReadProtocolVersion(TRUE);

			uartProtocolCB2.step ++;
			
			break;

		case 1:			
			// 写档位
			UART_PROTOCOL2_WriteAssistLevel(PARAM_GetMaxAssist(), PARAM_GetAssistLevel());
			// 写大灯状态
			UART_PROTOCOL2_WriteLightSwitch(PARAM_GetLightSwitch());
			// 写限速
			UART_PROTOCOL2_WriteLimitSpeed(paramCB.nvm.param.common.speed.limitVal);

			uartProtocolCB2.step ++;
			
			break;

		case 2:			
			// 留出这段时间，让写档位大灯限速有充分的时间，防止控制器异常
			uartProtocolCB2.step ++;
			
			break;

		case 3:
			// 读取转速
			UART_PROTOCOL2_ReadRPM(TRUE);

			uartProtocolCB2.step ++;
			
			break;

		case 4:
			// 读驱动器状态
			UART_PROTOCOL2_ReadDriverStatus(TRUE);

			uartProtocolCB2.step ++;
			
			break;

		case 5:
			// 读取电池容量
			UART_PROTOCOL2_ReadBatteryCapacity(TRUE);

			uartProtocolCB2.step ++;
			
			break;
			
		case 6:
			// 读系统工作状态
			UART_PROTOCOL2_ReadSysWorkStatus(TRUE);
		
			uartProtocolCB2.step ++;
			
			break;
			

		case 7:
			// 读瞬时电流
			UART_PROTOCOL2_ReadInstantaneousCurrent(TRUE);

			uartProtocolCB2.step = 1;
			
			break;

		default:
			uartProtocolCB2.step = 1;
			break;	
	}
}

// 读取协议版本
void UART_PROTOCOL2_ReadProtocolVersion(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_PROTOCOL_VERSION;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 3;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_PROTOCOL_VERSION&0xFF00)>>8);

	// 命令为读协议版本
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_PROTOCOL_VERSION&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
	
}

// 读取驱动器状态
void UART_PROTOCOL2_ReadDriverStatus(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_DRIVER_STATUS;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 1;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_DRIVER_STATUS&0xFF00)>>8);

	// 命令为读驱动器状态
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_DRIVER_STATUS&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取系统工作状态
void UART_PROTOCOL2_ReadSysWorkStatus(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_WORK_STATUS;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 2;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_WORK_STATUS&0xFF00)>>8);

	// 命令为读系统工作状态
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_WORK_STATUS&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取瞬时电流
void UART_PROTOCOL2_ReadInstantaneousCurrent(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_INSTANTANEOUS_CURRENT;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 2;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_INSTANTANEOUS_CURRENT&0xFF00)>>8);

	// 命令为读瞬时电流
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_INSTANTANEOUS_CURRENT&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取电池容量
void UART_PROTOCOL2_ReadBatteryCapacity(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_BATTERY_CAPACITY;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 2;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_BATTERY_CAPACITY&0xFF00)>>8);

	// 命令为读电池容量
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_BATTERY_CAPACITY&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取车轮转速
void UART_PROTOCOL2_ReadRPM(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_RPM;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 3;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_RPM&0xFF00)>>8);

	// 命令为读取车轮转速
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_RPM&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取大灯值
void UART_PROTOCOL2_ReadLightSwitch(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_LIGHT_SWITCH;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 2;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_LIGHT_SWITCH&0xFF00)>>8);

	// 命令为读取大灯值
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_LIGHT_SWITCH&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取电池信息1
void UART_PROTOCOL2_ReadBatteryInfo1(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_BATTERY_INFO1;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 27;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_BATTERY_INFO1&0xFF00)>>8);

	// 命令为读取电池信息1
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_BATTERY_INFO1&0x00FF);

	// 校验和
	UART_PROTOCOL2_TxAddData(((UART_PROTOCOL2_CMD_READ_BATTERY_INFO1&0xFF00)>>8)+(UART_PROTOCOL2_CMD_READ_BATTERY_INFO1&0x00FF));

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取电池信息2
void UART_PROTOCOL2_ReadBatteryInfo2(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_BATTERY_INFO2;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = (13+1)*2;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_BATTERY_INFO2&0xFF00)>>8);

	// 命令为读取电池信息2
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_BATTERY_INFO2&0x00FF);

	// 校验和
	UART_PROTOCOL2_TxAddData(((UART_PROTOCOL2_CMD_READ_BATTERY_INFO2&0xFF00)>>8)+(UART_PROTOCOL2_CMD_READ_BATTERY_INFO2&0x00FF));

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取剩余里程
void UART_PROTOCOL2_ReadRemainderMileage(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_REMAINDER_MILEAGE;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 3;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_REMAINDER_MILEAGE&0xFF00)>>8);

	// 命令为读取剩余里程
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_REMAINDER_MILEAGE&0x00FF);

	// 校验和
	UART_PROTOCOL2_TxAddData(((UART_PROTOCOL2_CMD_READ_REMAINDER_MILEAGE&0xFF00)>>8)+(UART_PROTOCOL2_CMD_READ_REMAINDER_MILEAGE&0x00FF));

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取控制器错误码
void UART_PROTOCOL2_ReadDriverErrorCode(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_DRIVER_ERROR_CODE;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 11;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_DRIVER_ERROR_CODE&0xFF00)>>8);

	// 命令为读取控制器错误码
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_DRIVER_ERROR_CODE&0x00FF);

	// 校验和
	UART_PROTOCOL2_TxAddData(((UART_PROTOCOL2_CMD_READ_DRIVER_ERROR_CODE&0xFF00)>>8)+(UART_PROTOCOL2_CMD_READ_DRIVER_ERROR_CODE&0x00FF));

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取踏频
void UART_PROTOCOL2_ReadCadence(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_CADENCE;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 3;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_CADENCE&0xFF00)>>8);

	// 命令为读取踏频
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_CADENCE&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 读取脚踏半圈力矩最大值命令
void UART_PROTOCOL2_ReadTQKG(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_READ_TQKG;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 2;
	
	// 读取指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_READ_TQKG&0xFF00)>>8);

	// 命令为读取脚踏半圈力矩最大值命令
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_READ_TQKG&0x00FF);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

//============================================================
// 写限速
void UART_PROTOCOL2_WriteLimitSpeed(uint16 param)
{
	uint8 checkVal;
	//uint16 perimeter = PARAM_GetPerimeter(PARAM_GetWheelSizeID());
	uint16 perimeter = PARAM_GetNewperimeter();
	uint16 speedRpm = (uint16)(param*100000.0f/perimeter/60 + 0.5);

	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_WRITE_LIMIT_SPEED;
	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 0;
	
	// 写指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_WRITE_LIMIT_SPEED&0xFF00)>>8);

	// 命令为写限速
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_WRITE_LIMIT_SPEED&0x00FF);

	UART_PROTOCOL2_TxAddData(speedRpm>>8);
	UART_PROTOCOL2_TxAddData(speedRpm&0x00FF);

	checkVal = ((UART_PROTOCOL2_CMD_WRITE_LIMIT_SPEED&0xFF00)>>8) + (UART_PROTOCOL2_CMD_WRITE_LIMIT_SPEED&0x00FF) + (speedRpm>>8) + (speedRpm&0x00FF); 
	
	// 校验和
	UART_PROTOCOL2_TxAddData(checkVal);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 写档位
void UART_PROTOCOL2_WriteAssistLevel(uint8 maxAssistLevel, uint8 assistLevel)
{
	uint8 assistCode;
	uint8 checkVal;
	
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_WRITE_ASSIST_LEVEL;
	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 0;
	
	// 写指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_WRITE_ASSIST_LEVEL&0xFF00)>>8);

	// 命令为写档位
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_WRITE_ASSIST_LEVEL&0x00FF);

	switch (maxAssistLevel)
	{
		case 3:
			switch (assistLevel)
			{
				case ASSIST_0:
					assistCode = 0;
					break;

				case ASSIST_1:
					assistCode = 12;
					break;

				case ASSIST_2:
					assistCode = 2;
					break;

				case ASSIST_3:
					assistCode = 3;
					break;

				// 推车
				case ASSIST_P:
					assistCode = 6;
					break;

				// 限速
				case ASSIST_S:
					assistCode = 10;
					break;
			}
			break;

		case 5:
			switch (assistLevel)
			{
				case ASSIST_0:
					assistCode = 0;
					break;

				case ASSIST_1:
					assistCode = 11;
					break;

				case ASSIST_2:
					assistCode = 13;
					break;

				case ASSIST_3:
					assistCode = 21;
					break;

				case ASSIST_4:
					assistCode = 23;
					break;

				case ASSIST_5:
					assistCode = 3;
					break;

				// 推车
				case ASSIST_P:
					assistCode = 6;
					break;

				// 限速
				case ASSIST_S:
					assistCode = 10;
					break;
			}
			break;

		case 9:
			switch (assistLevel)
			{
				case ASSIST_0:
					assistCode = 0;
					break;

				case ASSIST_1:
					assistCode = 1;
					break;

				case ASSIST_2:
					assistCode = 11;
					break;

				case ASSIST_3:
					assistCode = 12;
					break;

				case ASSIST_4:
					assistCode = 13;
					break;

				case ASSIST_5:
					assistCode = 2;
					break;

				case ASSIST_6:
					assistCode = 21;
					break;

				case ASSIST_7:
					assistCode = 22;
					break;

				case ASSIST_8:
					assistCode = 23;
					break;

				case ASSIST_9:
					assistCode = 3;
					break;

				// 推车
				case ASSIST_P:
					assistCode = 6;
					break;

				// 限速
				case ASSIST_S:
					assistCode = 10;
					break;
			}
			break;
	}
	UART_PROTOCOL2_TxAddData(assistCode);

	checkVal = ((UART_PROTOCOL2_CMD_WRITE_ASSIST_LEVEL&0xFF00)>>8) + (UART_PROTOCOL2_CMD_WRITE_ASSIST_LEVEL&0x00FF) + assistCode; 
	
	// 校验和
	UART_PROTOCOL2_TxAddData(checkVal);

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 写大灯
void UART_PROTOCOL2_WriteLightSwitch(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_WRITE_LIGHT_SWITCH;
	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 0;
	
	// 写指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_WRITE_LIGHT_SWITCH&0xFF00)>>8);

	// 命令为写大灯
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_WRITE_LIGHT_SWITCH&0x00FF);

	if (param)
	{
		UART_PROTOCOL2_TxAddData(0xF1);	// 开大灯
	}
	else
	{
		UART_PROTOCOL2_TxAddData(0xF0);	// 关大灯
	}

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}

// 写清除控制器错误码
void UART_PROTOCOL2_WriteClearDriverErrorCode(uint8 param)
{
	// 设置命令类型
	uartProtocolCB2.cmdType = UART_PROTOCOL2_CMD_WRITE_CLEAR_DRIVER_ERROR_CODE;

	// 设置接收数据长度
	uartProtocolCB2.rx.expectLength = 2;
	
	
	// 写指令
	UART_PROTOCOL2_TxAddData((UART_PROTOCOL2_CMD_WRITE_CLEAR_DRIVER_ERROR_CODE&0xFF00)>>8);

	// 命令为写清除控制器错误码
	UART_PROTOCOL2_TxAddData(UART_PROTOCOL2_CMD_WRITE_CLEAR_DRIVER_ERROR_CODE&0x00FF);

	// 校验和
	UART_PROTOCOL2_TxAddData(((UART_PROTOCOL2_CMD_WRITE_CLEAR_DRIVER_ERROR_CODE&0xFF00)>>8) + (UART_PROTOCOL2_CMD_WRITE_CLEAR_DRIVER_ERROR_CODE&0x00FF));

	// 组成完整命令帧
	UART_PROTOCOL2_TxAddFrame();


	// 清空一级接收队列以及命令帧队列
	uartProtocolCB2.rxFIFO.head = uartProtocolCB2.rxFIFO.end;
	uartProtocolCB2.rx.head = uartProtocolCB2.rx.end;
	uartProtocolCB2.rx.cmdQueue[uartProtocolCB2.rx.end].length = 0;
}


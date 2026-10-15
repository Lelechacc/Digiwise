#include "common.h"
#include "timer.h"
#include "delay.h"
#include "uartDrive.h"
#include "uartProtocol.h"
#include "state.h"
#include "Param.h"
#include "iap.h"
#include "lcdTest.h"
#include "keyMsgProcess.h"
#include "lcd.h"
#include "BleUart.h"

/******************************************************************************
* 【内部接口声明】
******************************************************************************/

// 数据结构初始化
void UART_PROTOCOL_DataStructInit(UART_PROTOCOL_CB* pCB);

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void UART_PROTOCOL_RxFIFOProcess(UART_PROTOCOL_CB* pCB);

// UART命令帧缓冲区处理
void UART_PROTOCOL_CmdFrameProcess(UART_PROTOCOL_CB* pCB);

// 对传入的命令帧进行校验，返回校验结果
BOOL UART_PROTOCOL_CheckSUM(UART_PROTOCOL_RX_CMD_FRAME* pCmdFrame);

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL UART_PROTOCOL_ConfirmTempCmdFrameBuff(UART_PROTOCOL_CB* pCB);

// 通讯超时处理-单向
void UART_PROTOCOL_CALLBACK_RxTimeOut(uint32 param);

// 停止RX通讯超时检测任务
void UART_PROTOCOL_StopRxTimeOutCheck(void);

// 协议层发送处理过程
void UART_PROTOCOL_TxStateProcess(void);

// UART协议层向驱动层注册数据发送接口
void UART_PROTOCOL_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length));

//===============================================================================================================
// 码表设置周期下发数据请求标志
void UART_PROTOCOL_CALLBACK_SetTxPeriodRequest(uint32 param);

// 码表发送数据通讯时序处理
void UART_PROTOCOL_DataCommunicationSequenceProcess(void);

// 启动通讯超时判断任务
void UART_PROTOCOL_StartTimeoutCheckTask(void);

// UART总线超时错误处理
void UART_PROTOCOL_CALLBACK_UartBusError(uint32 param);

//=========================================================================
// 接收驱动器上报数据
void UART_PROTOCOL_OnReceiveReport(uint8* pBuff, uint32 len);

// 发送配置参数命令到驱动器
void UART_PROTOCOL_SendConfigParam(void);

// 发送运行参数命令到驱动器
void UART_PROTOCOL_SendRuntimeParam(void);

// 上报写配置参数结果
void UART_PROTOCOL_ReportWriteParamResult(uint32 param);

// 上报配置参数
void UART_PROTOCOL_ReportConfigureParam(uint32 param);

// 发送命令带结果
void UART_PROTOCOL_SendCmdWithResult(uint8 cmdWord, uint8 result);

// 发送按键值
void UART_PROTOCOL_SendCmdCheckKeyValue(uint32 param);

//==================================================================================
// 速度平滑滤波处理
void UART_PROTOCOL_CALLBACK_SpeedFilterProcess(uint32 param);


// 全局变量定义
UART_PROTOCOL_CB uartProtocolCB;

UART_PROTOCOL_PARAM_CB hmiDriveCB;

// ■■函数定义区■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// 启动通讯超时判断任务
void UART_PROTOCOL_StartTimeoutCheckTask(void)
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
					UART_PROTOCOL_CALLBACK_UartBusError,
					TRUE,
					1,
					ACTION_MODE_ADD_TO_QUEUE);
#endif

	// 到此处证明通信与命令帧符合要求，则设置电量获取方式正常
	PARAM_SetBatteryDataSrc((BATTERY_DATA_SRC_E)PARAM_GetPercentageMethod());
}

// 协议初始化
void UART_PROTOCOL_Init(void)
{
	// 协议层数据结构初始化
	UART_PROTOCOL_DataStructInit(&uartProtocolCB);

	// 向驱动层注册数据接收接口
	UART_DRIVE_RegisterDataSendService(UART_PROTOCOL_MacProcess);

	// 向驱动层注册数据发送接口
	UART_PROTOCOL_RegisterDataSendService(UART_DRIVE_AddTxArray);

	// 注册间隔参数设置时间
	TIMER_AddTask(TIMER_ID_PROTOCOL_PARAM_TX,
					UART_PROTOCOL_CMD_SEND_TIME,
					UART_PROTOCOL_CALLBACK_SetTxPeriodRequest,
					TRUE,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);

	// 启动UART总线通讯超时判断
	UART_PROTOCOL_StartTimeoutCheckTask();

}



// UART协议层过程处理
void UART_PROTOCOL_Process(void)
{
	// UART接收FIFO缓冲区处理
	UART_PROTOCOL_RxFIFOProcess(&uartProtocolCB);

	// UART接收命令缓冲区处理
	UART_PROTOCOL_CmdFrameProcess(&uartProtocolCB);
	
	// UART协议层发送处理过程
	UART_PROTOCOL_TxStateProcess();

	// 码表发送数据通讯时序处理
	UART_PROTOCOL_DataCommunicationSequenceProcess();
}

// 向发送命令帧队列中添加数据
void UART_PROTOCOL_TxAddData(uint8 data)
{
	uint16 head = uartProtocolCB.tx.head;
	uint16 end =  uartProtocolCB.tx.end;
	UART_PROTOCOL_TX_CMD_FRAME* pCmdFrame = &uartProtocolCB.tx.cmdQueue[uartProtocolCB.tx.end];

	// 发送缓冲区已满，不予接收
	if((end + 1) % UART_PROTOCOL_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 队尾命令帧已满，退出
	if(pCmdFrame->length >= UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX)
	{
		return;
	}

	// 数据添加到帧末尾，并更新帧长度
	pCmdFrame->buff[pCmdFrame->length] = data;
	pCmdFrame->length ++;
}

// 确认添加命令帧，即承认之前填充的数据为命令帧，将其添加到发送队列中，由main进行调度发送，本函数内会自动校正命令长度，并添加校验码
void UART_PROTOCOL_TxAddFrame(void)
{
	uint16 checkSum = 0;
	uint16 i = 0;
	uint16 head = uartProtocolCB.tx.head;
	uint16 end  = uartProtocolCB.tx.end;
	UART_PROTOCOL_TX_CMD_FRAME* pCmdFrame = &uartProtocolCB.tx.cmdQueue[uartProtocolCB.tx.end];
	uint16 length = pCmdFrame->length;

	// 发送缓冲区已满，不予接收
	if((end + 1) % UART_PROTOCOL_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 命令帧长度不足，清除已填充的数据，退出
	if(UART_PROTOCOL_CMD_FRAME_LENGTH_MIN-4 > length)	// 减去"校验和L、校验和H、结束标识0xD、结束标识OxA"4个字节
	{
		pCmdFrame->length = 0;
		
		return;
	}

	// 队尾命令帧已满，退出
	if((length >= UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX) || (length+1 >= UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX)
	|| (length+2 >= UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX) || (length+3 >= UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX))
	{
		pCmdFrame->length = 0;
		
		return;
	}

	// 重新设置数据长度，系统在准备数据时，填充的"数据长度"可以为任意值，并且不需要添加校验码，在这里重新设置为正确的值
	pCmdFrame->buff[UART_PROTOCOL_CMD_LENGTH_INDEX] = length - 4;	// 重设数据长度，减去"命令头、设备地址、命令字、数据长度"4个字节
	for(i=1; i<length; i++)
	{
		checkSum += pCmdFrame->buff[i];
	}
	pCmdFrame->buff[pCmdFrame->length++] = (checkSum & 0x00FF);			// 低字节在前
	pCmdFrame->buff[pCmdFrame->length++] = ((checkSum >> 8) & 0x00FF);	// 高字节在后

	// 结束标识
	pCmdFrame->buff[pCmdFrame->length++] = 0x0D;
	pCmdFrame->buff[pCmdFrame->length++] = 0x0A;

	uartProtocolCB.tx.end ++;
	uartProtocolCB.tx.end %= UART_PROTOCOL_TX_QUEUE_SIZE;
	//pCB->tx.cmdQueue[pCB->tx.end].length = 0;   //2015.12.2修改
}

// 数据结构初始化
void UART_PROTOCOL_DataStructInit(UART_PROTOCOL_CB* pCB)
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
	for(i = 0; i < UART_PROTOCOL_TX_QUEUE_SIZE; i++)
	{
		pCB->tx.cmdQueue[i].length = 0;
	}

	pCB->rxFIFO.head = 0;
	pCB->rxFIFO.end = 0;
	pCB->rxFIFO.currentProcessIndex = 0;

	pCB->rx.head = 0;
	pCB->rx.end  = 0;
	for(i=0; i<UART_PROTOCOL_RX_QUEUE_SIZE; i++)
	{
		pCB->rx.cmdQueue[i].length = 0;
	}
}

// UART报文接收处理函数(注意根据具体模块修改)
void UART_PROTOCOL_MacProcess(uint16 standarID, uint8* pData, uint16 length)
{
	uint16 end = uartProtocolCB.rxFIFO.end;
	uint16 head = uartProtocolCB.rxFIFO.head;
	uint8 rxdata = 0x00;
	
	// 接收数据
	rxdata = *pData;

	// 一级缓冲区已满，不予接收
	if((end + 1)%UART_PROTOCOL_RX_FIFO_SIZE == head)
	{
		return;
	}
	// 一级缓冲区未满，接收 
	else
	{
		// 将接收到的数据放到临时缓冲区中
		uartProtocolCB.rxFIFO.buff[end] = rxdata;
		uartProtocolCB.rxFIFO.end ++;
		uartProtocolCB.rxFIFO.end %= UART_PROTOCOL_RX_FIFO_SIZE;
	}	
}

// UART协议层向驱动层注册数据发送接口
void UART_PROTOCOL_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length))
{		
	uartProtocolCB.sendDataThrowService = service;
}

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL UART_PROTOCOL_ConfirmTempCmdFrameBuff(UART_PROTOCOL_CB* pCB)
{
	UART_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;
	
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
	pCB->rx.end %= UART_PROTOCOL_RX_QUEUE_SIZE;
	pCB->rx.cmdQueue[pCB->rx.end].length = 0;	// 该行的作用是将新的添加位置有效数据个数清零，以便将这个位置当做临时帧缓冲区
	
	return TRUE;
}

// 协议层发送处理过程
void UART_PROTOCOL_TxStateProcess(void)
{
	uint16 head = uartProtocolCB.tx.head;
	uint16 end =  uartProtocolCB.tx.end;
	uint16 length = uartProtocolCB.tx.cmdQueue[head].length;
	uint8* pCmd = uartProtocolCB.tx.cmdQueue[head].buff;
	uint16 localDeviceID = uartProtocolCB.tx.cmdQueue[head].deviceID;

	// 发送缓冲区为空，说明无数据
	if (head == end)
	{
		return;
	}

	// 发送函数没有注册直接返回
	if (NULL == uartProtocolCB.sendDataThrowService)
	{
		return;
	}

	// 协议层有数据需要发送到驱动层		
	if (!(*uartProtocolCB.sendDataThrowService)(localDeviceID, pCmd, length))
	{
		return;
	}

	// 发送环形队列更新位置
	uartProtocolCB.tx.cmdQueue[head].length = 0;
	uartProtocolCB.tx.head ++;
	uartProtocolCB.tx.head %= UART_PROTOCOL_TX_QUEUE_SIZE;
}

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void UART_PROTOCOL_RxFIFOProcess(UART_PROTOCOL_CB* pCB)
{
	uint16 end = pCB->rxFIFO.end;
	uint16 head = pCB->rxFIFO.head;
	UART_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;
	uint16 length = 0;
	uint8 currentData = 0;
	
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

	// 获取临时缓冲区指针
	pCmdFrame = &pCB->rx.cmdQueue[pCB->rx.end];
	// 取出当前要处理的字节
	currentData = pCB->rxFIFO.buff[pCB->rxFIFO.currentProcessIndex];
	
	// 临时缓冲区长度为0时，搜索首字节
	if(0 == pCmdFrame->length)
	{
		// 命令头错误，删除当前字节并退出
		if(UART_PROTOCOL_CMD_HEAD != currentData)
		{
			pCB->rxFIFO.head ++;
			pCB->rxFIFO.head %= UART_PROTOCOL_RX_FIFO_SIZE;
			pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;

			return;
		}
		
		// 命令头正确，但无临时缓冲区可用，退出
		if((pCB->rx.end + 1)%UART_PROTOCOL_RX_QUEUE_SIZE == pCB->rx.head)
		{
			return;
		}

		// 添加UART通讯超时时间设置-2016.1.5增加
#if UART_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
		TIMER_AddTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL,
						UART_PROTOCOL_BUS_UNIDIRECTIONAL_TIME_OUT,
						UART_PROTOCOL_CALLBACK_RxTimeOut,
						0,
						1,
						ACTION_MODE_ADD_TO_QUEUE);
#endif
		
		// 命令头正确，有临时缓冲区可用，则将其添加到命令帧临时缓冲区中
		pCmdFrame->buff[pCmdFrame->length++]= currentData;
		pCB->rxFIFO.currentProcessIndex ++;
		pCB->rxFIFO.currentProcessIndex %= UART_PROTOCOL_RX_FIFO_SIZE;
	}
	// 非首字节，将数据添加到命令帧临时缓冲区中，但暂不删除当前数据
	else
	{
		// 临时缓冲区溢出，说明当前正在接收的命令帧是错误的，正确的命令帧不会出现长度溢出的情况
		if(pCmdFrame->length >= UART_PROTOCOL_RX_CMD_FRAME_LENGTH_MAX)
		{
#if UART_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
			// 停止RX通讯超时检测
			UART_PROTOCOL_StopRxTimeOutCheck();
#endif

			// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
			pCmdFrame->length = 0;	// 2016.1.5增加
			// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
			pCB->rxFIFO.head ++;
			pCB->rxFIFO.head %= UART_PROTOCOL_RX_FIFO_SIZE;
			pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;

			return;
		}

		// 一直取到末尾
		while(end != pCB->rxFIFO.currentProcessIndex)
		{
			// 取出当前要处理的字节
			currentData = pCB->rxFIFO.buff[pCB->rxFIFO.currentProcessIndex];
			// 缓冲区未溢出，正常接收，将数据添加到临时缓冲区中
			pCmdFrame->buff[pCmdFrame->length++] = currentData;
			pCB->rxFIFO.currentProcessIndex ++;
			pCB->rxFIFO.currentProcessIndex %= UART_PROTOCOL_RX_FIFO_SIZE;

			// ■■接下来，需要检查命令帧是否完整，如果完整，则将命令帧临时缓冲区扶正■■
			
			// 首先判断命令帧最小长度，一个完整的命令字至少包括8个字节: 命令头、设备地址、命令字、数据长度、校验和L、校验和H、结束标识0xD、结束标识OxA，因此不足8个字节的必定不完整
			if(pCmdFrame->length < UART_PROTOCOL_CMD_FRAME_LENGTH_MIN)
			{
				// 继续接收
				continue;
			}

			// 命令帧长度数值越界，说明当前命令帧错误，停止接收
			if(pCmdFrame->buff[UART_PROTOCOL_CMD_LENGTH_INDEX] > UART_PROTOCOL_RX_CMD_FRAME_LENGTH_MAX-UART_PROTOCOL_CMD_FRAME_LENGTH_MIN)
			{
#if UART_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
				// 停止RX通讯超时检测
				UART_PROTOCOL_StopRxTimeOutCheck();
#endif
			
				// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
				pCmdFrame->length = 0;
				// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
				pCB->rxFIFO.head ++;
				pCB->rxFIFO.head %= UART_PROTOCOL_RX_FIFO_SIZE;
				pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;

				return;
			}

			// 命令帧长度校验，在命令长度描述字的数值上，增加命令头、设备地址、命令字、数据长度、校验和L、校验和H、结束标识0xD、结束标识OxA，即为命令帧实际长度
			length = pCmdFrame->length;
			if(length < pCmdFrame->buff[UART_PROTOCOL_CMD_LENGTH_INDEX] + UART_PROTOCOL_CMD_FRAME_LENGTH_MIN)
			{
				// 长度要求不一致，说明未接收完毕，退出继续
				continue;
			}

			// 命令帧长度OK，则进行校验，失败时删除命令头
			if(!UART_PROTOCOL_CheckSUM(pCmdFrame))
			{
#if UART_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
				// 停止RX通讯超时检测
				UART_PROTOCOL_StopRxTimeOutCheck();
#endif
				
				// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
				pCmdFrame->length = 0;
				// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
				pCB->rxFIFO.head ++;
				pCB->rxFIFO.head %= UART_PROTOCOL_RX_FIFO_SIZE;
				pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;
				
				return;
			}

#if UART_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
			// 停止RX通讯超时检测
			UART_PROTOCOL_StopRxTimeOutCheck();
#endif
			
			// 执行到这里，即说明接收到了一个完整并且正确的命令帧，此时需将处理过的数据从一级缓冲区中删除，并将该命令帧扶正
			pCB->rxFIFO.head += length;
			pCB->rxFIFO.head %= UART_PROTOCOL_RX_FIFO_SIZE;
			pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;
			UART_PROTOCOL_ConfirmTempCmdFrameBuff(pCB);

			return;
		}
	}
}

// 对传入的命令帧进行校验，返回校验结果
BOOL UART_PROTOCOL_CheckSUM(UART_PROTOCOL_RX_CMD_FRAME* pCmdFrame)
{
	uint16 checkSum = 0;
	uint16 sumTemp;
	uint16 i = 0;
	
	if(NULL == pCmdFrame)
	{
		return FALSE;
	}

	// 从设备地址开始，到校验码之前的一个字节，依次进行累加运算
	for(i=1; i<pCmdFrame->length-4; i++)
	{
		checkSum += pCmdFrame->buff[i];
	}

	// 累加和，低字节在前，高字节在后
	sumTemp = pCmdFrame->buff[pCmdFrame->length-3];		// 高字节
	sumTemp <<= 8;
	sumTemp += pCmdFrame->buff[pCmdFrame->length-4];	// 低字节
	
	// 判断计算得到的校验码与命令帧中的校验码是否相同
	if(sumTemp != checkSum)
	{
		return FALSE;
	}
	
	return TRUE;
}

// UART命令帧缓冲区处理
void UART_PROTOCOL_CmdFrameProcess(UART_PROTOCOL_CB* pCB)
{
	UART_PROTOCOL_CMD cmd = UART_PROTOCOL_CMD_NULL;
	UART_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;
	uint32 tempVal;
	uint32 tempVal2;
	uint32 tempVal3;
	//uint32 tempseg;
	//uint8 length;
	uint8 i;
	uint8 *tempBuf;
	uint8 flag;
	uint32  temp1,temp2,temp3;
	uint16 powerPassword;
	uint16 menuPassword;
	RET_RESULT lcdTestFlag;
	uint16 value, value1;
	uint16 tempSeg[50] = {0};
	
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
	
	// 命令头非法，退出
	if(UART_PROTOCOL_CMD_HEAD != pCmdFrame->buff[UART_PROTOCOL_CMD_HEAD_INDEX])
	{
		// 删除命令帧
		pCB->rx.head ++;
		pCB->rx.head %= UART_PROTOCOL_RX_QUEUE_SIZE;
		return;
	}

	// 命令头合法，则提取命令
	cmd = (UART_PROTOCOL_CMD)pCmdFrame->buff[UART_PROTOCOL_CMD_CMD_INDEX];

	// 执行命令帧
	switch(cmd)
	{
		// 空命令，不予执行
		case UART_PROTOCOL_CMD_NULL:
			break;

		// 参数设置
		case UART_PROTOCOL_CMD_HMI_CTRL_PARAM_SET:
		
			// 参数设置成功
			uartProtocolCB.paramSetOK = TRUE;
			
			break;
			
		// 正常运行状态
		case UART_PROTOCOL_CMD_HMI_CTRL_RUN:

			UART_PROTOCOL_OnReceiveReport(&pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX], UART_PROTOCOL_RX_PARAM_SIZE);

			// 老化接收次数也借用此命令字
			PARAM_SetRxCount(PARAM_GetRxCount() + 1);

			// 数据长度为2说明是仪表对接发送的数据，则进入老化，那么关闭定时关机时间
			if (2 == pCmdFrame->buff[UART_PROTOCOL_CMD_LENGTH_INDEX])
			{
				TIMER_KillTask(TIMER_ID_AUTO_POWER_OFF);
			}
						
			break;

		// 上位机写参数
		case UART_PROTOCOL_CMD_WRITE_CONTROL_PARAM:
		
			// 背光亮度
			if ((pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX] >= 1) && (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX] <= 5))
			{
				PARAM_SetBrightness(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX]);
			}

			// 休眠时间
			if (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA2_INDEX] <= 10)
			{
				PARAM_SetPowerOffTime(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA2_INDEX]);
			}

			// 系统时间，年月日时分
			if ((pCmdFrame->buff[UART_PROTOCOL_CMD_DATA3_INDEX] <= 99) 
			&& (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA4_INDEX] >= 1) && (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA4_INDEX] <= 12)
			&& (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA5_INDEX] >= 1) && (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA5_INDEX] <= 31)
			&& (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA6_INDEX] <= 23)
			&& (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA7_INDEX] <= 59))
			{
				/*DS1302_SetDate(2000+pCmdFrame->buff[UART_PROTOCOL_CMD_DATA3_INDEX], 
							   pCmdFrame->buff[UART_PROTOCOL_CMD_DATA4_INDEX],
							   pCmdFrame->buff[UART_PROTOCOL_CMD_DATA5_INDEX],
							   pCmdFrame->buff[UART_PROTOCOL_CMD_DATA6_INDEX],
							   pCmdFrame->buff[UART_PROTOCOL_CMD_DATA7_INDEX],
							   0);
				PARAM_SetRTC(2000+pCmdFrame->buff[UART_PROTOCOL_CMD_DATA3_INDEX], 
							   pCmdFrame->buff[UART_PROTOCOL_CMD_DATA4_INDEX],
							   pCmdFrame->buff[UART_PROTOCOL_CMD_DATA5_INDEX],
							   pCmdFrame->buff[UART_PROTOCOL_CMD_DATA6_INDEX],
							   pCmdFrame->buff[UART_PROTOCOL_CMD_DATA7_INDEX],
							   0);*/
				
			}

			// 系统电压
			if ((24 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA8_INDEX]) ||
				(36 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA8_INDEX]) ||
				(48 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA8_INDEX]) ||
				(52 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA8_INDEX]) ||
				(60 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA8_INDEX]) ||
				(72 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA8_INDEX]) 
				)
			{ 
				PARAM_SetBatteryVoltageLevel(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA8_INDEX]);	// 单位:V
			}

			// 助力档位数
			if ((3 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA9_INDEX]) || (5 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA9_INDEX]) || (9 == pCmdFrame->buff[UART_PROTOCOL_CMD_DATA9_INDEX]))
			{
				PARAM_SetMaxAssist((ASSIST_ID_E)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA9_INDEX]);
			}

			// 助力正反
			PARAM_SetAssistDirection((BOOL)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA10_INDEX]);

			// 助力开始磁钢数
			if ((pCmdFrame->buff[UART_PROTOCOL_CMD_DATA11_INDEX] >= 2) && (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA11_INDEX] <= 64))
			{
				PARAM_SetAssitStartOfSteelNum(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA11_INDEX]);
			}

			// 助力比例
			PARAM_SetAssistPercent(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA12_INDEX]);

			// 转把分档
			PARAM_SetTurnbarLevel((BOOL)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA13_INDEX]);

			// 转把限速6kmph
			PARAM_SetTurnBarSpeed6kmphLimit((BOOL)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA14_INDEX]);

			// 缓启动参数
			if (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA15_INDEX] <= 3)
			{
				PARAM_SetSlowStart(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA15_INDEX]);
			}

			
			PARAM_SetCycleOfSteelNum(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA16_INDEX]);
			

			// 欠压门限
			tempVal = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA17_INDEX];
			tempVal <<= 8;
			tempVal |= pCmdFrame->buff[UART_PROTOCOL_CMD_DATA18_INDEX];			
			PARAM_SetLowVoltageThreshold(tempVal);

			// 限流门限
			PARAM_SetCurrentLimit(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA19_INDEX]*1000);	// 单位:mA

			PARAM_SetSpeedLimit(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA20_INDEX]*10);	// 单位:0.1km/h
			
			// 轮径代码
			switch (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA21_INDEX])
			{
				case 0:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_16_INCH);

					PARAM_SetNewWheelSizeInch(160);

					break;

				case 1:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_18_INCH);

					PARAM_SetNewWheelSizeInch(180);

					break;

				case 2:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_20_INCH);

					PARAM_SetNewWheelSizeInch(200);

					break;

				case 3:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_22_INCH);

					PARAM_SetNewWheelSizeInch(220);

					break;

				case 4:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_24_INCH);

					PARAM_SetNewWheelSizeInch(240);

					break;

				case 5:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_26_INCH);

					PARAM_SetNewWheelSizeInch(260);

					break;

				case 6:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_700C);

					PARAM_SetNewWheelSizeInch(275);

					break;

				case 7:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_28_INCH);

					PARAM_SetNewWheelSizeInch(280);
					
					break;
					
				case 55:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_27_5_INCH);

					PARAM_SetNewWheelSizeInch(275) ;				
					break;
					
				case 58:
					PARAM_SetWheelSizeID(PARAM_WHEEL_SIZE_29_INCH);

					PARAM_SetNewWheelSizeInch(290);
					
					break;
					
				default:
					
					PARAM_SetNewWheelSizeInch(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA21_INDEX] * 5);
					break;
			}	

			// 电量显示变化时间，单位:s
			PARAM_SetBatteryCapVaryTime(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA22_INDEX]);
			

			PARAM_SetBusAliveTime(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA23_INDEX] * 1000UL);	// 秒转为毫秒
		
			PARAM_SetSpeedFilterLevel(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA24_INDEX]);
			

			// 显示单位
			// 公英制单位 0公制 1英制
			PARAM_SetUnit((pCmdFrame->buff[UART_PROTOCOL_CMD_DATA25_INDEX]?UNIT_INCH:UNIT_METRIC));

			// 蓝牙 0无蓝牙 1有蓝牙
			PARAM_SetExistBle((pCmdFrame->buff[UART_PROTOCOL_CMD_DATA26_INDEX]?TRUE:FALSE));

			// 周长
			tempVal = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA27_INDEX];
			tempVal <<= 8;
			tempVal |= pCmdFrame->buff[UART_PROTOCOL_CMD_DATA28_INDEX];
			PARAM_SetNewperimeter(tempVal);

			// 协议
			PARAM_SetUartProtocol(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA29_INDEX]);

			// 电量计算方式
			PARAM_SetPercentageMethod((BATTERY_DATA_SRC_E)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA30_INDEX]));

			// 推车助力功能
			PARAM_SetPushSpeedSwitch(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA31_INDEX]);

			// 默认档位
			PARAM_SetDefaultAssist(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA32_INDEX]);

			// LOGO项
			PARAM_SetlogoMenu(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA33_INDEX]);

			// 串口通信电平
			PARAM_SetUartLevel(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA34_INDEX]);

			// 蜂鸣器开关
			PARAM_SetBeepSwitch((BOOL)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA35_INDEX]);

			// 高速蜂鸣器提醒
			PARAM_SetLimitSpeedBeep(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA36_INDEX]);
			
			// 定速巡航功能
			PARAM_SetCruiseEnableSwitch((BOOL)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA37_INDEX]);

			powerPassword = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA38_INDEX];
			powerPassword <<= 8;
			powerPassword |= pCmdFrame->buff[UART_PROTOCOL_CMD_DATA39_INDEX];
			
			// 开机密码
			PARAM_SetPowerOnPassword(powerPassword);

			menuPassword = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA40_INDEX];
			menuPassword <<= 8;
			menuPassword |= pCmdFrame->buff[UART_PROTOCOL_CMD_DATA41_INDEX];
			
			// 菜单密码
			PARAM_SetMenuPassword(menuPassword);

			// 恢复出厂设置
			PARAM_SetResFactorySet((BOOL)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA42_INDEX]);

			PARAM_SeteBikeName(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA43_INDEX]);

			value = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA44_INDEX];
			value <<= 8;
			value |= pCmdFrame->buff[UART_PROTOCOL_CMD_DATA45_INDEX];
			
			PARAM_SetPowerMax(value);

			value1 = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA46_INDEX];
			value1 <<= 8;
			value1 |= pCmdFrame->buff[UART_PROTOCOL_CMD_DATA47_INDEX];

			PARAM_SetbatteryCap(value1);

			// 显示轮径信息
			tempVal = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA48_INDEX];
			tempVal <<= 8;
			tempVal |= pCmdFrame->buff[UART_PROTOCOL_CMD_DATA49_INDEX];	
			PARAM_SetShowWheelSizeInch(tempVal);
			
			PARAM_SettiresSize(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA50_INDEX]);

			paramCB.nvm.param.protocol.carModel[0] = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA51_INDEX];
			paramCB.nvm.param.protocol.carModel[1] = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA52_INDEX];
			
			// 设置是否有开机密码
			PARAM_SetPowerPasswordSwitch((BOOL)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA53_INDEX]);

			// 设置是否有菜单密码
			PARAM_SetMenuPasswordSwitch((BOOL)pCmdFrame->buff[UART_PROTOCOL_CMD_DATA54_INDEX]);
			
			// NVM更新，立即写入
			NVM_SetDirtyFlag(TRUE);

			// 写入出厂设置备区
			NVM_Save_FactoryReset(TRUE);
			
			// 发送应答
			UART_PROTOCOL_ReportWriteParamResult(TRUE);			
			break;

		// 上位机读参数
		case UART_PROTOCOL_CMD_READ_CONTROL_PARAM:
			UART_PROTOCOL_ReportConfigureParam(TRUE);			
			break;

		// 数码管显示指令
		case UART_PROTOCOL_CMD_TEST_LED:
			// 第一个为界面码
			//length = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX];
			for (i = 0; i < 31;i++)
			{
				tempSeg[i] = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA2_INDEX+i];
			}
			
			LCD_DIS_AllState(tempSeg);
			
			STATE_EnterState(STATE_FUCTION_TEST);
			
			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_TEST_LED);

			// 数据长度
			UART_PROTOCOL_TxAddData(0);

			// 结果
			UART_PROTOCOL_TxAddData(TRUE);

			// 添加检验和，并添加至发送	
			UART_PROTOCOL_TxAddFrame();
			break;

		case UART_PROTOCOL_CMD_READ_FLAG_DATA:
			// 读出数据并串口发送			
			tempBuf = (uint8 *)malloc(128);
			if (NULL == tempBuf)
			{
				break;
			}
			
			// 读出FALSH指定长度数据
			IAP_FlashReadWordArray(PARAM_MCU_TEST_FLAG_ADDEESS, (uint32 *)tempBuf, 16);


			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_READ_FLAG_DATA);

			// 数据长度
			UART_PROTOCOL_TxAddData(0);

			// 标志区数据长度
			UART_PROTOCOL_TxAddData(16*4);

			// 高字节在前，低字节在后
			for (i = 0; i < 16; i++)
			{
				UART_PROTOCOL_TxAddData(tempBuf[4*i+3]);
				UART_PROTOCOL_TxAddData(tempBuf[4*i+2]);
				UART_PROTOCOL_TxAddData(tempBuf[4*i+1]);
				UART_PROTOCOL_TxAddData(tempBuf[4*i+0]);
			}

			// 添加检验和，并添加至发送	
			UART_PROTOCOL_TxAddFrame();

			free(tempBuf);
			break;

		case UART_PROTOCOL_CMD_WRITE_FLAG_DATA:
			
			// 写入标志位置
			tempVal3 = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX];
			
			// 写入标志内容
			tempVal2 = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA2_INDEX];	
			tempVal2 = ((tempVal2 << 8) | pCmdFrame->buff[UART_PROTOCOL_CMD_DATA3_INDEX]);	
			tempVal2 = ((tempVal2 << 8) | pCmdFrame->buff[UART_PROTOCOL_CMD_DATA4_INDEX]);	
			tempVal2 = ((tempVal2 << 8) | pCmdFrame->buff[UART_PROTOCOL_CMD_DATA5_INDEX]);
			
			IAP_FlashWriteWordArrayWithErase(PARAM_MCU_TEST_FLAG_ADDEESS + tempVal3*4, &tempVal2, 1);
			//PARAM_Init();

			if (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA6_INDEX])
			{
				STATE_EnterState(STATE_SHOW_POWER_OFF);
			}

			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_WRITE_FLAG_DATA);

			// 数据长度
			UART_PROTOCOL_TxAddData(0);

			// 结果
			UART_PROTOCOL_TxAddData(TRUE);

			// 添加检验和，并添加至发送	
			UART_PROTOCOL_TxAddFrame();
			break;

		case UART_PROTOCOL_CMD_WRITE_VERSION_TYPE:
		
			// 第一个写入类型
			tempVal3 = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX];
			
			// 第二个为信息长度
			tempVal2 = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA2_INDEX];

			tempBuf = (uint8 *)malloc(128);
			if (NULL == tempBuf)
			{
				break;
			}
			
			// 清零
			for (i = 0; i < 128; i++)
			{
				tempBuf[i] = 0;
			}
			// 类型信息
			for (i = 0; i < tempVal2 + 1; i++)
			{
				tempBuf[i] = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX+1+i];
			}
			
			// 判断类型种类是否合法
			if (16 >= tempVal3)
			{
				flag = IAP_FlashWriteWordArrayWithErase(PARAM_MCU_VERSION_ADDRESS[tempVal3], (uint32 *)tempBuf, (tempVal2 + 1 + 3) / 4);
				//PARAM_Init();
			}
			else
			{
				flag = FALSE;
			}

			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_WRITE_VERSION_TYPE);

			// 数据长度
			UART_PROTOCOL_TxAddData(0);

			// 类型
			UART_PROTOCOL_TxAddData(tempVal3);

			// 结果
			UART_PROTOCOL_TxAddData(flag);

			// 添加检验和，并添加至发送	
			UART_PROTOCOL_TxAddFrame();

			free(tempBuf);
			break;

		case UART_PROTOCOL_CMD_READ_VERSION_TYPE:
			tempVal3 = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX];
			
			tempBuf = (uint8 *)malloc(128);
			if (NULL == tempBuf)
			{
				break;
			}
			// 清零
			for (i = 0; i < 128; i++)
			{
				tempBuf[i] = 0;
			}

			// 目前最长为flag，为64个字节
			IAP_FlashReadWordArray(PARAM_MCU_VERSION_ADDRESS[tempVal3], (uint32 *)tempBuf, 64/4);

			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_READ_VERSION_TYPE);

			// 数据长度
			UART_PROTOCOL_TxAddData(0);
			
			// 要读取的类型
			tempVal3 = pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX];
			UART_PROTOCOL_TxAddData(tempVal3);

			switch (tempVal3)
			{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
					if ((tempBuf[0] > 0) && (tempBuf[0] < 64))
					{
						// 信息长度
						UART_PROTOCOL_TxAddData(tempBuf[0]);

						// 类型信息
						for (i = 0; i < tempBuf[0]; i++)
						{
							UART_PROTOCOL_TxAddData(tempBuf[i+1]);
						}
					}
					else
					{
						// 信息长度
						UART_PROTOCOL_TxAddData(0);
					}
					break;

				case 5:
				case 6:
					// 信息长度
					UART_PROTOCOL_TxAddData(4);
					
					// 类型信息，高字节在前，低字节在后
					UART_PROTOCOL_TxAddData(tempBuf[4]);
					UART_PROTOCOL_TxAddData(tempBuf[3]);
					UART_PROTOCOL_TxAddData(tempBuf[2]);
					UART_PROTOCOL_TxAddData(tempBuf[1]);
					break;
					
				case 7:
					// 信息长度
					UART_PROTOCOL_TxAddData(32);

					// 类型信息
					for (i = 0; i < 32; i++)
					{
						UART_PROTOCOL_TxAddData(tempBuf[i+1]);
					}
					break;
					
				

				default:
					break;
			}

			UART_PROTOCOL_TxAddFrame();

			free(tempBuf);
			break;
		case UART_ECO_CMD_ECO_MCU_UID:
			
			// 接受的MCU_UID长度是否等于12
			if (12 == pCmdFrame->buff[UART_PROTOCOL_CMD_LENGTH_INDEX])
			{
				
				temp1 = ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA4_INDEX]) << 24) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA3_INDEX]) << 16) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA2_INDEX]) << 8) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX]));

				temp2 = ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA8_INDEX]) << 24) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA7_INDEX]) << 16) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA6_INDEX]) << 8) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA5_INDEX]));

				temp3 = ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA12_INDEX]) << 24) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA11_INDEX]) << 16) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA10_INDEX]) << 8) 
						+ ((uint32)(pCmdFrame->buff[UART_PROTOCOL_CMD_DATA9_INDEX]));
			
				// MCU_UID一致则不理会
				if ((paramCB.runtime.watchUid.sn0 == temp1) && (paramCB.runtime.watchUid.sn1 == temp2) && (paramCB.runtime.watchUid.sn2 == temp3))
				{
					break;
				}
				// MCU_UID不同则写入老化标志
				else
				{					
					uartProtocolCB.readMcuUid = TRUE;
				}
			}
			break;
			
		case UART_PROTOCOL_CMD_TEST_KEY:
			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_TEST_KEY);

			// 数据长度
			UART_PROTOCOL_TxAddData(0);

			// 结果
			UART_PROTOCOL_TxAddData( PARAM_GetKeyValue() );

			// 添加检验和，并添加至发送	
			UART_PROTOCOL_TxAddFrame();
			break;

		case UART_PROTOCOL_CMD_TEST_LCD:

			lcdTestFlag = LcdIoTestResult();

			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_TEST_LCD);

			// 数据长度
			UART_PROTOCOL_TxAddData(0);

			// 结果
			UART_PROTOCOL_TxAddData(lcdTestFlag);

			// 添加检验和，并添加至发送	
			UART_PROTOCOL_TxAddFrame();
			break;

		case UART_PROTOCOL_CMD_PROTOCOL_SWITCH:
		
			if (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX])
			{
				static uint8 km5sRun, lidian2Run, bafang52Run, J3Run;

				// 关闭定时关机
				TIMER_KillTask(TIMER_ID_AUTO_POWER_OFF);
				
				switch (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA2_INDEX])
				{
					case 0:
						if (0 == km5sRun)
						{
							km5sRun = 1;
							lidian2Run = 0;
							bafang52Run = 0;
							J3Run = 0;
							
							UART_PROTOCOL_Init();
						}

						UART_PROTOCOL_SendCmdWithResult(UART_PROTOCOL_CMD_PROTOCOL_SWITCH, TRUE);
						break;

					case 1:
						if (0 == lidian2Run)
						{
							lidian2Run = 1;
							km5sRun = 0;
							bafang52Run = 0;
							J3Run = 0;
							
							// 由锂电2号协议切换到KM5S
							UART_PROTOCOL_Init();
						}

						UART_PROTOCOL_SendCmdWithResult(UART_PROTOCOL_CMD_PROTOCOL_SWITCH, TRUE);
						break;

					case 2:
						if (0 == bafang52Run)
						{
							bafang52Run = 1;
							km5sRun = 0;
							lidian2Run = 0;
							J3Run = 0;
							
							// 由八方V5.2切换到KM5S
							UART_PROTOCOL_Init();
						}

						UART_PROTOCOL_SendCmdWithResult(UART_PROTOCOL_CMD_PROTOCOL_SWITCH, TRUE);
						break;

					case 3:
						if (0 == J3Run)
						{
							J3Run = 1;
							bafang52Run = 0;
							km5sRun = 0;
							lidian2Run = 0;

							// J协议切换到KM5S
							UART_PROTOCOL_Init();
						}

						UART_PROTOCOL_SendCmdWithResult(UART_PROTOCOL_CMD_PROTOCOL_SWITCH, TRUE);
						break;

					default:
						break;
				}
			}		
			
			break;
			
			// 按键测试命令
		case UART_PROTOCOL_CMD_KEY_CHECK_TEST:
		
			// 注销所有按键服务
			KEYMSG_StopAllService();
		
			KEYMSG_RegisterMsgService(KEY_NAME_ADD, KEY_MSG_UP, UART_PROTOCOL_SendCmdCheckKeyValue, 1);
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_UP, UART_PROTOCOL_SendCmdCheckKeyValue, 1<<1);
			KEYMSG_RegisterMsgService(KEY_NAME_DEC, KEY_MSG_UP, UART_PROTOCOL_SendCmdCheckKeyValue, 1<<2);
			
			KEYMSG_RegisterMsgService(KEY_NAME_ADD_DEC, KEY_MSG_UP, UART_PROTOCOL_SendCmdCheckKeyValue, 1<<3);
			KEYMSG_RegisterMsgService(KEY_NAME_ADD_POWER, KEY_MSG_UP, UART_PROTOCOL_SendCmdCheckKeyValue, 1<<4);

			// 电源键长按，关机
			KEYMSG_RegisterMsgService(KEY_NAME_POWER, KEY_MSG_HOLD, STATE_EnterState, STATE_SHOW_POWER_OFF);
			
			break;

		// 蓝牙MAC地址读取
		case UART_PROTOCOL_CMD_BLUE_MACADDR_TEST:
		
			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_BLUE_MACADDR_TEST);
			
			// 数据长度
			UART_PROTOCOL_TxAddData(12);					
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[0]);
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[1]);
		
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[3]);
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[4]);
		
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[6]);
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[7]);
		
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[9]);
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[10]);
			
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[12]);
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[13]);
			
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[15]);
			UART_PROTOCOL_TxAddData(bleUartCB.rx.macBuff[16]);
			
			UART_PROTOCOL_TxAddFrame();
		
			break;
			
		// 清除里程数据
		case UART_PROTOCOL_CMD_CLEAR_MILEAGE:

			// 清除Tirp数据
			if (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA1_INDEX])
			{	
				PARAM_ClearTrip();
			}

			// 清除总里程
			if (pCmdFrame->buff[UART_PROTOCOL_CMD_DATA2_INDEX])
			{
				PARAM_SetTotalDistance(0);
			}

			// NVM更新，立即写入
			NVM_SetDirtyFlag(TRUE);

			// 添加命令头
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

			// 添加设备地址
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

			// 添加命令字
			UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_CLEAR_MILEAGE);

			// 添加数据长度
			UART_PROTOCOL_TxAddData(2);
			
			UART_PROTOCOL_TxAddData(TRUE);
			UART_PROTOCOL_TxAddData(TRUE);
			
			UART_PROTOCOL_TxAddFrame();
			
			break;
			
		default:
			break;
	}

	// 启动UART总线通讯超时判断
	UART_PROTOCOL_StartTimeoutCheckTask();
	
	// 删除命令帧
	pCB->rx.head ++;
	pCB->rx.head %= UART_PROTOCOL_RX_QUEUE_SIZE;
}

// RX通讯超时处理-单向
#if UART_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
void UART_PROTOCOL_CALLBACK_RxTimeOut(uint32 param)
{
	UART_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;

	pCmdFrame = &uartProtocolCB.rx.cmdQueue[uartProtocolCB.rx.end];

	// 超时错误，将命令帧长度清零，即认为抛弃该命令帧
	pCmdFrame->length = 0;	// 2016.1.6增加
	// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
	uartProtocolCB.rxFIFO.head ++;
	uartProtocolCB.rxFIFO.head %= UART_PROTOCOL_RX_FIFO_SIZE;
	uartProtocolCB.rxFIFO.currentProcessIndex = uartProtocolCB.rxFIFO.head;
}

// 停止Rx通讯超时检测任务
void UART_PROTOCOL_StopRxTimeOutCheck(void)
{
	TIMER_KillTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL);
}
#endif

// 码表设置立刻下发数据请求标志
void UART_PROTOCOL_SetTxAtOnceRequest(uint32 param)
{
	uartProtocolCB.txAtOnceRequest = (BOOL)param;
}

// 码表设置周期下发数据请求标志
void UART_PROTOCOL_CALLBACK_SetTxPeriodRequest(uint32 param)
{
	uartProtocolCB.txPeriodRequest = (BOOL)param;
}

// 码表发送数据通讯时序处理
void UART_PROTOCOL_DataCommunicationSequenceProcess(void)
{
	// 立即发送请求或是周期性发送请求
	if ((uartProtocolCB.txAtOnceRequest) || (uartProtocolCB.txPeriodRequest))
	{
		// 参数设置成功，则发送运行参数命令
		if (uartProtocolCB.paramSetOK)
		{
			UART_PROTOCOL_SendRuntimeParam();
		}
		// 参数设置未成功，则发送系统设置参数命令
		else
		{
			// 发送系统参数设置给控制器
			UART_PROTOCOL_SendConfigParam();
		}

		if (uartProtocolCB.txAtOnceRequest)
		{
			// 重新复位参数下发定时器
			TIMER_ResetTimer(TIMER_ID_PROTOCOL_PARAM_TX);
		}

		uartProtocolCB.txAtOnceRequest = FALSE;
		uartProtocolCB.txPeriodRequest = FALSE;
	}
}

// 	UART总线超时错误处理
void UART_PROTOCOL_CALLBACK_UartBusError(uint32 param)
{
	PARAM_SetErrorCode(ERROR_TYPE_COMMUNICATION_TIME_OUT);
	PARAM_SetBatteryDataSrc(BATTERY_DATA_SRC_ADC);
	
}


// 发送配置参数命令到驱动器
void UART_PROTOCOL_SendConfigParam(void)
{
	uint8 i;
	uint8 wheelSizeCode = 0;
	uint16 voltage;

	// 轮径映射到显示
	switch (PARAM_GetNewWheelSizeInch())
	{

		case 160:
			wheelSizeCode = 0;
			break;
			
		case 180:
			wheelSizeCode = 1;
			break;
			
		case 200:
			wheelSizeCode = 2;
			break;
			
		case 220:
			wheelSizeCode = 3;
			break;
			
		case 240:
			wheelSizeCode = 4;
			break;
			
		case 260:
			wheelSizeCode = 5;
			break;
			
		case 275:
			wheelSizeCode = 6;
			break;
			
		case 280:
			wheelSizeCode = 7;
			break;
			
		// 非法值，默认为26寸
		default:
			wheelSizeCode = 5;
			break;
	}

	// 添加命令头
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);	

	// 添加设备地址
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

	// 添加参数设置命令
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HMI_CTRL_PARAM_SET);	

	// 添加有效数据长度，这里固定为7
	UART_PROTOCOL_TxAddData(7);

	// ■■构造字节5■■: 助力传感器参数1，包含【助力开始磁钢数】和【助力正反】
	hmiDriveCB.set.param.assistStartOfSteelNum = PARAM_GetAssitStartOfSteelNum();
	
	hmiDriveCB.set.param.assistDirection = PARAM_GetAssistDirection();

	// ■■构造字节6■■: 助力传感器参数2，【助力比例】，范围0~255，通常6颗磁钢设为128，12颗磁钢设为64
	hmiDriveCB.set.param.ucAssistPercent = PARAM_GetAssistPercent();

	// ■■构造字节7■■: 速度传感器磁钢数，包含【转把是否分档】、【转把类型】、【缓启动参数】、【速度传感器一圈测速磁钢数】
	// 根据转把是否分档，设置参数
	if(PARAM_GetTurnbarLevel())
	{
		hmiDriveCB.set.param.ucTurnBarLevel = 1;
	}
	else
	{
		hmiDriveCB.set.param.ucTurnBarLevel = 0;
	}

	// 根据转把是否限速，设置参数
	if(PARAM_GetTurnBarSpeed6kmphLimit())
	{
		hmiDriveCB.set.param.ucTurnBarSpeed6kmphLimit = 1;
	}
	else
	{
		hmiDriveCB.set.param.ucTurnBarSpeed6kmphLimit = 0;
	}

	hmiDriveCB.set.param.ucSlowStart = PARAM_GetSlowStart();
	hmiDriveCB.set.param.steelNumOfSpeedSensor = PARAM_GetCycleOfSteelNum();

	// ■■构造字节8■■: 欠压偏压值，该值和下文的字节9一起构造
	//hmiDriveCB.set.param.lowVoltageThresholdOffset = 0;

	// ■■构造字节9■■: 电压电流
	voltage = PARAM_GetLowVoltageThreshold()/100;		// 获取欠压门限，单位0.1V
	// 协议中规定的三个基准门限为21V、31.5V、42V，因此，这里通过将欠压门限与基准值比较来计算基准值和偏压值
	// 21V和31.5V的中间值取为26.2V，当欠压门限≤该值时，基准值选择21V
	if(voltage <= 262)
	{
		hmiDriveCB.set.param.lowVoltageThreshold = 1;

		// 欠压门限低于基准值，偏压为负
		if(voltage < 210)
		{
			hmiDriveCB.set.param.lowVoltageThresholdOffset = 210-voltage;
			hmiDriveCB.set.param.lowVoltageThresholdOffset |= 0x80;
		}
		else
		{
			hmiDriveCB.set.param.lowVoltageThresholdOffset = voltage-210;
		}
	}
	// 31.5V和42V的中间值取为36.7V，当欠压门限≤该值时，基准值选择31.5V
	else if(voltage <= 367)
	{
		hmiDriveCB.set.param.lowVoltageThreshold = 2;

		// 欠压门限低于基准值，偏压为负
		if(voltage < 315)
		{
			hmiDriveCB.set.param.lowVoltageThresholdOffset = 315-voltage;
			hmiDriveCB.set.param.lowVoltageThresholdOffset |= 0x80;
		}
		else
		{
			hmiDriveCB.set.param.lowVoltageThresholdOffset = voltage-315;
		}
	}
	// 欠压门限高于31.5V和42V的中间值，则基准值选择42V
	else
	{
		hmiDriveCB.set.param.lowVoltageThreshold = 3;

		// 欠压门限低于基准值，偏压为负
		if(voltage < 420)
		{
			hmiDriveCB.set.param.lowVoltageThresholdOffset = 420-voltage;
			hmiDriveCB.set.param.lowVoltageThresholdOffset |= 0x80;
		}
		else
		{
			hmiDriveCB.set.param.lowVoltageThresholdOffset = voltage-420;
		}
	}

	hmiDriveCB.set.param.currentThreshold = (PARAM_GetCurrentLimit() / 1000 * 2); 			// 根据协议修改,限流值*2

	// ■■构造字节10■■: 握手字节，范围: 0~63的随机数，科迪思仪表发送的是2
	hmiDriveCB.set.param.shakeHandFlag = 2;	

	// ■■构造字节11■■: 限速代码
	hmiDriveCB.set.param.wheelSize = wheelSizeCode;	

	if (paramCB.nvm.param.common.speed.limitVal < 100)
	{
		hmiDriveCB.set.param.speedLimit = 0;
	}
	else if (paramCB.nvm.param.common.speed.limitVal > 410)

	{
		hmiDriveCB.set.param.speedLimit = 31;
	}
	else
	{
// 每档限速不同
#if PROTOCOL_SPEED_LIMIT_SEPERATED
		switch (PARAM_GetAssistLevel())
		{
			case ASSIST_1:
				hmiDriveCB.set.param.speedLimit = (paramCB.nvm.param.common.speed.limitVal * 0.48 + 5)/10-10;
				break;
			
			case ASSIST_2:
				hmiDriveCB.set.param.speedLimit = (paramCB.nvm.param.common.speed.limitVal * 0.60 + 5)/10-10;
				break;
			
			case ASSIST_3:
				hmiDriveCB.set.param.speedLimit = (paramCB.nvm.param.common.speed.limitVal * 0.72 + 5)/10-10;
				break;
			
			case ASSIST_4:
				hmiDriveCB.set.param.speedLimit = (paramCB.nvm.param.common.speed.limitVal * 0.84 + 5)/10-10;
				break;
			
			case ASSIST_5:
				hmiDriveCB.set.param.speedLimit = (paramCB.nvm.param.common.speed.limitVal/10)-10;		// 根据协议，限速值需要减去10
				break;
			
			default:
				hmiDriveCB.set.param.speedLimit = (paramCB.nvm.param.common.speed.limitVal/10)-10;		// 根据协议，限速值需要减去10
				break;
		}
// 每档限速相同
#else
		hmiDriveCB.set.param.speedLimit = (paramCB.nvm.param.common.speed.limitVal/10)-10;		// 根据协议，限速值需要减去10
#endif
	}

	for ( i = 0; i < UART_PROTOCOL_SET_PARAM_SIZE; i++ )
	{
		UART_PROTOCOL_TxAddData(hmiDriveCB.set.buff[i]);
	}

	// 添加检验和与结束符，并添加至发送
	UART_PROTOCOL_TxAddFrame();
}

// 发送实时运行参数到驱动器
void UART_PROTOCOL_SendRuntimeParam(void)
{
	uint8 i;

	// 添加命令头
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

	// 添加设备地址
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

	// 添加命令
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HMI_CTRL_RUN);

	// 添加数据长度，固定为7
	UART_PROTOCOL_TxAddData(7);

	// ■■构造字节5■■: PWM值
	//hmiDriveCB.running.param.pwm = UART_PROTOCOL_GetPWM(hmiDriveCB.running.param.pwm);
	hmiDriveCB.running.param.pwm = PARAM_GetPwmFixed(PARAM_GetAssistLevel());
	
	// ■■构造字节6■■: 状态值
	// bit7：大灯，0关1开
	// bit6：预留
	// bit5：电池容量低，1欠压
	// bit4：6km/h推行功能，0关1开
	// bit3：预留
	// bit2：巡航，0关1开
	// bit1：预留
	// bit0：超速，1超速
	hmiDriveCB.running.param.isLightOn = PARAM_GetLightSwitch();
	hmiDriveCB.running.param.isLowVoltageAlarm = PARAM_GetBatteryLowAlarm();
	hmiDriveCB.running.param.isPushModeOn = PARAM_IsPushAssistOn();
	hmiDriveCB.running.param.isCruiseOn = !PARAM_GetCruiseEnableSwitch();
	hmiDriveCB.running.param.isOverSpeedAlarm = PARAM_GetOverSpeedAlarm();
	
	for ( i = 0; i < UART_PROTOCOL_RUNNING_PARAM_SIZE; i++ )
	{
		UART_PROTOCOL_TxAddData(hmiDriveCB.running.buff[i]);
	}

	// 添加检验和与结束符，并添加至发送
	UART_PROTOCOL_TxAddFrame();
}

// 驱动器上报参数处理
void UART_PROTOCOL_OnReceiveReport(uint8* pBuff, uint32 len)
{
	uint8 i;
	double fSpeedTemp;
	//PARAM_WHEEL_SIZE_ID wheelSizeID = PARAM_GetWheelSizeID();
	uint16 perimeter = 0;
	uint16 tempVal;

	CHECK_PARAM_SAME_RETURN(NULL, pBuff);	
	CHECK_PARAM_SAME_RETURN(0, len);
	//CHECK_PARAM_OVER_SAME_RETURN(wheelSizeID, PARAM_WHEEL_SIZE_MAX);

	// 从缓存取出数据到结构体
	for (i = 0; (i < len) && (i < UART_PROTOCOL_RX_PARAM_SIZE); i++)
	{
		hmiDriveCB.rx.buff[i] = pBuff[i];
	}

	// 到这里，即把驱动器上报的数据放入了 hmiDriveCB.rx.buff 中

	// 提取电池电压值
//	#warning "注意，KM5S电压上报其实不开放，因此用此值会错，另外adc模块已经将电压传入参数层了，因此不要用此处上报的电压值"
	//tempVal = hmiDriveCB.rx.param.ucCurBatteryVoltage;
	//tempVal *= 1000;											// 单位转换: 1V转换为1000mV	
	//PARAM_SetBatteryVoltage(tempVal);
	if (BATTERY_DATA_SRC_CONTROLLER == PARAM_GetBatteryDataSrc())
	{
		
		tempVal = hmiDriveCB.rx.param.ucCurBatteryVoltage;
		tempVal *= 1000;											// 单位转换: 1V转换为1000mV 
			
		PARAM_SetBatteryVoltage(tempVal);
	}
	else if (BATTERY_DATA_SRC_BMS == PARAM_GetBatteryDataSrc())
	{
		//PARAM_SetBatteryPercent(100);
	}

	// 提取巡航状态
	PARAM_SetCruiseState((BOOL)hmiDriveCB.rx.param.ucCurCruiseState);

	// 提取欠压状态
	PARAM_SetUnderVoltageAlarm((BOOL)hmiDriveCB.rx.param.ucUnderVoltageAlarm);

	// 提取电流值
	PARAM_SetBatteryCurrent((uint16)(((float)hmiDriveCB.rx.param.ucCurBatteryCurrent / 3.0f) * 1000)); 	// 根据协议修改，单位转换: 1A转换为1000mA 

	// 提取实时速度，单位:KM/H
	tempVal = hmiDriveCB.rx.param.oneCycleTimeH;
	tempVal <<= 8;
	tempVal |= hmiDriveCB.rx.param.oneCycleTimeL;
	// 与溧水缪工确认结果: 当收到控制器上报转一圈时间为3500时，就认为速度为0
	if (tempVal >= 3500UL)
	{
		// 计算偏差值	
		PARAM_SpeedFilterDevValue(0);
	}
	else
	{
		//perimeter = PARAM_GetPerimeter((uint8)wheelSizeID);
		perimeter = PARAM_GetNewperimeter();
		fSpeedTemp = (tempVal ? (3.6 / tempVal * perimeter) : 0);
		LIMIT_TO_MAX(fSpeedTemp, 99.9);

		// 计算偏差值
		PARAM_SpeedFilterDevValue((uint16)(fSpeedTemp*10));		// 转换为0.1KM/H
	}


	// 驱动器上报的错误代码
	if ((hmiDriveCB.rx.param.ucErrorCode == ERROR_TYPE_CURRENT_ERROR) || (hmiDriveCB.rx.param.ucErrorCode      == ERROR_TYPE_THROTTLE_ERROR)
	|| (hmiDriveCB.rx.param.ucErrorCode  == ERROR_TYPE_MOTOR_PHASE_ERROR ) || (hmiDriveCB.rx.param.ucErrorCode == ERROR_TYPE_HALL_SENSOR_ERROR)
	|| (hmiDriveCB.rx.param.ucErrorCode  == ERROR_TYPE_BRAKE_ERROR) || (hmiDriveCB.rx.param.ucErrorCode        == ERROR_TYPE_LOW_VOLTAGE_ERROR)
	|| (hmiDriveCB.rx.param.ucErrorCode  == ERROR_TYPE_NO_ERROR))
	{
		PARAM_SetErrorCode((ERROR_TYPE_E)hmiDriveCB.rx.param.ucErrorCode);
	}
}

// 上报写配置参数结果
void UART_PROTOCOL_ReportWriteParamResult(uint32 param)
{
	// 添加命令头
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

	// 添加设备地址
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

	// 添加命令字
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_WRITE_CONTROL_PARAM_RESULT);

	// 添加数据长度
	UART_PROTOCOL_TxAddData(1);

	// 写入结果
	UART_PROTOCOL_TxAddData(param);

	// 添加检验和与结束符，并添加至发送
	UART_PROTOCOL_TxAddFrame();
}

// 上报配置参数
void UART_PROTOCOL_ReportConfigureParam(uint32 param)
{	
	// 添加命令头
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

	// 添加设备地址
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

	// 添加命令字
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_READ_CONTROL_PARAM_REPORT);

	// 添加数据长度
	UART_PROTOCOL_TxAddData(19);

	// 背光亮度
	UART_PROTOCOL_TxAddData(PARAM_GetBrightness());

	// 休眠时间
	UART_PROTOCOL_TxAddData(PARAM_GetPowerOffTime());

	// 系统电压
	UART_PROTOCOL_TxAddData(PARAM_GetBatteryVoltageLevel());

	// 助力档位数
	UART_PROTOCOL_TxAddData(PARAM_GetMaxAssist());

	// 助力正反
	UART_PROTOCOL_TxAddData(PARAM_GetAssistDirection());

	// 助力开始磁钢数
	UART_PROTOCOL_TxAddData(PARAM_GetAssitStartOfSteelNum());

	// 助力比例
	UART_PROTOCOL_TxAddData(PARAM_GetAssistPercent());

	// 转把分档
	UART_PROTOCOL_TxAddData(PARAM_GetTurnbarLevel());

	// 转把限速6kmph
	UART_PROTOCOL_TxAddData(PARAM_GetTurnBarSpeed6kmphLimit());

	// 缓启动参数
	UART_PROTOCOL_TxAddData(PARAM_GetSlowStart());

	// 测速磁钢数
	UART_PROTOCOL_TxAddData(PARAM_GetCycleOfSteelNum());

	// 欠压门限
	UART_PROTOCOL_TxAddData(PARAM_GetLowVoltageThreshold()>>8);
	UART_PROTOCOL_TxAddData(PARAM_GetLowVoltageThreshold()&0x00FF);

	// 限流门限
	UART_PROTOCOL_TxAddData(PARAM_GetCurrentLimit()/1000);

	// 助力限速
	UART_PROTOCOL_TxAddData(paramCB.nvm.param.common.speed.limitVal/10);

	// 轮径代码
	switch (PARAM_GetNewWheelSizeInch())
	{
		case 160:
			UART_PROTOCOL_TxAddData(0);
			break;

		case 180:
			UART_PROTOCOL_TxAddData(1);
			break;

		case 200:
			UART_PROTOCOL_TxAddData(2);
			break;

		case 220:
			UART_PROTOCOL_TxAddData(3);
			break;

		case 240:
			UART_PROTOCOL_TxAddData(4);
			break;

		case 260:
			UART_PROTOCOL_TxAddData(5);
			break;

		case 275:
			if(PARAM_GetWheelSizeID() ==PARAM_WHEEL_SIZE_700C )
			{
				UART_PROTOCOL_TxAddData(6);
			}
			else
			{
				UART_PROTOCOL_TxAddData(PARAM_GetNewWheelSizeInch() / 5);
			}
			break;

		case 280:
			UART_PROTOCOL_TxAddData(7);
			break;

		default:
			UART_PROTOCOL_TxAddData(PARAM_GetNewWheelSizeInch() / 5);
			break;
	}

	// 电量显示变化时间
	UART_PROTOCOL_TxAddData(PARAM_GetBatteryCapVaryTime());

	// 总线故障超时时间
	UART_PROTOCOL_TxAddData(PARAM_GetBusAliveTime()/1000);

	// 速度平滑度等级
	UART_PROTOCOL_TxAddData(PARAM_GetSpeedFilterLevel());

	// 公英制单位
	UART_PROTOCOL_TxAddData(PARAM_GetUnit());
	
	// 硬件是否有蓝牙
	UART_PROTOCOL_TxAddData(PARAM_GetExistBle());

	// 周长
	UART_PROTOCOL_TxAddData(PARAM_GetNewperimeter()>>8);
	UART_PROTOCOL_TxAddData(PARAM_GetNewperimeter()&0x00FF);

	// 协议
	UART_PROTOCOL_TxAddData(PARAM_GetUartProtocol());

	// 电量计算方式
	UART_PROTOCOL_TxAddData(PARAM_GetPercentageMethod());

	// 推车助力功能
	UART_PROTOCOL_TxAddData(PARAM_GetPushSpeedSwitch());
	
	// 默认档位
	UART_PROTOCOL_TxAddData(PARAM_GetDefaultAssist());

	// LOGO项
	UART_PROTOCOL_TxAddData(PARAM_GetlogoMenu());

	// 串口通信电平
	UART_PROTOCOL_TxAddData(PARAM_GetUartLevel());

	// 蜂鸣器开关
	UART_PROTOCOL_TxAddData(PARAM_GetBeepSwitch());

	// 高速蜂鸣器提醒
	UART_PROTOCOL_TxAddData(PARAM_GetLimitSpeedBeep());

	// 定速巡航功能
	UART_PROTOCOL_TxAddData(PARAM_GetCruiseEnableSwitch());

	// 开机密码
	UART_PROTOCOL_TxAddData(PARAM_GetPowerOnPassword() >> 8);
	UART_PROTOCOL_TxAddData(PARAM_GetPowerOnPassword() & 0x00FF);

	// 菜单密码
	UART_PROTOCOL_TxAddData(PARAM_GetMenuPassword() >> 8);
	UART_PROTOCOL_TxAddData(PARAM_GetMenuPassword() & 0x00FF);

	// 恢复出厂设置
	UART_PROTOCOL_TxAddData(PARAM_GetResFactorySet());

	// 车型
	UART_PROTOCOL_TxAddData(PARAM_GeteBikeName());

	// 电机功率
	UART_PROTOCOL_TxAddData(PARAM_GetPowerMax()>>8);
	UART_PROTOCOL_TxAddData(PARAM_GetPowerMax()&0x00FF);

	// 电池容量
	UART_PROTOCOL_TxAddData(PARAM_GetbatteryCap()>>8);
	UART_PROTOCOL_TxAddData(PARAM_GetbatteryCap()&0x00FF);

	// 显示轮径信息
	UART_PROTOCOL_TxAddData(PARAM_GetShowWheelSizeInch()>>8);
	UART_PROTOCOL_TxAddData(PARAM_GetShowWheelSizeInch()&0x00FF);
	
	// 轮宽
	UART_PROTOCOL_TxAddData(PARAM_GettiresSize());

	// 服务器车型
	UART_PROTOCOL_TxAddData(paramCB.nvm.param.protocol.carModel[0]);
	UART_PROTOCOL_TxAddData(paramCB.nvm.param.protocol.carModel[1]);
	
	// 有无开机密码
	UART_PROTOCOL_TxAddData(PARAM_GetPowerPasswordSwitch());
	
	// 有无菜单密码
	UART_PROTOCOL_TxAddData(PARAM_GetMenuPasswordSwitch());
	
	// 添加检验和与结束符，并添加至发送
	UART_PROTOCOL_TxAddFrame();
}

// 发送老化累加次数
void UART_PROTOCOL_SendCmdAging(uint32 param)
{
	// 添加命令头
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

	// 添加设备地址
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

	// 添加命令字
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HMI_CTRL_RUN);

	// 添加数据长度
	UART_PROTOCOL_TxAddData(2);

	// 填充假的数据即可
	UART_PROTOCOL_TxAddData(1);
	UART_PROTOCOL_TxAddData(2);

	// 添加检验和与结束符，并添加至发送
	UART_PROTOCOL_TxAddFrame();
}

// 发送自身MCU的唯一序列号，防止误入老化
void UART_PROTOCOL_SendMcuUid(uint32 param)
{
	uint8 buff[12];
	uint8 i;

	// 按位发送sn0
	buff[0] = paramCB.runtime.watchUid.sn0 & 0xff;
	buff[1] = (paramCB.runtime.watchUid.sn0 >> 8)  & 0xff;
	buff[2] = (paramCB.runtime.watchUid.sn0 >> 16) & 0xff;
	buff[3] = (paramCB.runtime.watchUid.sn0 >> 24) & 0xff;

	// 按位发送sn1
	buff[4] = paramCB.runtime.watchUid.sn1 & 0xff;
	buff[5] = (paramCB.runtime.watchUid.sn1 >> 8)  & 0xff;
	buff[6] = (paramCB.runtime.watchUid.sn1 >> 16) & 0xff;
	buff[7] = (paramCB.runtime.watchUid.sn1 >> 24) & 0xff;

	// 按位发送sn2
	buff[8]  = paramCB.runtime.watchUid.sn2 & 0xff;
	buff[9]  = (paramCB.runtime.watchUid.sn2 >> 8)  & 0xff;
	buff[10] = (paramCB.runtime.watchUid.sn2 >> 16) & 0xff;
	buff[11] = (paramCB.runtime.watchUid.sn2 >> 24) & 0xff;

	
	// 添加命令头
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);
	
	// 添加设备地址
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);
	
	// 添加命令字
	UART_PROTOCOL_TxAddData(UART_ECO_CMD_ECO_MCU_UID);
	
	UART_PROTOCOL_TxAddData(0);	
	
	for (i = 0; i < 12; i++)
	{
		UART_PROTOCOL_TxAddData(buff[i]);
	}
	
	// 添加检验和与结束符，并添加至发送
	UART_PROTOCOL_TxAddFrame();
}

// 发送命令带结果
void UART_PROTOCOL_SendCmdWithResult(uint8 cmdWord, uint8 result)
{
	// 添加命令头
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_HEAD);

	// 添加设备地址
	UART_PROTOCOL_TxAddData(UART_PROTOCOL_CMD_DEVICE_ADDR);

	// 添加命令字
	UART_PROTOCOL_TxAddData(cmdWord);

	// 添加数据长度
	UART_PROTOCOL_TxAddData(1);
	
	UART_PROTOCOL_TxAddData(result);
	
	UART_PROTOCOL_TxAddFrame();
}

// 发送按键值
void UART_PROTOCOL_SendCmdCheckKeyValue(uint32 param)
{
	// 启动UART总线通讯超时判断
	UART_PROTOCOL_StartTimeoutCheckTask();
	
	UART_PROTOCOL_SendCmdWithResult(UART_PROTOCOL_CMD_KEY_CHECK_TEST, param);
}


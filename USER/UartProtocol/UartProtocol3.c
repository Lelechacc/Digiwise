#include "common.h"
#include "timer.h"
#include "delay.h"
#include "uartDrive.h"
#include "uartProtocol3.h"
#include "uartProtocol.h"
#include "param.h"
#include "state.h"

/******************************************************************************
* 【内部接口声明】
******************************************************************************/

// 数据结构初始化
void UART_PROTOCOL3_DataStructInit(UART_PROTOCOL3_CB* pCB);

// UART报文接收处理函数(注意根据具体模块修改)
void UART_PROTOCOL3_MacProcess(uint16 standarID, uint8* pData, uint16 length);

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void UART_PROTOCOL3_RxFIFOProcess(UART_PROTOCOL3_CB* pCB);

// UART命令帧缓冲区处理
void UART_PROTOCOL3_CmdFrameProcess(UART_PROTOCOL3_CB* pCB);

// 对传入的命令帧进行校验，返回校验结果
BOOL UART_PROTOCOL3_CheckSUM(UART_PROTOCOL3_RX_CMD_FRAME* pCmdFrame);

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL UART_PROTOCOL3_ConfirmTempCmdFrameBuff(UART_PROTOCOL3_CB* pCB);

// 通讯超时处理-单向
void UART_PROTOCOL3_CALLBACK_RxTimeOut(uint32 param);

// 停止RX通讯超时检测任务
void UART_PROTOCOL3_StopRxTimeOutCheck(void);

// 协议层发送处理过程
void UART_PROTOCOL3_TxStateProcess(void);

// UART协议层向驱动层注册数据发送接口
void UART_PROTOCOL3_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length));

//===============================================================================================================
// 码表设置周期下发数据请求标志
void UART_PROTOCOL3_CALLBACK_SetTxPeriodRequest(uint32 param);

// 码表发送数据通讯时序处理
void UART_PROTOCOL3_DataCommunicationSequenceProcess(void);

// 启动通讯超时判断任务
void UART_PROTOCOL3_StartTimeoutCheckTask(void);

// UART总线超时错误处理
void UART_PROTOCOL3_CALLBACK_UartBusError(uint32 param);

//=========================================================================
// 接收驱动器上报数据
void UART_PROTOCOL3_OnReceiveReport(uint8* pBuff, uint32 len);

// 发送配置参数命令到驱动器
void UART_PROTOCOL3_SendConfigParam(void);

// 发送运行参数命令到驱动器
void UART_PROTOCOL3_SendRuntimeParam(void);

// 上报写配置参数结果
void UART_PROTOCOL3_ReportWriteParamResult(uint32 param);

// 上报配置参数
void UART_PROTOCOL3_ReportConfigureParam(uint32 param);

//==================================================================================
// 速度平滑滤波处理
void UART_PROTOCOL3_CALLBACK_SpeedFilterProcess(uint32 param);


// 全局变量定义
UART_PROTOCOL3_CB uartProtocolCB3;

UART_PROTOCOL3_PARAM_CB hmiDriveCB3;

// ■■函数定义区■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// 启动通讯超时判断任务
void UART_PROTOCOL3_StartTimeoutCheckTask(void)
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
					UART_PROTOCOL3_CALLBACK_UartBusError,
					TRUE,
					1,
					ACTION_MODE_ADD_TO_QUEUE);
#endif
}

// 协议初始化
void UART_PROTOCOL3_Init(void)
{
	// 协议层数据结构初始化
	UART_PROTOCOL3_DataStructInit(&uartProtocolCB3);

	// 向驱动层注册数据接收接口
	UART_DRIVE_RegisterDataSendService(UART_PROTOCOL3_MacProcess);

	// 向驱动层注册数据发送接口
	UART_PROTOCOL3_RegisterDataSendService(UART_DRIVE_AddTxArray);

	// 注册间隔参数设置时间
	TIMER_AddTask(TIMER_ID_PROTOCOL_PARAM_TX,
					UART_PROTOCOL3_CMD_SEND_TIME,
					UART_PROTOCOL3_CALLBACK_SetTxPeriodRequest,
					TRUE,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);
 

	// 启动UART总线通讯超时判断
	UART_PROTOCOL3_StartTimeoutCheckTask();

}

// UART协议层过程处理
void UART_PROTOCOL3_Process(void)
{
	// UART接收FIFO缓冲区处理
	UART_PROTOCOL3_RxFIFOProcess(&uartProtocolCB3);

	// UART接收命令缓冲区处理
	UART_PROTOCOL3_CmdFrameProcess(&uartProtocolCB3);
	
	// UART协议层发送处理过程
	UART_PROTOCOL3_TxStateProcess();

	// 码表发送数据通讯时序处理
	UART_PROTOCOL3_DataCommunicationSequenceProcess();

	// 在J协议中嵌入KM5S协议
	UART_PROTOCOL_Process();
}

// 向发送命令帧队列中添加数据
void UART_PROTOCOL3_TxAddData(uint8 data)
{
	uint16 head = uartProtocolCB3.tx.head;
	uint16 end =  uartProtocolCB3.tx.end;
	UART_PROTOCOL3_TX_CMD_FRAME* pCmdFrame = &uartProtocolCB3.tx.cmdQueue[uartProtocolCB3.tx.end];	

	// 发送缓冲区已满，不予接收
	if((end + 1) % UART_PROTOCOL3_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 队尾命令帧已满，退出
	if(pCmdFrame->length >= UART_PROTOCOL3_TX_CMD_FRAME_LENGTH_MAX)
	{
		return;
	}

	// 数据添加到帧末尾，并更新帧长度
	pCmdFrame->buff[pCmdFrame->length] = data;
	pCmdFrame->length ++;
}

// 确认添加命令帧，即承认之前填充的数据为命令帧，将其添加到发送队列中，由main进行调度发送，本函数内会自动校正命令长度，并添加校验码
void UART_PROTOCOL3_TxAddFrame(void)
{
	uint8 checkSum = 0;
	uint16 i = 0;
	uint8 xorVal;
	uint16 head = uartProtocolCB3.tx.head;
	uint16 end  = uartProtocolCB3.tx.end;
	UART_PROTOCOL3_TX_CMD_FRAME* pCmdFrame = &uartProtocolCB3.tx.cmdQueue[uartProtocolCB3.tx.end];
	uint16 length = pCmdFrame->length;	

	// 发送缓冲区已满，不予接收
	if((end + 1) % UART_PROTOCOL3_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 命令帧长度不足，清除已填充的数据，退出
	if(UART_PROTOCOL3_CMD_FRAME_LENGTH_MIN-3 > length)	// 减去"异或调整值、异或校验、结束符Ox0D"3个字节
	{
		pCmdFrame->length = 0;
		
		return;
	}

	// 队尾命令帧已满，退出
	if((length >= UART_PROTOCOL3_TX_CMD_FRAME_LENGTH_MAX) 
		|| (length+1 >= UART_PROTOCOL3_TX_CMD_FRAME_LENGTH_MAX) 
		|| (length+2 >= UART_PROTOCOL3_TX_CMD_FRAME_LENGTH_MAX))
	{
		return;
	}

	// 该协议调整异或值，若异或校验为0x0D，则改变此值，重新校验
	for (xorVal = 0; xorVal <= 255; xorVal++)
	{
		pCmdFrame->buff[pCmdFrame->length++] = xorVal;
		length = pCmdFrame->length;
		
		for (i = 1; i < length; i++)
		{
			checkSum ^= pCmdFrame->buff[i];
		}

		// 异或校验为0x0D，则改变此值，重新校验
		if (0x0D == checkSum)
		{
			pCmdFrame->length--;

			checkSum = 0;
		}
		else
		{
			break;
		}
	}

	// 异或校验值
	pCmdFrame->buff[pCmdFrame->length++] = checkSum;

	// 结束标识
	pCmdFrame->buff[pCmdFrame->length++] = 0x0D;

	uartProtocolCB3.tx.end ++;
	uartProtocolCB3.tx.end %= UART_PROTOCOL3_TX_QUEUE_SIZE;
	//pCB->tx.cmdQueue[pCB->tx.end].length = 0;   //2015.12.2修改
}

// 数据结构初始化
void UART_PROTOCOL3_DataStructInit(UART_PROTOCOL3_CB* pCB)
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
	for(i = 0; i < UART_PROTOCOL3_TX_QUEUE_SIZE; i++)
	{
		pCB->tx.cmdQueue[i].length = 0;
	}

	pCB->rxFIFO.head = 0;
	pCB->rxFIFO.end = 0;
	pCB->rxFIFO.currentProcessIndex = 0;

	pCB->rx.head = 0;
	pCB->rx.end  = 0;
	for(i=0; i<UART_PROTOCOL3_RX_QUEUE_SIZE; i++)
	{
		pCB->rx.cmdQueue[i].length = 0;
	}
}

// UART报文接收处理函数(注意根据具体模块修改)
void UART_PROTOCOL3_MacProcess(uint16 standarID, uint8* pData, uint16 length)
{
	uint16 end = uartProtocolCB3.rxFIFO.end;
	uint16 head = uartProtocolCB3.rxFIFO.head;
	uint8 rxdata = 0x00;
	
	// 接收数据
	rxdata = *pData;

	// 一级缓冲区已满，不予接收
	if((end + 1)%UART_PROTOCOL3_RX_FIFO_SIZE == head)
	{
		return;
	}
	// 一级缓冲区未满，接收 
	else
	{
		// 将接收到的数据放到临时缓冲区中
		uartProtocolCB3.rxFIFO.buff[end] = rxdata;
		uartProtocolCB3.rxFIFO.end ++;
		uartProtocolCB3.rxFIFO.end %= UART_PROTOCOL3_RX_FIFO_SIZE;
	}	

	// 借用KM5S协议解析
	UART_PROTOCOL_MacProcess(standarID, pData, length);
}

// UART协议层向驱动层注册数据发送接口
void UART_PROTOCOL3_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length))
{		
	uartProtocolCB3.sendDataThrowService = service;
}

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL UART_PROTOCOL3_ConfirmTempCmdFrameBuff(UART_PROTOCOL3_CB* pCB)
{
	UART_PROTOCOL3_RX_CMD_FRAME* pCmdFrame = NULL;
	
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
	pCB->rx.end %= UART_PROTOCOL3_RX_QUEUE_SIZE;
	pCB->rx.cmdQueue[pCB->rx.end].length = 0;	// 该行的作用是将新的添加位置有效数据个数清零，以便将这个位置当做临时帧缓冲区
	
	return TRUE;
}

// 协议层发送处理过程
void UART_PROTOCOL3_TxStateProcess(void)
{
	uint16 head = uartProtocolCB3.tx.head;
	uint16 end =  uartProtocolCB3.tx.end;
	uint16 length = uartProtocolCB3.tx.cmdQueue[head].length;
	uint8* pCmd = uartProtocolCB3.tx.cmdQueue[head].buff;
	uint16 localDeviceID = uartProtocolCB3.tx.cmdQueue[head].deviceID;

	// 发送缓冲区为空，说明无数据
	if (head == end)
	{
		return;
	}

	// 发送函数没有注册直接返回
	if (NULL == uartProtocolCB3.sendDataThrowService)
	{
		return;
	}

	// 协议层有数据需要发送到驱动层		
	if (!(*uartProtocolCB3.sendDataThrowService)(localDeviceID, pCmd, length))
	{
		return;
	}

	// 发送环形队列更新位置
	uartProtocolCB3.tx.cmdQueue[head].length = 0;
	uartProtocolCB3.tx.head ++;
	uartProtocolCB3.tx.head %= UART_PROTOCOL3_TX_QUEUE_SIZE;
}

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void UART_PROTOCOL3_RxFIFOProcess(UART_PROTOCOL3_CB* pCB)
{
	uint16 end = pCB->rxFIFO.end;
	uint16 head = pCB->rxFIFO.head;
	UART_PROTOCOL3_RX_CMD_FRAME* pCmdFrame = NULL;
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
		switch (currentData)
		{
			case UART_PROTOCOL3_CMD_HMI_CTRL_RUN:
				pCB->rxFIFO.curCmdFrameLength = UART_PROTOCOL3_RX_CMD_HMI_CTRL_RUN_LENGTH_MAX;
				break;

			default:
				// 命令头错误，删除当前字节并退出
				pCB->rxFIFO.head ++;
				pCB->rxFIFO.head %= UART_PROTOCOL3_RX_FIFO_SIZE;
				pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;

				pCB->rxFIFO.curCmdFrameLength = 0;
				break;
		}

		// 如果长度为0说明非法，退出
		if (0 == pCB->rxFIFO.curCmdFrameLength)
		{
			return;
		}

		// 命令头正确，但无临时缓冲区可用，退出
		if((pCB->rx.end + 1)%UART_PROTOCOL3_RX_QUEUE_SIZE == pCB->rx.head)
		{
			return;
		}

		// 添加UART通讯超时时间设置-2016.1.5增加
#if UART_PROTOCOL3_RX_TIME_OUT_CHECK_ENABLE
		TIMER_AddTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL,
						UART_PROTOCOL3_BUS_UNIDIRECTIONAL_TIME_OUT,
						UART_PROTOCOL3_CALLBACK_RxTimeOut,
						0,
						1,
						ACTION_MODE_ADD_TO_QUEUE);
#endif
		
		// 命令头正确，有临时缓冲区可用，则将其添加到命令帧临时缓冲区中
		pCmdFrame->buff[pCmdFrame->length++]= currentData;
		pCB->rxFIFO.currentProcessIndex ++;
		pCB->rxFIFO.currentProcessIndex %= UART_PROTOCOL3_RX_FIFO_SIZE;
	}
	// 非首字节，将数据添加到命令帧临时缓冲区中，但暂不删除当前数据
	else
	{
		// 临时缓冲区溢出，说明当前正在接收的命令帧是错误的，正确的命令帧不会出现长度溢出的情况
		if(pCmdFrame->length >= UART_PROTOCOL3_RX_CMD_FRAME_LENGTH_MAX)
		{
#if UART_PROTOCOL3_RX_TIME_OUT_CHECK_ENABLE
			// 停止RX通讯超时检测
			UART_PROTOCOL3_StopRxTimeOutCheck();
#endif

			// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
			pCmdFrame->length = 0;	// 2016.1.5增加
			// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
			pCB->rxFIFO.head ++;
			pCB->rxFIFO.head %= UART_PROTOCOL3_RX_FIFO_SIZE;
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
			pCB->rxFIFO.currentProcessIndex %= UART_PROTOCOL3_RX_FIFO_SIZE;

			// ■■接下来，需要检查命令帧是否完整，如果完整，则将命令帧临时缓冲区扶正■■
			
			// 首先判断命令帧最小长度，一个完整的命令字至少包括4个字节: 包含:起始符、异或调整值、错误信息、异或校验，因此不足4个字节的必定不完整
			if(pCmdFrame->length < UART_PROTOCOL3_CMD_FRAME_LENGTH_MIN)
			{
				// 继续接收
				continue;
			}

			// 命令帧长度校验
			length = pCmdFrame->length;
			if(length < pCB->rxFIFO.curCmdFrameLength)
			{
				// 长度要求不一致，说明未接收完毕，退出继续
				continue;
			}

			// 命令帧长度OK，则进行校验，失败时删除命令头
			if(!UART_PROTOCOL3_CheckSUM(pCmdFrame))
			{
#if UART_PROTOCOL3_RX_TIME_OUT_CHECK_ENABLE
				// 停止RX通讯超时检测
				UART_PROTOCOL3_StopRxTimeOutCheck();
#endif
				
				// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
				pCmdFrame->length = 0;
				// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
				pCB->rxFIFO.head ++;
				pCB->rxFIFO.head %= UART_PROTOCOL3_RX_FIFO_SIZE;
				pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;
				
				return;
			}

#if UART_PROTOCOL3_RX_TIME_OUT_CHECK_ENABLE
			// 停止RX通讯超时检测
			UART_PROTOCOL3_StopRxTimeOutCheck();
#endif
			
			// 执行到这里，即说明接收到了一个完整并且正确的命令帧，此时需将处理过的数据从一级缓冲区中删除，并将该命令帧扶正
			pCB->rxFIFO.head += length;
			pCB->rxFIFO.head %= UART_PROTOCOL3_RX_FIFO_SIZE;
			pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;
			UART_PROTOCOL3_ConfirmTempCmdFrameBuff(pCB);

			return;
		}
	}
}

// 对传入的命令帧进行校验，返回校验结果
BOOL UART_PROTOCOL3_CheckSUM(UART_PROTOCOL3_RX_CMD_FRAME* pCmdFrame)
{
	uint8 checkSum = 0;
	uint16 i = 0;
	
	if(NULL == pCmdFrame)
	{
		return FALSE;
	}

	// 从数据1开始，到校验码之前的一个字节，依次进行异或运算
	for(i=1; i<pCmdFrame->length-1; i++)
	{
		checkSum ^= pCmdFrame->buff[i];
	}
	
	// 判断计算得到的校验码与命令帧中的校验码是否相同
	if(pCmdFrame->buff[pCmdFrame->length-1] != checkSum)
	{
		return FALSE;
	}
	
	return TRUE;
}

// UART命令帧缓冲区处理
void UART_PROTOCOL3_CmdFrameProcess(UART_PROTOCOL3_CB* pCB)
{
	UART_PROTOCOL3_CMD cmd = UART_PROTOCOL3_CMD_NULL;
	UART_PROTOCOL3_RX_CMD_FRAME* pCmdFrame = NULL;

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
	
	// 起始符或命令字非法，退出
	if((UART_PROTOCOL3_CMD_HMI_CTRL_RUN != pCmdFrame->buff[UART_PROTOCOL3_CMD_START_SYMBOL_CMD_INDEX])
	&& (UART_PROTOCOL3_CMD_WRITE_CONTROL_PARAM != pCmdFrame->buff[UART_PROTOCOL3_CMD_START_SYMBOL_CMD_INDEX]) 
	&& (UART_PROTOCOL3_CMD_READ_CONTROL_PARAM != pCmdFrame->buff[UART_PROTOCOL3_CMD_START_SYMBOL_CMD_INDEX]))
	{
		// 删除命令帧
		pCB->rx.head ++;
		pCB->rx.head %= UART_PROTOCOL3_RX_QUEUE_SIZE;
		return;
	}

	// 命令头合法，则提取命令
	cmd = (UART_PROTOCOL3_CMD)pCmdFrame->buff[UART_PROTOCOL3_CMD_START_SYMBOL_CMD_INDEX];
	
	// 执行命令帧
	switch(cmd)
	{
		// 空命令，不予执行
		case UART_PROTOCOL3_CMD_NULL:
			break;

		// 参数设置状态，正常运行状态
		// 注意:无论仪表发送F或S，控制器始终回复F
		case UART_PROTOCOL3_CMD_HMI_CTRL_RUN:

			UART_PROTOCOL3_OnReceiveReport(&pCmdFrame->buff[UART_PROTOCOL3_CMD_DATA1_INDEX], UART_PROTOCOL3_RX_PARAM_SIZE);

			// 参数设置成功
			uartProtocolCB3.paramSetOK = TRUE;

			break;	
			
		default:
			break;
	}

	// 启动UART总线通讯超时判断
	UART_PROTOCOL3_StartTimeoutCheckTask();
	
	// 删除命令帧
	pCB->rx.head ++;
	pCB->rx.head %= UART_PROTOCOL3_RX_QUEUE_SIZE;
}

// RX通讯超时处理-单向
#if UART_PROTOCOL3_RX_TIME_OUT_CHECK_ENABLE
void UART_PROTOCOL3_CALLBACK_RxTimeOut(uint32 param)
{
	UART_PROTOCOL3_RX_CMD_FRAME* pCmdFrame = NULL;

	pCmdFrame = &uartProtocolCB3.rx.cmdQueue[uartProtocolCB3.rx.end];

	// 超时错误，将命令帧长度清零，即认为抛弃该命令帧
	pCmdFrame->length = 0;	// 2016.1.6增加
	// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
	uartProtocolCB3.rxFIFO.head ++;
	uartProtocolCB3.rxFIFO.head %= UART_PROTOCOL3_RX_FIFO_SIZE;
	uartProtocolCB3.rxFIFO.currentProcessIndex = uartProtocolCB3.rxFIFO.head;
}

// 停止Rx通讯超时检测任务
void UART_PROTOCOL3_StopRxTimeOutCheck(void)
{
	TIMER_KillTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL);
}
#endif

// 码表设置立刻下发数据请求标志
void UART_PROTOCOL3_SetTxAtOnceRequest(uint32 param)
{
	uartProtocolCB3.txAtOnceRequest = (BOOL)param;
}

// 码表设置周期下发数据请求标志
void UART_PROTOCOL3_CALLBACK_SetTxPeriodRequest(uint32 param)
{
	uartProtocolCB3.txPeriodRequest = (BOOL)param;
}

// 码表发送数据通讯时序处理
void UART_PROTOCOL3_DataCommunicationSequenceProcess(void)
{
	// 立即发送请求或是周期性发送请求
	if ((uartProtocolCB3.txAtOnceRequest) || (uartProtocolCB3.txPeriodRequest))
	{	
		UART_PROTOCOL3_SendRuntimeParam();

		if (uartProtocolCB3.txAtOnceRequest)
		{
			// 重新复位参数下发定时器
			TIMER_ResetTimer(TIMER_ID_PROTOCOL_PARAM_TX);
		}

		uartProtocolCB3.txAtOnceRequest = FALSE;
		uartProtocolCB3.txPeriodRequest = FALSE;
	}
}

// 	UART总线超时错误处理
void UART_PROTOCOL3_CALLBACK_UartBusError(uint32 param)
{
	
	PARAM_SetErrorCode(ERROR_TYPE_COMMUNICATION_TIME_OUT);
	
}

// 发送配置参数命令到驱动器
void UART_PROTOCOL3_SendConfigParam(void)
{
	uint8 i;
	uint8 wheelSizeCode = 0;

	// 添加起始符->控制器
	UART_PROTOCOL3_TxAddData(UART_PROTOCOL3_CMD_HMI_CTRL_PARAM_SET);	

	// ■■构造数据1■■: 档位和助力，配置参数时不需要，因为这几个参数是动态的
	hmiDriveCB3.set.param.assitLevel = 0x00;
	hmiDriveCB3.set.param.isPushModeOn = 0x00;
	hmiDriveCB3.set.param.reserveByte1Bit65 = 0x00;
	hmiDriveCB3.set.param.isLightOn = 0x00;

	// ■■构造数据2■■: 限速代码以及轮径代码
	hmiDriveCB3.set.param.speedLimitCode = paramCB.nvm.param.common.speed.limitVal/10 - 20;
	if (paramCB.nvm.param.common.speed.limitVal > 510)
	{
		hmiDriveCB3.set.param.speedLimitCode = 0;
	}

	switch (PARAM_GetNewWheelSizeInch()) // 新轮径单位0.1inch
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

		case 290:
			wheelSizeCode = 7;
			break;

		// 非法值，默认为26寸
		default:
			wheelSizeCode = 5;
			break;
	}
	hmiDriveCB3.set.param.wheelSizeCode = wheelSizeCode;

	// 到此组装数据
	for (i = 0; i < UART_PROTOCOL3_SET_PARAM_SIZE; i++)
	{
		UART_PROTOCOL3_TxAddData(hmiDriveCB3.set.buff[i]);
	}

	// 添加检验和，并添加至发送
	UART_PROTOCOL3_TxAddFrame();
}

// 发送实时运行参数到驱动器
void UART_PROTOCOL3_SendRuntimeParam(void)
{
	uint8 i;
	uint8 wheelSizeCode = 0;

	// 添加起始符->控制器
	UART_PROTOCOL3_TxAddData(UART_PROTOCOL3_CMD_HMI_CTRL_RUN);

	// ■■构造数据1■■: 大灯开关，推车，助力档位
	hmiDriveCB3.running.param.isPushModeOn = PARAM_IsPushAssistOn();
	hmiDriveCB3.running.param.isLightOn = PARAM_GetLightSwitch();
	
	hmiDriveCB3.running.param.assitLevel = PARAM_GetAssistLevel();

	// 0档时发送0x0F
	if (ASSIST_0 == PARAM_GetAssistLevel())
	{
		hmiDriveCB3.running.param.assitLevel = 0x0F;
	}


	// ■■构造数据2■■: 限速代码以及轮径代码
	hmiDriveCB3.running.param.speedLimitCode = paramCB.nvm.param.common.speed.limitVal/10 - 20;
	if (paramCB.nvm.param.common.speed.limitVal > 510)
	{
		hmiDriveCB3.set.param.speedLimitCode = 0;
	}

	switch (PARAM_GetNewWheelSizeInch()) // 新轮径单位0.1inch
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

		case 290:
			wheelSizeCode = 7;
			break;

		// 非法值，默认为26寸
		default:
			wheelSizeCode = 5;
			break;
	}
	hmiDriveCB3.running.param.wheelSizeCode = wheelSizeCode;

	// 到此组装数据
	for (i = 0; i < UART_PROTOCOL3_RUNNING_PARAM_SIZE; i++)
	{
		UART_PROTOCOL3_TxAddData(hmiDriveCB3.running.buff[i]);
	}

	// 添加检验和，并添加至发送
	UART_PROTOCOL3_TxAddFrame();
}

// 驱动器上报参数处理
void UART_PROTOCOL3_OnReceiveReport(uint8* pBuff, uint32 len)
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
	for (i = 0; (i < len) && (i < UART_PROTOCOL3_RX_PARAM_SIZE); i++)
	{
		hmiDriveCB3.rx.buff[i] = pBuff[i];
	}

	// 到这里，即把驱动器上报的数据放入了 hmiDriveCB3.rx.buff 中

	// 提取电量格数:控制器分档电压，0表示欠压，1～5表示电量值。
	//if (hmiDriveCB3.rx.param.batteryVoltageLevel <= 5)
	//{
		// 注意，此部分已经在adc模块实现
		//PARAM_SetBatteryPercent(hmiDriveCB3.rx.param.batteryVoltageLevel*20);
	//}

	if (BATTERY_DATA_SRC_CONTROLLER == PARAM_GetBatteryDataSrc())
	{
		// J协议没有电压上报的字段，故为了兼容人为采用固定电压
		tempVal = PARAM_GetBatteryVoltageLevel();
		tempVal *= 1000;											// 单位转换: 1V转换为1000mV	
		
		PARAM_SetBatteryVoltage(tempVal);
	}
	else if (BATTERY_DATA_SRC_BMS == PARAM_GetBatteryDataSrc())
	{
		// 电量
		// 提取电量格数:控制器分档电压，0表示欠压，1～5表示电量值。
		PARAM_SetBatteryPercent(hmiDriveCB3.rx.param.batteryVoltageLevel*20);
	}

	// 提取电流值
	PARAM_SetBatteryCurrent((uint16)((float)hmiDriveCB3.rx.param.batteryCurrent / 3.0f * 1000)); 	// 根据协议修改，单位转换: 1A转换为1000mA 

	// 【数据4】为车轮转一圈时间
	tempVal = hmiDriveCB3.rx.param.oneCycleTimeH;
	tempVal <<= 8;
	tempVal |= hmiDriveCB3.rx.param.oneCycleTimeL;
	// 当收到控制器上报转一圈时间为3500时，就认为速度为0
	if(tempVal >= 3341)
	{
		PARAM_SpeedFilterDevValue(0);
	}
	else
	{
		//perimeter = PARAM_GetPerimeter((uint8)wheelSizeID);
		perimeter = PARAM_GetNewperimeter();
		fSpeedTemp = (tempVal ? (3.6 / tempVal * perimeter) : 0);
		LIMIT_TO_MAX(fSpeedTemp, 99.9);
		
		PARAM_SpeedFilterDevValue((uint16)(fSpeedTemp*10));		// 转换为0.1KM/H
	}

	// 驱动器上报的错误代码
	if ((hmiDriveCB3.rx.param.ucErrorCode == ERROR_TYPE_CURRENT_ERROR) || (hmiDriveCB3.rx.param.ucErrorCode      == ERROR_TYPE_THROTTLE_ERROR)
	|| (hmiDriveCB3.rx.param.ucErrorCode  == ERROR_TYPE_MOTOR_PHASE_ERROR ) || (hmiDriveCB3.rx.param.ucErrorCode == ERROR_TYPE_HALL_SENSOR_ERROR)
	|| (hmiDriveCB3.rx.param.ucErrorCode  == ERROR_TYPE_BRAKE_ERROR) || (hmiDriveCB3.rx.param.ucErrorCode        == ERROR_TYPE_TEMP_ERROR)
	|| (hmiDriveCB3.rx.param.ucErrorCode  == ERROR_TYPE_MOTOR_BLOCK_ERROR) || (hmiDriveCB3.rx.param.ucErrorCode  == ERROR_TYPE_OVER_VOLTAGE_ERROR))
	{
		PARAM_SetErrorCode((ERROR_TYPE_E)hmiDriveCB3.rx.param.ucErrorCode);	
	}
	else
	{
		PARAM_SetErrorCode(ERROR_TYPE_NO_ERROR);
	}
}

// 上报写配置参数结果
void UART_PROTOCOL3_ReportWriteParamResult(uint32 param)
{
	// 添加起始符->控制器
	UART_PROTOCOL3_TxAddData(UART_PROTOCOL3_CMD_WRITE_CONTROL_PARAM_RESULT);
	
	// 写入结果
	UART_PROTOCOL3_TxAddData(param);

	// 添加检验和，并添加至发送
	UART_PROTOCOL3_TxAddFrame();
}

// 上报配置参数
void UART_PROTOCOL3_ReportConfigureParam(uint32 param)
{	
	// 添加起始符->控制器
	UART_PROTOCOL3_TxAddData(UART_PROTOCOL3_CMD_READ_CONTROL_PARAM_REPORT);

	// 背光亮度
	UART_PROTOCOL3_TxAddData(PARAM_GetBrightness());

	// 休眠时间
	UART_PROTOCOL3_TxAddData(PARAM_GetPowerOffTime());

	// 系统电压
	UART_PROTOCOL3_TxAddData(PARAM_GetBatteryVoltageLevel()/1000);

	// 电量显示变化时间
	UART_PROTOCOL3_TxAddData(PARAM_GetBatteryCapVaryTime());

	// 总线故障超时时间
	UART_PROTOCOL3_TxAddData(PARAM_GetBusAliveTime());

	// 助力档位数
	UART_PROTOCOL3_TxAddData(PARAM_GetMaxAssist());

	// 助力限速
	UART_PROTOCOL3_TxAddData(paramCB.nvm.param.common.speed.limitVal/10);

	// 轮径代码
	/*switch (PARAM_GetWheelSizeID())
	{
		case PARAM_WHEEL_SIZE_16_INCH:
			UART_PROTOCOL3_TxAddData(0);
			break;

		case PARAM_WHEEL_SIZE_18_INCH:
			UART_PROTOCOL3_TxAddData(1);
			break;

		case PARAM_WHEEL_SIZE_20_INCH:
			UART_PROTOCOL3_TxAddData(2);
			break;

		case PARAM_WHEEL_SIZE_22_INCH:
			UART_PROTOCOL3_TxAddData(3);
			break;

		case PARAM_WHEEL_SIZE_24_INCH:
			UART_PROTOCOL3_TxAddData(4);
			break;

		case PARAM_WHEEL_SIZE_26_INCH:
			UART_PROTOCOL3_TxAddData(5);
			break;

		case PARAM_WHEEL_SIZE_700C:
			UART_PROTOCOL3_TxAddData(6);
			break;

		case PARAM_WHEEL_SIZE_28_INCH:
			UART_PROTOCOL3_TxAddData(7);
			break;

		default:
			UART_PROTOCOL3_TxAddData(5);
			break;
	}*/
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
			UART_PROTOCOL_TxAddData(6);
			break;

		case 280:
			UART_PROTOCOL_TxAddData(7);
			break;

		default:
			UART_PROTOCOL_TxAddData(0);
			break;
	}
	
	// 速度平滑度等级
	UART_PROTOCOL3_TxAddData(PARAM_GetSpeedFilterLevel());

	// 添加检验和，并添加至发送
	UART_PROTOCOL3_TxAddFrame();
}


#include "common.h"
#include "timer.h"
#include "delay.h"
#include "uartDrive.h"
#include "uartProtocol1.h"
#include "param.h"
#include "state.h"
#include "lcd.h"
#include "iap.h"
#include "backlight.h"
#include "lcdTest.h"

/******************************************************************************
* 【内部接口声明】
******************************************************************************/

// 数据结构初始化
void UART_PROTOCOL1_DataStructInit(UART_PROTOCOL1_CB* pCB);

// UART报文接收处理函数(注意根据具体模块修改)
void UART_PROTOCOL1_MacProcess(uint16 standarID, uint8* pData, uint16 length);

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void UART_PROTOCOL1_RxFIFOProcess(UART_PROTOCOL1_CB* pCB);

// UART命令帧缓冲区处理
void UART_PROTOCOL1_CmdFrameProcess(UART_PROTOCOL1_CB* pCB);

// 对传入的命令帧进行校验，返回校验结果
BOOL UART_PROTOCOL1_CheckSUM(UART_PROTOCOL1_RX_CMD_FRAME* pCmdFrame);

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL UART_PROTOCOL1_ConfirmTempCmdFrameBuff(UART_PROTOCOL1_CB* pCB);

// 通讯超时处理-单向
void UART_PROTOCOL1_CALLBACK_RxTimeOut(uint32 param);

// 停止RX通讯超时检测任务
void UART_PROTOCOL1_StopRxTimeOutCheck(void);

// 协议层发送处理过程
void UART_PROTOCOL1_TxStateProcess(void);

// UART协议层向驱动层注册数据发送接口
void UART_PROTOCOL1_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length));

//===============================================================================================================
// 码表设置周期下发数据请求标志
void UART_PROTOCOL1_CALLBACK_SetTxPeriodRequest(uint32 param);

// 码表发送数据通讯时序处理
void UART_PROTOCOL1_DataCommunicationSequenceProcess(void);

// 启动通讯超时判断任务
void UART_PROTOCOL1_StartTimeoutCheckTask(void);

// UART总线超时错误处理
void UART_PROTOCOL1_CALLBACK_UartBusError(uint32 param);

//=========================================================================
// 接收驱动器上报数据
void UART_PROTOCOL1_OnReceiveReport(uint8* pBuff, uint32 len);

// 发送运行参数命令到驱动器
void UART_PROTOCOL1_SendRuntimeParam(void);

// 上报写配置参数结果
void UART_PROTOCOL1_ReportWriteParamResult(uint32 param);

// 上报配置参数
void UART_PROTOCOL1_ReportConfigureParam(uint32 param);

//==========================================================================

// 全局变量定义
UART_PROTOCOL1_CB uartProtocolCB1;

UART_PROTOCOL1_PARAM_CB hmiDriveCB1;

// ■■函数定义区■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// 启动通讯超时判断任务
void UART_PROTOCOL1_StartTimeoutCheckTask(void)
{
	// 清除通讯异常
	if(ERROR_TYPE_COMMUNICATION_TIME_OUT == PARAM_GetErrorCode())
	{
		PARAM_SetErrorCode(ERROR_TYPE_NO_ERROR);
	}

	// 开启UART通讯超时定时器
#if __SYSTEM_NO_TIME_OUT_ERROR__	// 研发调试时不启用
#else
	TIMER_AddTask(TIMER_ID_PROTOCOL_TXRX_TIME_OUT,
					PARAM_GetBusAliveTime(),
					UART_PROTOCOL1_CALLBACK_UartBusError,
					TRUE,
					1,
					ACTION_MODE_ADD_TO_QUEUE);
#endif
}

// 协议初始化
void UART_PROTOCOL1_Init(void)
{
	// 协议层数据结构初始化
	UART_PROTOCOL1_DataStructInit(&uartProtocolCB1);

	// 向驱动层注册数据接收接口
	UART_DRIVE_RegisterDataSendService(UART_PROTOCOL1_MacProcess);

	// 向驱动层注册数据发送接口
	UART_PROTOCOL1_RegisterDataSendService(UART_DRIVE_AddTxArray);

	// 注册间隔参数设置时间
	TIMER_AddTask(TIMER_ID_PROTOCOL_PARAM_TX,
					UART_PROTOCOL1_CMD_SEND_TIME,
					UART_PROTOCOL1_CALLBACK_SetTxPeriodRequest,
					TRUE,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);
 

	// 启动UART总线通讯超时判断
	UART_PROTOCOL1_StartTimeoutCheckTask();

}


// UART协议层过程处理
void UART_PROTOCOL1_Process(void)
{
	// UART接收FIFO缓冲区处理
	UART_PROTOCOL1_RxFIFOProcess(&uartProtocolCB1);

	// UART接收命令缓冲区处理
	UART_PROTOCOL1_CmdFrameProcess(&uartProtocolCB1);
	
	// UART协议层发送处理过程
	UART_PROTOCOL1_TxStateProcess();

	// 码表发送数据通讯时序处理
	UART_PROTOCOL1_DataCommunicationSequenceProcess();

//=========================================================
	// 在锂电2号协议中嵌入KM5S协议
	UART_PROTOCOL_Process();
}

// 向发送命令帧队列中添加数据
void UART_PROTOCOL1_TxAddData(uint8 data)
{
	uint16 head = uartProtocolCB1.tx.head;
	uint16 end =  uartProtocolCB1.tx.end;
	UART_PROTOCOL1_TX_CMD_FRAME* pCmdFrame = &uartProtocolCB1.tx.cmdQueue[uartProtocolCB1.tx.end];

	// 发送缓冲区已满，不予接收
	if((end + 1) % UART_PROTOCOL1_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 队尾命令帧已满，退出
	if(pCmdFrame->length >= UART_PROTOCOL1_TX_CMD_FRAME_LENGTH_MAX)
	{
		return;
	}

	// 数据添加到帧末尾，并更新帧长度
	pCmdFrame->buff[pCmdFrame->length] = data;
	pCmdFrame->length ++;
}

// 确认添加命令帧，即承认之前填充的数据为命令帧，将其添加到发送队列中，由main进行调度发送，本函数内会自动校正命令长度，并添加校验码
void UART_PROTOCOL1_TxAddFrame(void)
{
	uint16 cc = 0;
	uint16 i = 0;
	uint16 head = uartProtocolCB1.tx.head;
	uint16 end  = uartProtocolCB1.tx.end;
	UART_PROTOCOL1_TX_CMD_FRAME* pCmdFrame = &uartProtocolCB1.tx.cmdQueue[uartProtocolCB1.tx.end];
	uint16 length = pCmdFrame->length;

	// 发送缓冲区已满，不予接收
	if((end + 1) % UART_PROTOCOL1_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 命令帧长度不足，清除已填充的数据，退出
	if(UART_PROTOCOL1_CMD_FRAME_LENGTH_MIN-1 > length)	// 减去"校验和"1个字节
	{
		pCmdFrame->length = 0;
		
		return;
	}

	// 队尾命令帧已满，退出
	if(length >= UART_PROTOCOL1_TX_CMD_FRAME_LENGTH_MAX)
	{
		return;
	}

	// 重新设置数据长度，系统在准备数据时，填充的"数据长度"可以为任意值，并且不需要添加校验码，在这里重新设置为正确的值
	pCmdFrame->buff[UART_PROTOCOL1_CMD_LENGTH_INDEX] = length + 1;	// 重设数据长度，要加上1字节校验长度

	for(i=0; i<length; i++)
	{
		cc ^= pCmdFrame->buff[i];
	}
	pCmdFrame->buff[pCmdFrame->length++] = cc ;

	uartProtocolCB1.tx.end ++;
	uartProtocolCB1.tx.end %= UART_PROTOCOL1_TX_QUEUE_SIZE;
	//pCB->tx.cmdQueue[pCB->tx.end].length = 0;   //2015.12.2修改
}

// 数据结构初始化
void UART_PROTOCOL1_DataStructInit(UART_PROTOCOL1_CB* pCB)
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
	for(i = 0; i < UART_PROTOCOL1_TX_QUEUE_SIZE; i++)
	{
		pCB->tx.cmdQueue[i].length = 0;
	}

	pCB->rxFIFO.head = 0;
	pCB->rxFIFO.end = 0;
	pCB->rxFIFO.currentProcessIndex = 0;

	pCB->rx.head = 0;
	pCB->rx.end  = 0;
	for(i=0; i<UART_PROTOCOL1_RX_QUEUE_SIZE; i++)
	{
		pCB->rx.cmdQueue[i].length = 0;
	}
}

// UART报文接收处理函数(注意根据具体模块修改)
void UART_PROTOCOL1_MacProcess(uint16 standarID, uint8* pData, uint16 length)
{
	uint16 end = uartProtocolCB1.rxFIFO.end;
	uint16 head = uartProtocolCB1.rxFIFO.head;
	uint8 rxdata = 0x00;
	
	// 接收数据
	rxdata = *pData;

	// 一级缓冲区已满，不予接收
	if((end + 1)%UART_PROTOCOL1_RX_FIFO_SIZE == head)
	{
		return;
	}
	// 一级缓冲区未满，接收 
	else
	{
		// 将接收到的数据放到临时缓冲区中
		uartProtocolCB1.rxFIFO.buff[end] = rxdata;
		uartProtocolCB1.rxFIFO.end ++;
		uartProtocolCB1.rxFIFO.end %= UART_PROTOCOL1_RX_FIFO_SIZE;
	}

//====================================================================
	// 借用KM5S协议解析
	UART_PROTOCOL_MacProcess(standarID, pData, length);
}

// UART协议层向驱动层注册数据发送接口
void UART_PROTOCOL1_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length))
{		
	uartProtocolCB1.sendDataThrowService = service;
}

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL UART_PROTOCOL1_ConfirmTempCmdFrameBuff(UART_PROTOCOL1_CB* pCB)
{
	UART_PROTOCOL1_RX_CMD_FRAME* pCmdFrame = NULL;
	
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
	pCB->rx.end %= UART_PROTOCOL1_RX_QUEUE_SIZE;
	pCB->rx.cmdQueue[pCB->rx.end].length = 0;	// 该行的作用是将新的添加位置有效数据个数清零，以便将这个位置当做临时帧缓冲区
	
	return TRUE;
}

// 协议层发送处理过程
void UART_PROTOCOL1_TxStateProcess(void)
{
	uint16 head = uartProtocolCB1.tx.head;
	uint16 end =  uartProtocolCB1.tx.end;
	uint16 length = uartProtocolCB1.tx.cmdQueue[head].length;
	uint8* pCmd = uartProtocolCB1.tx.cmdQueue[head].buff;
	uint16 localDeviceID = uartProtocolCB1.tx.cmdQueue[head].deviceID;

	// 发送缓冲区为空，说明无数据
	if (head == end)
	{
		return;
	}

	// 发送函数没有注册直接返回
	if (NULL == uartProtocolCB1.sendDataThrowService)
	{
		return;
	}

	// 协议层有数据需要发送到驱动层		
	if (!(*uartProtocolCB1.sendDataThrowService)(localDeviceID, pCmd, length))
	{
		return;
	}

	// 发送环形队列更新位置
	uartProtocolCB1.tx.cmdQueue[head].length = 0;
	uartProtocolCB1.tx.head ++;
	uartProtocolCB1.tx.head %= UART_PROTOCOL1_TX_QUEUE_SIZE;
}

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void UART_PROTOCOL1_RxFIFOProcess(UART_PROTOCOL1_CB* pCB)
{
	uint16 end = pCB->rxFIFO.end;
	uint16 head = pCB->rxFIFO.head;
	UART_PROTOCOL1_RX_CMD_FRAME* pCmdFrame = NULL;
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
		if(UART_PROTOCOL1_CMD_RX_DEVICE_ADDR != currentData)
		{
			pCB->rxFIFO.head ++;
			pCB->rxFIFO.head %= UART_PROTOCOL1_RX_FIFO_SIZE;
			pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;

			return;
		}
		
		// 命令头正确，但无临时缓冲区可用，退出
		if((pCB->rx.end + 1)%UART_PROTOCOL1_RX_QUEUE_SIZE == pCB->rx.head)
		{
			return;
		}

		// 添加UART通讯超时时间设置-2016.1.5增加
#if UART_PROTOCOL1_RX_TIME_OUT_CHECK_ENABLE
		TIMER_AddTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL,
						UART_PROTOCOL1_BUS_UNIDIRECTIONAL_TIME_OUT,
						UART_PROTOCOL1_CALLBACK_RxTimeOut,
						0,
						1,
						ACTION_MODE_ADD_TO_QUEUE);
#endif
		
		// 命令头正确，有临时缓冲区可用，则将其添加到命令帧临时缓冲区中
		pCmdFrame->buff[pCmdFrame->length++]= currentData;
		pCB->rxFIFO.currentProcessIndex ++;
		pCB->rxFIFO.currentProcessIndex %= UART_PROTOCOL1_RX_FIFO_SIZE;
	}
	// 非首字节，将数据添加到命令帧临时缓冲区中，但暂不删除当前数据
	else
	{
		// 临时缓冲区溢出，说明当前正在接收的命令帧是错误的，正确的命令帧不会出现长度溢出的情况
		if(pCmdFrame->length >= UART_PROTOCOL1_RX_CMD_FRAME_LENGTH_MAX)
		{
#if UART_PROTOCOL1_RX_TIME_OUT_CHECK_ENABLE
			// 停止RX通讯超时检测
			UART_PROTOCOL1_StopRxTimeOutCheck();
#endif

			// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
			pCmdFrame->length = 0;	// 2016.1.5增加
			// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
			pCB->rxFIFO.head ++;
			pCB->rxFIFO.head %= UART_PROTOCOL1_RX_FIFO_SIZE;
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
			pCB->rxFIFO.currentProcessIndex %= UART_PROTOCOL1_RX_FIFO_SIZE;

			// ■■接下来，需要检查命令帧是否完整，如果完整，则将命令帧临时缓冲区扶正 ■■
						
			// 首先判断命令帧最小长度，一个完整的命令字至少包括4个字节: 命令帧最小长度，包含:设备地址、数据长度、命令字、校验和，因此不足4个字节的必定不完整
			if(pCmdFrame->length < UART_PROTOCOL1_CMD_FRAME_LENGTH_MIN)
			{
				// 继续接收
				continue;
			}

			// 命令帧长度数值越界，说明当前命令帧错误，停止接收
			if(pCmdFrame->buff[UART_PROTOCOL1_CMD_LENGTH_INDEX] > UART_PROTOCOL1_RX_CMD_FRAME_LENGTH_MAX)
			{
#if UART_PROTOCOL1_RX_TIME_OUT_CHECK_ENABLE
				// 停止RX通讯超时检测
				UART_PROTOCOL1_StopRxTimeOutCheck();
#endif
			
				// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
				pCmdFrame->length = 0;
				// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
				pCB->rxFIFO.head ++;
				pCB->rxFIFO.head %= UART_PROTOCOL1_RX_FIFO_SIZE;
				pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;

				return;
			}

			// 命令帧长度校验
			length = pCmdFrame->length;
			if(length < pCmdFrame->buff[UART_PROTOCOL1_CMD_LENGTH_INDEX])
			{
				// 长度要求不一致，说明未接收完毕，退出继续
				continue;
			}

			// 命令帧长度OK，则进行校验，失败时删除命令头
			if(!UART_PROTOCOL1_CheckSUM(pCmdFrame))
			{
#if UART_PROTOCOL1_RX_TIME_OUT_CHECK_ENABLE
				// 停止RX通讯超时检测
				UART_PROTOCOL1_StopRxTimeOutCheck();
#endif
				
				// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
				pCmdFrame->length = 0;
				// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
				pCB->rxFIFO.head ++;
				pCB->rxFIFO.head %= UART_PROTOCOL1_RX_FIFO_SIZE;
				pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;
				
				return;
			}

#if UART_PROTOCOL1_RX_TIME_OUT_CHECK_ENABLE
			// 停止RX通讯超时检测
			UART_PROTOCOL1_StopRxTimeOutCheck();
#endif
			
			// 执行到这里，即说明接收到了一个完整并且正确的命令帧，此时需将处理过的数据从一级缓冲区中删除，并将该命令帧扶正
			pCB->rxFIFO.head += length;
			pCB->rxFIFO.head %= UART_PROTOCOL1_RX_FIFO_SIZE;
			pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;
			UART_PROTOCOL1_ConfirmTempCmdFrameBuff(pCB);

			return;
		}
	}
}

// 对传入的命令帧进行校验，返回校验结果
BOOL UART_PROTOCOL1_CheckSUM(UART_PROTOCOL1_RX_CMD_FRAME* pCmdFrame)
{
	uint16 cc = 0;
	uint16 i = 0;
	
	if(NULL == pCmdFrame)
	{
		return FALSE;
	}

	// 从设备地址开始，到校验码之前的一个字节，依次进行异或运算
	for(i=0; i<pCmdFrame->length-1; i++)
	{
		cc ^= pCmdFrame->buff[i];
	}
	
	// 判断计算得到的校验码与命令帧中的校验码是否相同
	if(pCmdFrame->buff[pCmdFrame->length-1] != cc)
	{
		return FALSE;
	}
	
	return TRUE;
}

// UART命令帧缓冲区处理
void UART_PROTOCOL1_CmdFrameProcess(UART_PROTOCOL1_CB* pCB)
{
	UART_PROTOCOL1_CMD cmd = UART_PROTOCOL1_CMD_NULL;
	UART_PROTOCOL1_RX_CMD_FRAME* pCmdFrame = NULL;
	uint32 tempVal, tempVal2;
	uint16 i;
	uint8* pBuff = NULL;
	BOOL bTemp;
	RET_RESULT lcdTestFlag;
	uint16 powerPassword, menuPassword;

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
	if(UART_PROTOCOL1_CMD_RX_DEVICE_ADDR != pCmdFrame->buff[UART_PROTOCOL1_CMD_DEVICE_ADDR_INDEX])
	{
		// 删除命令帧
		pCB->rx.head ++;
		pCB->rx.head %= UART_PROTOCOL1_RX_QUEUE_SIZE;
		return;
	}

	// 命令头合法，则提取命令
	cmd = (UART_PROTOCOL1_CMD)pCmdFrame->buff[UART_PROTOCOL1_CMD_CMD_INDEX];
	
	// 执行命令帧
	switch(cmd)
	{
		// 空命令，不予执行
		case UART_PROTOCOL1_CMD_NULL:
			break;

		// 正常运行状态
		case UART_PROTOCOL1_CMD_HMI_CTRL_RUN:

			UART_PROTOCOL1_OnReceiveReport(&pCmdFrame->buff[UART_PROTOCOL1_CMD_DATA1_INDEX], UART_PROTOCOL1_RX_PARAM_SIZE);
						
			break;

		default:
			break;
	}

	// 启动UART总线通讯超时判断
	UART_PROTOCOL1_StartTimeoutCheckTask();
	
	// 删除命令帧
	pCB->rx.head ++;
	pCB->rx.head %= UART_PROTOCOL1_RX_QUEUE_SIZE;
}

// RX通讯超时处理-单向
#if UART_PROTOCOL1_RX_TIME_OUT_CHECK_ENABLE
void UART_PROTOCOL1_CALLBACK_RxTimeOut(uint32 param)
{
	UART_PROTOCOL1_RX_CMD_FRAME* pCmdFrame = NULL;

	pCmdFrame = &uartProtocolCB1.rx.cmdQueue[uartProtocolCB1.rx.end];

	// 超时错误，将命令帧长度清零，即认为抛弃该命令帧
	pCmdFrame->length = 0;	// 2016.1.6增加
	// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
	uartProtocolCB1.rxFIFO.head ++;
	uartProtocolCB1.rxFIFO.head %= UART_PROTOCOL1_RX_FIFO_SIZE;
	uartProtocolCB1.rxFIFO.currentProcessIndex = uartProtocolCB1.rxFIFO.head;
}

// 停止Rx通讯超时检测任务
void UART_PROTOCOL1_StopRxTimeOutCheck(void)
{
	TIMER_KillTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL);
}
#endif

// 码表设置立刻下发数据请求标志
void UART_PROTOCOL1_SetTxAtOnceRequest(uint32 param)
{
	uartProtocolCB1.txAtOnceRequest = (BOOL)param;
}

// 码表设置周期下发数据请求标志
void UART_PROTOCOL1_CALLBACK_SetTxPeriodRequest(uint32 param)
{
	uartProtocolCB1.txPeriodRequest = (BOOL)param;
}

// 码表发送数据通讯时序处理
void UART_PROTOCOL1_DataCommunicationSequenceProcess(void)
{
	// 立即发送请求或是周期性发送请求
	if ((uartProtocolCB1.txAtOnceRequest) || (uartProtocolCB1.txPeriodRequest))
	{
		// 发送运行参数命令
		UART_PROTOCOL1_SendRuntimeParam();

		if (uartProtocolCB1.txAtOnceRequest)
		{
			// 重新复位参数下发定时器
			TIMER_ResetTimer(TIMER_ID_PROTOCOL_PARAM_TX);
		}

		uartProtocolCB1.txAtOnceRequest = FALSE;
		uartProtocolCB1.txPeriodRequest = FALSE;
	}
}

// 	UART总线超时错误处理
void UART_PROTOCOL1_CALLBACK_UartBusError(uint32 param)
{
	PARAM_SetErrorCode(ERROR_TYPE_COMMUNICATION_TIME_OUT);

}

// 发送运行参数命令到驱动器
void UART_PROTOCOL1_SendRuntimeParam(void)
{
	static uint8 assistCode;
	uint16 wheelSize;
	
	uint16 pwm;
	uint16 lowVoltageThreshold;

	// 添加设备地址
	UART_PROTOCOL1_TxAddData(UART_PROTOCOL1_CMD_TX_DEVICE_ADDR);

	// 添加数据长度，固定为20
	UART_PROTOCOL1_TxAddData(20);

	// 添加命令字
	UART_PROTOCOL1_TxAddData(UART_PROTOCOL1_CMD_HMI_CTRL_RUN);

	// 驱动方式设定
	// 0 : 助力驱动(通过助力档位决定输出多少助力，此时转把无效)。
	// 1 : 电驱动(通过转把驱动, 此时助力档位无效)。
	// 2 : 助力驱动和电驱动同时共存(电驱动零启动状态下无效)。
	UART_PROTOCOL1_TxAddData(PARAM_GetDriverControlMode());

	// 助力档位
	switch (PARAM_GetMaxAssist())
	{
		case ASSIST_3:
			switch (PARAM_GetAssistLevel())
			{
				case ASSIST_0:
					assistCode = 0;
					break;

				case ASSIST_1:
					assistCode = 5;
					break;

				case ASSIST_2:
					assistCode = 10;
					break;

				case ASSIST_3:
					assistCode = 15;
					break;

				// 推车
				case ASSIST_P:
					assistCode = assistCode;
					break;
			}
			break;

		case ASSIST_5:
			switch (PARAM_GetAssistLevel())
			{
				case ASSIST_0:
					assistCode = 0;
					break;

				case ASSIST_1:
					assistCode = 3;
					break;

				case ASSIST_2:
					assistCode = 6;
					break;

				case ASSIST_3:
					assistCode = 9;
					break;

				case ASSIST_4:
					assistCode = 12;
					break;

				case ASSIST_5:
					assistCode = 15;
					break;

				// 推车
				case ASSIST_P:
					assistCode = assistCode;
					break;

			}
			break;

		case ASSIST_9:
			switch (PARAM_GetAssistLevel())
			{
				case ASSIST_0:
					assistCode = 0;
					break;

				case ASSIST_1:
					assistCode = 1;
					break;

				case ASSIST_2:
					assistCode = 3;
					break;

				case ASSIST_3:
					assistCode = 5;
					break;

				case ASSIST_4:
					assistCode = 7;
					break;

				case ASSIST_5:
					assistCode = 9;
					break;

				case ASSIST_6:
					assistCode = 11;
					break;

				case ASSIST_7:
					assistCode = 13;
					break;

				case ASSIST_8:
					assistCode = 14;
					break;

				case ASSIST_9:
					assistCode = 15;
					break;

				// 推车
				case ASSIST_P:
					assistCode = assistCode;
					break;
			}
			break;
	}
	UART_PROTOCOL1_TxAddData(assistCode);

	// 控制器控制设定1
	// bit7：0: 控制器不工作，1: 控制器工作
	// bit6：0: 零启动 1: 非零启动
	// bit5：灯光控制 0: 关闭 1: 开
	// bit4：0: 通讯正常 1: 通讯故障(不能接收到控制器的数据)
	// bit3：切换巡航的方式
	//		 0: 使用触发方式，是否巡航见BIT0 (仪表默认状态)
	//		 1: 使用长按键方式 见BIT1
	//		 多数情况下，控制器出厂时，巡航方式已确定，可以不考虑本Bit3，直接根据需要读取Bit1，或者Bit0 即可
	// bit2：限速状态 1：超过限速值；0：未超过限速值（不限速一直为0)；
	// bit1：巡航保持模式 (0: 非巡航，1: 巡航中)
	//		(如果要”6KM/助推“ 这种按住保持，松手推出的巡航，控制器可以直接读此位置）
	// bit0：巡航切换模式 (0: 不需要切换巡航状态 1: 要切换巡航状态)
	//		（如果需要按住后反转状态的巡航，比如定速巡航，可以直接读此位置）
	hmiDriveCB1.DriverSet1.param.driverState = 1;
	hmiDriveCB1.DriverSet1.param.zeroStartOrNot = PARAM_GetZeroStartOrNot();
	hmiDriveCB1.DriverSet1.param.isLightOn = PARAM_GetLightSwitch();
	hmiDriveCB1.DriverSet1.param.comunicationError = 0;
	hmiDriveCB1.DriverSet1.param.switchCruiseWay = PARAM_GetSwitchCruiseWay();

	// 限速使能
	if (PARAM_GetSpeedLimitSwitch())
	{
		// 超过限速值
		if (paramCB.runtime.speed > paramCB.nvm.param.common.speed.limitVal)
		{
			hmiDriveCB1.DriverSet1.param.speedLimitState = 1;
		}
		// 未超过限速值
		else
		{
			hmiDriveCB1.DriverSet1.param.speedLimitState = 0;
		}
	}
	// 不限速一直为0
	else
	{
		hmiDriveCB1.DriverSet1.param.speedLimitState = 0;
	}


	// 定时巡航和6KM助推
	if (PARAM_GetSwitchCruiseWay())	// 长按键方式
	{		
		hmiDriveCB1.DriverSet1.param.switchCruiseMode = 0;
	}
	else							// 触发方式
	{
		if (PARAM_GetCruiseSwitch())
		{
			hmiDriveCB1.DriverSet1.param.switchCruiseMode = 1;

			// 巡航开关使用后立刻关闭
			PARAM_SetCruiseSwitch(FALSE);
		}
		else
		{
			hmiDriveCB1.DriverSet1.param.switchCruiseMode = 0;
		}

	}

	// 推车助力状态
	hmiDriveCB1.DriverSet1.param.isPushModeOn = PARAM_IsPushAssistOn();
	
	UART_PROTOCOL1_TxAddData(hmiDriveCB1.DriverSet1.val);

	// 测速磁钢
	UART_PROTOCOL1_TxAddData(PARAM_GetCycleOfSteelNum());


	wheelSize = PARAM_GetNewWheelSizeInch();
	UART_PROTOCOL1_TxAddData(wheelSize>>8);
	UART_PROTOCOL1_TxAddData(wheelSize&0xFF);

	// 助力灵敏度
	UART_PROTOCOL1_TxAddData(PARAM_GetAssistSensitivity());

	// 助力启动强度
	UART_PROTOCOL1_TxAddData(PARAM_GetAssistStartIntensity());

	// 限速专用电机换向霍尔磁钢片数
	UART_PROTOCOL1_TxAddData(PARAM_GetReversalHolzerSteelNum());

	// 限速值，此处一定要用公制km/h
	UART_PROTOCOL1_TxAddData(paramCB.nvm.param.common.speed.limitVal/10);

	// 控制器限流值
	UART_PROTOCOL1_TxAddData(PARAM_GetCurrentLimit()/1000);		// 客户的单位为1A

	// 控制器欠压值
	lowVoltageThreshold = PARAM_GetLowVoltageThreshold()/100;
	UART_PROTOCOL1_TxAddData(lowVoltageThreshold>>8);			// 客户的单位为0.1V
	UART_PROTOCOL1_TxAddData(lowVoltageThreshold&0xFF);

	// 转把调速PWM占空比
	//pwm = PARAM_GetPwmTopLimit(PARAM_GetAssistLevel())*10;			// 客户的单位为0.1%
	pwm = 0;
	UART_PROTOCOL1_TxAddData(pwm>>8);
	UART_PROTOCOL1_TxAddData(pwm&0xFF);

	// 控制器控制设定2 + 助力磁钢盘磁钢个数
	// bit7：倒车标志 0: 无效，1: 倒车
	// bit6：自动巡航标志 0: 无自动巡航， 1: 有自动巡航（时间控制器确定）
	// bit5：备用0
	// bit4：备用0
	// bit3 2 1 0：助力磁钢盘磁钢个数
	// 			   5 : 5 颗磁钢
	// 			   8 : 8 颗磁钢
	// 			   12: 12 颗磁钢
	hmiDriveCB1.DriverSet2.param.astern = 0;
	hmiDriveCB1.DriverSet2.param.autoCruise = 0;
	hmiDriveCB1.DriverSet2.param.assistSteelType = PARAM_GetSteelType();
	UART_PROTOCOL1_TxAddData(hmiDriveCB1.DriverSet2.val);

	// 添加检验和，并添加至发送
	UART_PROTOCOL1_TxAddFrame();
}

// 驱动器上报参数处理
void UART_PROTOCOL1_OnReceiveReport(uint8* pBuff, uint32 len)
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
	for (i = 0; (i < len) && (i < UART_PROTOCOL1_RX_PARAM_SIZE); i++)
	{
		hmiDriveCB1.rx.buff[i] = pBuff[i];
	}

	// 到这里，即把驱动器上报的数据放入了 hmiDriveCB1.rx.buff 中

	// 提取故障状态
	// 刹把故障
	if (hmiDriveCB1.rx.param.breakError)
	{
		PARAM_SetErrorCode(ERROR_TYPE_BREAK_ERROR);
	}
	// 欠压故障
	else if (hmiDriveCB1.rx.param.underVoltageProtectionError)
	{
		PARAM_SetErrorCode(ERROR_TYPE_BATTERY_UNDER_VOLTAGE_ERROR);
	}
	// 电机缺相故障
	else if (hmiDriveCB1.rx.param.motorPhaseError)
	{
		PARAM_SetErrorCode(ERROR_TYPE_MOTOR_ERROR);
	}
	// 转把故障
	else if (hmiDriveCB1.rx.param.turnBarError)
	{
		PARAM_SetErrorCode(ERROR_TYPE_TURN_ERROR);
	}
	// 控制器故障
	else if (hmiDriveCB1.rx.param.driverError)
	{
		PARAM_SetErrorCode(ERROR_TYPE_DRIVER_ERROR);
	}
	// 霍尔故障
	else if (hmiDriveCB1.rx.param.holzerError)
	{
		PARAM_SetErrorCode(ERROR_TYPE_HALL_ERROR);
	}
	// 能收到命令，又排除所有的故障，确定无错误了
	else
	{
		PARAM_SetErrorCode(ERROR_TYPE_NO_ERROR);
	}

	// 提取定时巡航状态
	if (hmiDriveCB1.rx.param.cruiseState)
	{
		PARAM_SetCruiseState(TRUE);
	}
	else
	{
		PARAM_SetCruiseState(FALSE);
	}

	// 提取6KM推车助力状态
	if (hmiDriveCB1.rx.param.cruise6kmState)
	{
		PARAM_SetPushAssistState(TRUE);
	}
	else
	{
		PARAM_SetPushAssistState(FALSE);
	}

	// 提取运行电流
	// 预留高字节的bit7为正负，目前保持为0; 当高字节的bit6为1时，表示单位为0.1A，为0时，电流的单位依然是1A
	tempVal = hmiDriveCB1.rx.param.ucCurBatteryCurrentH;
	tempVal <<= 8;
	tempVal |= hmiDriveCB1.rx.param.ucCurBatteryCurrentL;
	if (tempVal & 0x4000)
	{
		tempVal &= ~0xC000;
		PARAM_SetBatteryCurrent((uint16)(tempVal*0.1f*1000));
	}
	else
	{
		tempVal &= ~0xC000;
		PARAM_SetBatteryCurrent((uint16)(tempVal*1000));
	}

	// 提取实时速度，单位:KM/H
	tempVal = 0;
	tempVal = hmiDriveCB1.rx.param.oneCycleTimeH;
	tempVal <<= 8;
	tempVal |= hmiDriveCB1.rx.param.oneCycleTimeL;
	// 建诺孙工确认大于等于3000时速度为零
	// 当收到控制器上报转一圈时间为3000时，就认为速度为0
	if(tempVal >= 3000UL)
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


	if (BATTERY_DATA_SRC_CONTROLLER == PARAM_GetBatteryDataSrc())
	{
		// 锂电2号没有电压上报的字段，故为了兼容人为采用固定电压
		tempVal = PARAM_GetBatteryVoltageLevel();
		tempVal *= 1000;											// 单位转换: 1V转换为1000mV	
		
		PARAM_SetBatteryVoltage(tempVal);
	}
	else if (BATTERY_DATA_SRC_BMS == PARAM_GetBatteryDataSrc())
	{
		// 电量
		PARAM_SetBatteryPercent(hmiDriveCB1.rx.param.batteryCapacity);
	}

}


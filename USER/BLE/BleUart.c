#include "common.h"
#include "delay.h"
#include "system.h"
#include "timer.h"
#include "BleUart.h"
#include "Param.h"
#include "BleProtocol.h"
#include "stringOperation.h"

/******************************************************************************
* 【内部接口声明】
******************************************************************************/
// UART初始化
void BLE_UART_HwInit(uint32 baud);

// 数据结构初始化
void BLE_UART_DataStructureInit(BLE_UART_CB* pCB);

// 发送处理
void BLE_UART_TxProcess(BLE_UART_CB* pCB);

// 启动中断字节发送
void BLE_UART_IR_StartSendData(uint8 data);

// 阻塞发送一个字节数据
//void BLE_UART_BC_SendData(uint8 data);

// UART接收与发送中断处理，用于内部通讯
void USART3_IRQHandler(void);

// BLE模块辅助控制引脚
void BLE_CMD_GPIO_CtrlPin(void);

// 全局变量定义
BLE_UART_CB bleUartCB;

// UART初始化
void BLE_UART_HwInit(uint32 baud)
{
	GPIO_InitType  GPIO_InitStructure;
	USART_InitType USART_InitStructure;
	NVIC_InitType  NVIC_InitStructure;	
		
	// 时钟配置 
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);		// 使能GPIO时钟

	// Initialize GPIO_InitStructure
	GPIO_InitStruct(&GPIO_InitStructure);

	// Configure USART Tx as alternate function push-pull
	GPIO_InitStructure.Pin            = GPIO_PIN_4;
	GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
	GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
	GPIO_InitStructure.GPIO_Pull      = GPIO_No_Pull;
	GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Alternate = GPIO_AF1_USART1;
	GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);

	// Configure USART Rx as alternate function push-pull and pull-up
	GPIO_InitStructure.Pin            = GPIO_PIN_5;
	GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
	GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
	GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
	GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Input;
	GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART1;
	GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);// 上拉输入

	// UART时钟使能
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_USART1, ENABLE);		// 使能USART时钟
	
	RCC_EnableAPB2PeriphReset(RCC_APB2_PERIPH_USART1, ENABLE);		// 复位串口
	RCC_EnableAPB2PeriphReset(RCC_APB2_PERIPH_USART1, DISABLE); 	// 停止复位

	// 串口配置
	USART_InitStructure.BaudRate = baud;							// 一般设置为9600;
	USART_InitStructure.WordLength = USART_WL_8B; 					// 8位数据长度
	USART_InitStructure.StopBits = USART_STPB_1;					// 一个停止位
	USART_InitStructure.Parity = USART_PE_NO; 						// 无奇偶校验位
	USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE; 	// 无硬件数据流控制
	USART_InitStructure.Mode = USART_MODE_RX | USART_MODE_TX; 		// 收发模式
	USART_Init(BLE_UART_TYPE_DEF, &USART_InitStructure);			// 初始化串口

	// 中断配置
	NVIC_InitStructure.NVIC_IRQChannel = BLE_UART_IRQn_DEF;		// 使能串口中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	// 先占优先级2级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;			// 从优先级1级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 			// 使能外部中断通道
	NVIC_Init(&NVIC_InitStructure); 							// 根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
 
	USART_ConfigInt(BLE_UART_TYPE_DEF, USART_INT_RXDNE, ENABLE);	// 开启中断
 
	USART_Enable(BLE_UART_TYPE_DEF, ENABLE);						// 使能串口
}

// 数据结构初始化
void BLE_UART_DataStructureInit(BLE_UART_CB* pCB)
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
	for(i = 0; i < BLE_UART_DRIVE_TX_QUEUE_SIZE; i++)
	{
		pCB->tx.cmdQueue[i].length = 0;
	}	

	bleUartCB.rcvBleMacOK = FALSE;
}

// 发送处理
void BLE_UART_TxProcess(BLE_UART_CB* pCB)
{
// 中断方式
#if (UART_DRIVE_TX_MODE == UART_DRIVE_INTERRUPT_TX_MODE)
	uint16 index = pCB->tx.index;							// 当前发送数据的索引号
	uint16 length = pCB->tx.cmdQueue[pCB->tx.head].length;	// 当前发送的命令帧的长度
	uint16 head = pCB->tx.head;								// 发送命令帧队列头索引号
	uint16 end = pCB->tx.end;								// 发送命令帧队列尾索引号

	// 参数合法性检验
	if (NULL == pCB)
	{
		return;
	}
	
	// 队列为空，不处理
	if (head == end)
	{
		return;
	}

	// 发送忙，退出
	if (pCB->tx.txBusy)
	{
		return;
	}

	// ■■执行到这里，说明队列非空■■
	
	// 当前命令帧未发送完时，取出一个字节放到发送寄存器中
	if (index < length)
	{		
		BLE_UART_IR_StartSendData(pCB->tx.cmdQueue[head].buff[pCB->tx.index++]);
		
		// 设置发送忙状态
		pCB->tx.txBusy = TRUE;
	}


// 阻塞方式时启动发送
#else
	uint16 index = pCB->tx.index;
	uint16 head = pCB->tx.head;
	uint16 end = pCB->tx.end;

	// 参数合法性检验
	if(NULL == pCB)
	{
		return;
	}

	// 队列为空，不处理
	if(head == end)
	{
		return;
	}
	
	// 当前命令帧未发送完时，持续发送
	while(index < pCB->tx.cmdQueue[head].length)
	{
		// 一直填充发送
		BLE_UART_BC_SendData(pCB->tx.cmdQueue[head].buff[pCB->tx.index++]);
		
		index = pCB->tx.index;
	}
	
	// 当前命令帧发送完时，删除之
	pCB->tx.cmdQueue[head].length = 0;
	pCB->tx.head ++;
	pCB->tx.head %= BLE_UART_DRIVE_TX_QUEUE_SIZE;
	pCB->tx.index = 0;
#endif
}

#if (BLE_UART_TX_MODE == BLE_UART_INTERRUPT_TX_MODE)
// 启动中断字节发送
void BLE_UART_IR_StartSendData(uint8 data)
{
	// 先读SR，再填充DR会把TC标志清掉
	USART_GetFlagStatus(BLE_UART_TYPE_DEF, USART_FLAG_TXC);

	// 发送一个字节
	USART_SendData(BLE_UART_TYPE_DEF, data);

	// 打开发送完成中断
	USART_ConfigInt(BLE_UART_TYPE_DEF, USART_INT_TXC, ENABLE);	
}

#else
// 阻塞发送一个字节数据
void BLE_UART_BC_SendData(uint8 data)
{	
	// 防止丢失第一个字节(清除空标志位,务必加) 
	USART_ClrFlag(BLE_UART_TYPE_DEF, USART_FLAG_TXC);		

	// 填充数据
	USART_SendData(BLE_UART_TYPE_DEF, data);

	// 未发送完，持续等待
	while(USART_GetFlagStatus(BLE_UART_TYPE_DEF, USART_FLAG_TXC) != SET);		
}
#endif

/**************************************************************
 * @brief  void USARTx_IRQHandler(void);
 * @input  串口x接收函数，串口x的中断发送函数
 * @output None
 * @return None	
 * @Notes					
*************************************************************/
uint8 bleRxBuff[100] = {0};
uint8 bleIndex = 0;
void BLE_UART_BleConnectState(uint32 param)
{	
	bleIndex = 0;

	if (-1 != STRING_Find(bleRxBuff, "TTM:CONNECTED"))
	{
		PARAM_SetBleConnectState(TRUE);
		
		// 开启鉴权定时器，10s钟内如果没有鉴权成功则退出蓝牙连接
		TIMER_AddTask(TIMER_ID_BLE_CONNECT_VERIFICATE,
					10000,
					BLE_MODULE_Reset,
					0,
					1,
					ACTION_MODE_ADD_TO_QUEUE);

	}

	if (-1 != STRING_Find(bleRxBuff, "TTM:DISCONNECT"))
	{
		PARAM_SetBleConnectState(FALSE);
	}
}

// 接收蓝牙mac地址
void BLE_UART_AtChannelDataProcess(uint8 data)
{
	uint8 i;
	
	if (!bleUartCB.rx.startFlag)
	{
		if (data == 'T')				// 起始标志
		{
			bleUartCB.rx.startFlag = TRUE;

			bleUartCB.rx.index = 0;

			bleUartCB.rx.fifoBuff[bleUartCB.rx.index++] = data;		
			
			return;
		}
	}
	else
	{
		// 连续接收数据
		bleUartCB.rx.fifoBuff[bleUartCB.rx.index++] = data;

		// 越界判断
		if (bleUartCB.rx.index >= sizeof(bleUartCB.rx.fifoBuff))
		{
			bleUartCB.rx.startFlag = TRUE;
			bleUartCB.rx.index = 0;

			return;
		}
		
		if (data == '\n')			// 收到结束标志
		{
			bleUartCB.rx.startFlag = FALSE;
			bleUartCB.rx.index = 0;

			bleUartCB.rx.macBuff[2] = ':';
			bleUartCB.rx.macBuff[5] = ':';
			bleUartCB.rx.macBuff[8] = ':';
			bleUartCB.rx.macBuff[11] = ':';
			bleUartCB.rx.macBuff[14] = ':';
			bleUartCB.rx.macBuff[17] = '\0';

			// 过滤回显指令
			if ((strstr((const char*)bleUartCB.rx.fifoBuff, (const char*)"TTM:MAC-?") != NULL))
			{
				return ;
			}

			// 将读到的蓝牙mac地址提取出来
			if ((strstr((const char*)bleUartCB.rx.fifoBuff, (const char*)"TTM:MAC-") != NULL) 
			&& (bleUartCB.rx.fifoBuff[0] == 'T') && (bleUartCB.rx.fifoBuff[1] == 'T') && (bleUartCB.rx.fifoBuff[2] == 'M'))
			{
				bleUartCB.rx.macBuff[0] = bleUartCB.rx.fifoBuff[8];
				bleUartCB.rx.macBuff[1] = bleUartCB.rx.fifoBuff[9];

				bleUartCB.rx.macBuff[3] = bleUartCB.rx.fifoBuff[10];
				bleUartCB.rx.macBuff[4] = bleUartCB.rx.fifoBuff[11];

				bleUartCB.rx.macBuff[6] = bleUartCB.rx.fifoBuff[12];
				bleUartCB.rx.macBuff[7] = bleUartCB.rx.fifoBuff[13];

				bleUartCB.rx.macBuff[9] = bleUartCB.rx.fifoBuff[14];
				bleUartCB.rx.macBuff[10] = bleUartCB.rx.fifoBuff[15];

				bleUartCB.rx.macBuff[12] = bleUartCB.rx.fifoBuff[16];
				bleUartCB.rx.macBuff[13] = bleUartCB.rx.fifoBuff[17];

				bleUartCB.rx.macBuff[15] = bleUartCB.rx.fifoBuff[18];
				bleUartCB.rx.macBuff[16] = bleUartCB.rx.fifoBuff[19];

				//for (i = 0; i < 12; i++)
				//{
				//	bleUartCB.rx.macBuff[i] = bleUartCB.rx.fifoBuff[i+10];
				//}
				//bleUartCB.rx.macBuff[i] = '\0';
				
				for (i = 0; i < sizeof(bleUartCB.rx.fifoBuff); i++)
				{
					bleUartCB.rx.fifoBuff[i] = 0;
				}				

				// 注销该定时器
				TIMER_KillTask(TIMER_ID_BLE_CONTROL);
				
				// 蓝牙地址接收完成
				bleUartCB.rcvBleMacOK = TRUE;
			}	
		}
	}
}


/**************************************************************
 * @brief  void USARTx_IRQHandler(void);
 * @input  串口x接收函数，串口x的中断发送函数
 * @output None
 * @return None	
 * @Notes					
*************************************************************/
void USART1_IRQHandler(void)
{
	// 判断DR是否有数据，中断接收
	if (USART_GetIntStatus(BLE_UART_TYPE_DEF, USART_INT_RXDNE) != RESET) 
	{
		uint8 rxdata = 0x00;
		
		// 接收数据
		rxdata = (uint8)USART_ReceiveData(BLE_UART_TYPE_DEF);
		
		if (NULL != bleUartCB.receiveDataThrowService)
		{			
			(*bleUartCB.receiveDataThrowService)(0xFFF, &rxdata, 1);
		}

		if (!bleUartCB.rcvBleMacOK)
		{
			BLE_UART_AtChannelDataProcess(rxdata);
		}

//=================================================================
		// 蓝牙连接判断
		if ((bleIndex + 1)< sizeof(bleRxBuff)/sizeof(bleRxBuff[0]))
		{
			bleRxBuff[bleIndex++] = rxdata;
			bleRxBuff[bleIndex] = '\0';
		}

		TIMER_AddTask(TIMER_ID_BLE_CONNECT_STATE,
					100,
					BLE_UART_BleConnectState,
					0,
					1,
					ACTION_MODE_ADD_TO_QUEUE);
	}


// 中断模式发送
#if (BLE_UART_TX_MODE == BLE_UART_INTERRUPT_TX_MODE)
	// 判断DR是否有数据，中断发送
	if (USART_GetIntStatus(BLE_UART_TYPE_DEF, USART_INT_TXC) != RESET)
	{
		uint16 head = bleUartCB.tx.head;
		uint16 end;
		uint16 index = bleUartCB.tx.index;
		uint8 txdata = 0x00;

		// 执行到这里，说明上一个数据已经发送完毕，当前命令帧未发送完时，取出一个字节放到发送寄存器中
		if (index < bleUartCB.tx.cmdQueue[head].length)
		{
			txdata = bleUartCB.tx.cmdQueue[head].buff[bleUartCB.tx.index++];
			
			// 填充数据
			USART_SendData(BLE_UART_TYPE_DEF, txdata);
		}
		// 当前命令帧发送完时，删除之
		else
		{
			bleUartCB.tx.cmdQueue[head].length = 0;
			bleUartCB.tx.head ++;
			bleUartCB.tx.head %= BLE_UART_DRIVE_TX_QUEUE_SIZE;
			bleUartCB.tx.index = 0;

			head = bleUartCB.tx.head;
			end = bleUartCB.tx.end;
			
			// 命令帧队列非空，继续发送下一个命令帧
			if (head != end)
			{
				txdata = bleUartCB.tx.cmdQueue[head].buff[bleUartCB.tx.index++];

				// 填充数据
				USART_SendData(BLE_UART_TYPE_DEF, txdata);
			}
			// 命令帧队列为空停止发送，设置空闲
			else
			{
				// 关闭发送完成中断
				USART_ConfigInt(BLE_UART_TYPE_DEF, USART_INT_TXC, DISABLE);
				
				bleUartCB.tx.txBusy = FALSE;				
			}
		}		
	}
#endif

	// Other USART3 interrupts handler can go here ...				 
	if (USART_GetIntStatus(BLE_UART_TYPE_DEF, USART_INT_OREF) != RESET)	//----------------------- 接收溢出中断 
	{
		USART_GetIntStatus(BLE_UART_TYPE_DEF, USART_INT_OREF); 			//----------------------- 清除接收溢出中断标志位 
		USART_ReceiveData(BLE_UART_TYPE_DEF);							//----------------------- 清空寄存器
	}
}

// BLE模块辅助控制引脚
void BLE_CMD_GPIO_CtrlPin(void)
{
	
}

// UART初始化
void BLE_UART_Init(void)
{
	// BLE模块辅助控制引脚初始化
	//BLE_CMD_GPIO_CtrlPin();

	// 串口初始化
	BLE_UART_HwInit(BLE_UART_BAUD_RATE);

	// UART数据结构初始化
	BLE_UART_DataStructureInit(&bleUartCB);
}

// UART模块处理入口
void BLE_UART_Process(void)
{
	// 发送处理
	BLE_UART_TxProcess(&bleUartCB);
}

// 注册数据抛出接口服务
void BLE_UART_RegisterDataSendService(void (*service)(uint16 id, uint8 *pData, uint16 length))
{
	bleUartCB.receiveDataThrowService = service;
}

// 向发送缓冲区中添加一条待发送序列
BOOL BLE_UART_AddTxArray(uint16 id, uint8 *pArray, uint16 length)
{
	uint16 i;
	uint16 head = bleUartCB.tx.head;
	uint16 end = bleUartCB.tx.end;
	
	// 参数检验
	if ((NULL == pArray) || (0 == length))
	{
		return FALSE;
	}

	// 发送缓冲区已满，不予接收
	if((end + 1) % BLE_UART_DRIVE_TX_QUEUE_SIZE == head)
	{
		return FALSE;
	}

	bleUartCB.tx.cmdQueue[end].deviceID = id;
	for (i = 0; i < length; i++)
	{
		bleUartCB.tx.cmdQueue[end].buff[i] = *pArray++;
	}
	bleUartCB.tx.cmdQueue[end].length = length;
	
	// 发送环形队列更新位置
	bleUartCB.tx.end ++;
	bleUartCB.tx.end %= BLE_UART_DRIVE_TX_QUEUE_SIZE;
	bleUartCB.tx.cmdQueue[bleUartCB.tx.end].length = 0;

	return TRUE;
}

// BLE模块复位
void BLE_MODULE_Reset(uint32 param)
{
	// 蓝牙复位
	BLE_RST_RESET();
	Delayms(10);
	
	// 释放复位
	BLE_RST_RELEASE();

	// 允许开始广播
	BLE_EN_ENABLE();
}



#include "uartDrive.h"
#include "param.h"
/******************************************************************************
* 【内部接口声明】
******************************************************************************/

// 数据结构初始化
void UART_DRIVE_DataStructInit(UART_DRIVE_CB* pCB);

// UART初始化
void UART_DRIVE_HwInit(uint32 baud);

// 发送处理
void UART_DRIVE_TxProcess(UART_DRIVE_CB* pCB);

// 启动中断字节发送
void UART_DRIVE_IR_StartSendData(uint8 data);

// 阻塞发送一个字节数据
void UART_DRIVE_BC_SendData(uint8 data);

// UART接收与发送中断处理，用于内部通讯
void USART3_IRQHandler(void);


// 全局变量定义
UART_DRIVE_CB uartDriveCB;

// ■■函数定义区■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// UART初始化
void UART_DRIVE_Init(uint32 baud)
{	
	// 硬件UART配置
	UART_DRIVE_HwInit(baud);

	// UART数据结构初始化
	UART_DRIVE_DataStructInit(&uartDriveCB);
}

// UART模块处理入口
void UART_DRIVE_Process(void)
{
	// 内部通讯接口的发送处理
	UART_DRIVE_TxProcess(&uartDriveCB);
}

// 向协议层注册数据接收接口
void UART_DRIVE_RegisterDataSendService(void (*service)(uint16 id, uint8 *pData, uint16 length))
{
	uartDriveCB.receiveDataThrowService = service;
}

// 数据结构初始化
void UART_DRIVE_DataStructInit(UART_DRIVE_CB* pCB)
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
	for(i = 0; i < UART_DRIVE_TX_QUEUE_SIZE; i++)
	{
		pCB->tx.cmdQueue[i].length = 0;
	}
}

// UART初始化
void UART_DRIVE_HwInit(uint32 baud)
{
	GPIO_InitType GPIO_InitStructure;
	USART_InitType USART_InitStructure;
	NVIC_InitType NVIC_InitStructure;

	// Enable GPIO clock  
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOD, ENABLE);  
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);    
	// Enable USART Clock   
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART4, ENABLE);

    // Initialize GPIO_InitStructure
    GPIO_InitStruct(&GPIO_InitStructure);

    // Configure USART Tx as alternate function push-pull
    GPIO_InitStructure.Pin            = GPIO_PIN_13;
    GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
    GPIO_InitStructure.GPIO_Pull      = GPIO_No_Pull;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART4;
    GPIO_InitPeripheral(GPIOD, &GPIO_InitStructure);

    // Configure USART Rx as alternate function push-pull and pull-up
    GPIO_InitStructure.Pin            = GPIO_PIN_12;
    GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Input;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART4;
    GPIO_InitPeripheral(GPIOD, &GPIO_InitStructure);

	// GPIO配置
	// Initialize GPIO_InitStructure
	GPIO_InitStruct(&GPIO_InitStructure);

	GPIO_InitStructure.Pin			  = GPIO_PIN_6;
	GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
	GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
	GPIO_InitStructure.GPIO_Pull	  = GPIO_No_Pull;
	GPIO_InitStructure.GPIO_Mode	  = GPIO_Mode_Out_PP;
	GPIO_InitPeripheral(GPIOD, &GPIO_InitStructure);
	
	// 5V
	if (PARAM_GetUartLevel())
	{
		GPIO_ResetBits(GPIOD, GPIO_PIN_6);
	}
	else
	// 3.3V
	{
		GPIO_SetBits(GPIOD, GPIO_PIN_6);
	}

    // USART configuration
    USART_InitStructure.BaudRate            = baud;
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = USART_STPB_1;
    USART_InitStructure.Parity              = USART_PE_NO;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;
    USART_Init(UART_DRIVE_TYPE_DEF, &USART_InitStructure);

    // Enable the USART Interrupt
    NVIC_InitStructure.NVIC_IRQChannel            = UART_DRIVE_IRQn_DEF;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd         = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Enable USART Receive and Transmit interrupts   
    USART_ConfigInt(UART_DRIVE_TYPE_DEF, USART_INT_RXDNE, ENABLE);    
    //USART_ConfigInt(UART_DRIVE_TYPE_DEF, USART_INT_TXC, ENABLE);    
    // Enable the USART
    USART_Enable(UART_DRIVE_TYPE_DEF, ENABLE);
}

/*********************************************************
函数名: void UART0_IRQHandler(void)
描  述: UART中断服务程序
输入值: 无
输出值: 无
返回值: 无 
**********************************************************/
void UART4_IRQHandler(void)
{
	// 接收中断
	if (USART_GetIntStatus(UART_DRIVE_TYPE_DEF, USART_INT_RXDNE) != RESET)
	{
		uint8 rxdata = 0x00;
		
		// 接收数据
		rxdata = (uint8)USART_ReceiveData(UART_DRIVE_TYPE_DEF);
		
		if (NULL != uartDriveCB.receiveDataThrowService)
		{			
			(*uartDriveCB.receiveDataThrowService)(0xFFF, &rxdata, 1);
		}
	}
	
	// 中断模式发送
#if (UART_DRIVE_TX_MODE == UART_DRIVE_INTERRUPT_TX_MODE)
	// 判断DR是否有数据，中断发送
	if (USART_GetIntStatus(UART_DRIVE_TYPE_DEF, USART_INT_TXC) != RESET)
	{
		uint16 head = uartDriveCB.tx.head;
		uint16 end;
		uint16 index = uartDriveCB.tx.index;
		uint8 txdata = 0x00;

		// 清除发送中断标志
		USART_ClrIntPendingBit(UART_DRIVE_TYPE_DEF, USART_INT_TXC);

		// 执行到这里，说明上一个数据已经发送完毕，当前命令帧未发送完时，取出一个字节放到发送寄存器中
		if (index < uartDriveCB.tx.cmdQueue[head].length)
		{
			txdata = uartDriveCB.tx.cmdQueue[head].buff[uartDriveCB.tx.index++];
			
			// 填充数据
			USART_SendData(UART_DRIVE_TYPE_DEF, txdata);
		}
		// 当前命令帧发送完时，删除之
		else
		{
			uartDriveCB.tx.cmdQueue[head].length = 0;
			uartDriveCB.tx.head ++;
			uartDriveCB.tx.head %= UART_DRIVE_TX_QUEUE_SIZE;
			uartDriveCB.tx.index = 0;

			head = uartDriveCB.tx.head;
			end = uartDriveCB.tx.end;
			
			// 命令帧队列非空，继续发送下一个命令帧
			if (head != end)
			{
				txdata = uartDriveCB.tx.cmdQueue[head].buff[uartDriveCB.tx.index++];

				// 填充数据
				USART_SendData(UART_DRIVE_TYPE_DEF, txdata);
			}
			// 命令帧队列为空停止发送，设置空闲
			else
			{
				// 关闭发送完成中断
				USART_ConfigInt(UART_DRIVE_TYPE_DEF, USART_INT_TXC, DISABLE);
				
				uartDriveCB.tx.txBusy = FALSE;				
			}
		}		
	}
#endif

	// Other USARTx interrupts handler can go here ...				 
	if(USART_GetFlagStatus(UART_DRIVE_TYPE_DEF, USART_FLAG_OREF) != RESET) //----------------------- 接收溢出中断 	
	{
		USART_ClrFlag(UART_DRIVE_TYPE_DEF,USART_FLAG_OREF); 		  //----------------------- 清除接收溢出中断标志位 
		USART_ReceiveData(UART_DRIVE_TYPE_DEF);						  //----------------------- 清空寄存器
	}
}

// 启动中断字节发送
void UART_DRIVE_IR_StartSendData(uint8 data)
{
	// 先读SR，再填充DR会把TC标志清掉
	//UART_GetITStatus(UART_DRIVE_TYPE_DEF, UART_IT_TC);

	// 发送一个字节
	USART_SendData(UART_DRIVE_TYPE_DEF, data);

	// 打开发送完成中断
	USART_ConfigInt(UART_DRIVE_TYPE_DEF, USART_INT_TXC, ENABLE);
}

// 阻塞发送一个字节数据
void UART_DRIVE_BC_SendData(uint8 data)
{	
	// 防止丢失第一个字节(清除空标志位,务必加) 
	//UART_ClearITPendingBit(UART_DRIVE_TYPE_DEF, UART_CLR_TC);		

	// 填充数据
	USART_SendData(UART_DRIVE_TYPE_DEF, data);

	// 未发送完，持续等待
	while(USART_GetFlagStatus(UART_DRIVE_TYPE_DEF, USART_FLAG_TXC) == RESET);	
}

// 启动阻塞发送
void UART_DRIVE_BC_StartTx(UART_DRIVE_CB* pCB)
{
	uint16 index = pCB->tx.index;
	uint16 head = pCB->tx.head;
	uint16 end = pCB->tx.end;

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
	
	// 当前命令帧未发送完时，持续发送
	while(index < pCB->tx.cmdQueue[head].length)
	{
		// 一直填充发送
		UART_DRIVE_BC_SendData(pCB->tx.cmdQueue[head].buff[pCB->tx.index++]);
		
		index = pCB->tx.index;
	}
	
	// 当前命令帧发送完时，删除之
	pCB->tx.cmdQueue[head].length = 0;
	pCB->tx.head ++;
	pCB->tx.head %= UART_DRIVE_TX_QUEUE_SIZE;
	pCB->tx.index = 0;
}

// 发送处理,发现发送缓冲区非空时,启动中断发送
void UART_DRIVE_TxProcess(UART_DRIVE_CB* pCB)
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
		UART_DRIVE_IR_StartSendData(pCB->tx.cmdQueue[head].buff[pCB->tx.index++]);
		
		// 设置发送忙状态
		pCB->tx.txBusy = TRUE;
	}


// 阻塞方式时启动发送
#else
	UART_DRIVE_BC_StartTx(pCB);
#endif
}

// 向发送缓冲区中添加一条待发送序列
BOOL UART_DRIVE_AddTxArray(uint16 id, uint8 *pArray, uint16 length)
{
	uint16 i;
	uint16 head = uartDriveCB.tx.head;
	uint16 end = uartDriveCB.tx.end;
	
	// 参数检验
	if ((NULL == pArray) || (0 == length))
	{
		return FALSE;
	}

	// 发送缓冲区已满，不予接收
	if ((end + 1) % UART_DRIVE_TX_QUEUE_SIZE == head)
	{
		return FALSE;
	}

	uartDriveCB.tx.cmdQueue[end].deviceID = id;
	for (i = 0; i < length; i++)
	{
		uartDriveCB.tx.cmdQueue[end].buff[i] = *pArray++;
	}
	uartDriveCB.tx.cmdQueue[end].length = length;
	
	// 发送环形队列更新位置
	uartDriveCB.tx.end ++;
	uartDriveCB.tx.end %= UART_DRIVE_TX_QUEUE_SIZE;
	uartDriveCB.tx.cmdQueue[uartDriveCB.tx.end].length = 0;

	return TRUE;
}



#ifndef 	__UART_DRIVE_H__
#define 	__UART_DRIVE_H__

/*******************************************************************************
 *                                  头文件	                                   *
********************************************************************************/
#include "common.h"

/*******************************************************************************
 *                                  宏定义	                                   *
********************************************************************************/
#define UART_DRIVE_TX_MODE						UART_DRIVE_INTERRUPT_TX_MODE	// 选择阻塞发送还是中断发送数据
#define UART_DRIVE_TYPE_DEF						UART4							// 选择串口(串口中断和时钟都需要修改)
#define UART_DRIVE_IRQn_DEF						UART4_IRQn 				// 选择串口(串口中断和时钟都需要修改)

#define UART_DRIVE_BAUD_RATE					9600	// 通讯波特率

#define UART_DRIVE_TX_QUEUE_SIZE				3		// 驱动层发送命令帧尺寸
#define UART_DRIVE_CMD_FRAME_LENGTH_MAX			150		// 命令帧最大长度

// ■■■■■■■ 以下部分不能随意修改 ■■■■■■■■■■■■■■■
#define UART_DRIVE_BLOCKING_TX_MODE				0U
#define UART_DRIVE_INTERRUPT_TX_MODE			1U

// UART命令帧定义
typedef struct
{
	volatile uint16	deviceID;
	volatile uint8	buff[UART_DRIVE_CMD_FRAME_LENGTH_MAX];	// 命令帧缓冲区 
	volatile uint16	length;									// 命令帧有效数据个数
}UART_DRIVE_CMD_FRAME;

// UART控制结构体定义
typedef struct
{
	// 接收数据抛出接口
	void (*receiveDataThrowService)(uint16 id, uint8 *pData, uint16 length);
	
	// 发送帧缓冲区数据结构
	struct{
		UART_DRIVE_CMD_FRAME cmdQueue[UART_DRIVE_TX_QUEUE_SIZE];
		volatile uint16 head;						// 队列头索引
		volatile uint16 end; 						// 队列尾索引
		volatile uint16 index;						// 当前待发送数据在命令帧中的索引号
		volatile BOOL txBusy;						// 发送请求
	}tx;
}UART_DRIVE_CB;

extern UART_DRIVE_CB uartDriveCB;

/******************************************************************************
* 【外部接口声明】
******************************************************************************/
// UART初始化，在main的大循环之前调用，完成模块初始化
void UART_DRIVE_Init(uint32 baud);

// UART模块处理入口，在main的大循环中调用
void UART_DRIVE_Process(void);

// 注册数据抛出接口服务
void UART_DRIVE_RegisterDataSendService(void (*service)(uint16 id, uint8 *pData, uint16 length));

// 向发送缓冲区中添加一条待发送序列
BOOL UART_DRIVE_AddTxArray(uint16 id, uint8 *pArray, uint16 length);

#endif



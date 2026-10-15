/********************************************************************************************************
	八方V5.2通讯协议
********************************************************************************************************/
#ifndef 	__UART_PROTOCOL2_H__
#define 	__UART_PROTOCOL2_H__

/*******************************************************************************
 *                                  头文件	                                   *
********************************************************************************/
#include "common.h"

//==========与协议强相关，根据不同协议进行修改======================================
#define UART_PROTOCOL2_CMD_SEND_TIME				120		// 命令发送时间，八方协议间隔时间至少需要100ms以上

//=====================================================================================================
#define UART_PROTOCOL2_RX_TIME_OUT_CHECK_ENABLE		0		// 通讯单向接收超时检测功能开关:0禁止，1使能

#define UART_PROTOCOL2_RX_QUEUE_SIZE				3		// 接收命令队列尺寸
#define UART_PROTOCOL2_TX_QUEUE_SIZE				5		// 发送命令队列尺寸

#define UART_PROTOCOL2_CMD_WRITE					0x16	// 写操作
#define UART_PROTOCOL2_CMD_READ						0x11	// 读操作

#define UART_PROTOCOL2_TX_CMD_FRAME_LENGTH_MIN 		2		// 最小发送命令帧长度
#define UART_PROTOCOL2_TX_CMD_FRAME_LENGTH_MAX 		5		// 最大发送命令帧长度
#define UART_PROTOCOL2_RX_CMD_FRAME_LENGTH_MAX 		28		// 最大接收命令帧长度
#define UART_PROTOCOL2_RX_FIFO_SIZE					200		// 接收一级缓冲区大小

// 通讯超时时间，根据波特率和命令长度确定，同时留足余量，为2倍
#define UART_PROTOCOL2_BUS_UNIDIRECTIONAL_TIME_OUT	(uint32)((1000.0/UART_DRIVE_BAUD_RATE*10*UART_PROTOCOL2_TX_CMD_FRAME_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)		// 单位:ms
#define UART_PROTOCOL2_BUS_BIDIRECTIONAL_TIME_OUT	(uint32)(((1000.0/UART_DRIVE_BAUD_RATE*10*UART_PROTOCOL2_TX_CMD_FRAME_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)*2)	// 单位:ms

// UART发送协议定义	
typedef enum
{
	UART_PROTOCOL2_CMD_DATA1_INDEX = 0,					// 数据索引
	UART_PROTOCOL2_CMD_DATA2_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA3_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA4_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA5_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA6_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA7_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA8_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA9_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA10_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA11_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA12_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA13_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA14_INDEX,						// 数据索引
	UART_PROTOCOL2_CMD_DATA15_INDEX,						// 数据索引

	UART_PROTOCOL2_CMD_INDEX_MAX
}UART_PROTOCOL2_DATE_FRAME;

// 命令
typedef enum{
	UART_PROTOCOL2_CMD_NULL = 0,							// 空命令

	UART_PROTOCOL2_CMD_READ_PROTOCOL_VERSION = 0x1190,

	UART_PROTOCOL2_CMD_READ_DRIVER_STATUS = 0x1108,
	
	UART_PROTOCOL2_CMD_READ_WORK_STATUS = 0x1131,

	UART_PROTOCOL2_CMD_READ_INSTANTANEOUS_CURRENT = 0x110A,

	UART_PROTOCOL2_CMD_READ_BATTERY_CAPACITY = 0x1111,

	UART_PROTOCOL2_CMD_READ_RPM = 0x1120,

	UART_PROTOCOL2_CMD_READ_LIGHT_SWITCH = 0x111B,

	UART_PROTOCOL2_CMD_READ_BATTERY_INFO1 = 0x1160,

	UART_PROTOCOL2_CMD_READ_BATTERY_INFO2 = 0x1161,

	UART_PROTOCOL2_CMD_READ_REMAINDER_MILEAGE = 0x1122,

	UART_PROTOCOL2_CMD_READ_DRIVER_ERROR_CODE = 0x1123,

	UART_PROTOCOL2_CMD_READ_CADENCE = 0x1121,

	UART_PROTOCOL2_CMD_READ_TQKG = 0x1124,		// 实际命令为0x1122，但是有重复的了，直接改一个不存在的即可

	//============================================
	UART_PROTOCOL2_CMD_WRITE_LIMIT_SPEED = 0x161F,

	UART_PROTOCOL2_CMD_WRITE_ASSIST_LEVEL = 0x160B,

	UART_PROTOCOL2_CMD_WRITE_LIGHT_SWITCH = 0x161A,

	UART_PROTOCOL2_CMD_WRITE_CLEAR_DRIVER_ERROR_CODE = 0x1623,

	UART_PROTOCOL2_CMD_MAX								// 总命令数
}UART_PROTOCOL2_CMD;

// UART发送协议定义	
typedef enum
{
	UART_PROTOCOL2_TX_CMD_TYPE_ASSIST = 0,
	UART_PROTOCOL2_TX_CMD_TYPE_LIGHT,	
}UART_PROTOCOL2_TX_AT_ONCE_CMD_TYPE;

// UART_RX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[UART_PROTOCOL2_RX_CMD_FRAME_LENGTH_MAX];	// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}UART_PROTOCOL2_RX_CMD_FRAME;

// UART_TX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[UART_PROTOCOL2_TX_CMD_FRAME_LENGTH_MAX];	// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}UART_PROTOCOL2_TX_CMD_FRAME;

// UART控制结构体定义
typedef struct
{
	// 一级接收缓冲区
	struct
	{
		volatile uint8	buff[UART_PROTOCOL2_RX_FIFO_SIZE];
		volatile uint16	head;
		volatile uint16	end;
		BOOL			rxFIFOBusy;
	}rxFIFO;
	
	// 接收帧缓冲区数据结构
	struct{
		UART_PROTOCOL2_RX_CMD_FRAME	cmdQueue[UART_PROTOCOL2_RX_QUEUE_SIZE];
		uint16			head;						// 队列头索引
		uint16			end;						// 队列尾索引
		uint8			expectLength;
	}rx;

	// 发送帧缓冲区数据结构
	struct{
		UART_PROTOCOL2_TX_CMD_FRAME	cmdQueue[UART_PROTOCOL2_TX_QUEUE_SIZE];
		uint16	head;						// 队列头索引
		uint16	end;						// 队列尾索引
		uint16	index;						// 当前待发送数据在命令帧中的索引号
		BOOL	txBusy;						// 发送请求，为TRUE时，从待发送队列中取出一个BYTE放入发送寄存器
	}tx;

	// 发送数据接口
	BOOL (*sendDataThrowService)(uint16 id, uint8 *pData, uint16 length);

	BOOL txPeriodRequest;		// 码表周期性发送数据请求
	BOOL txAtOnceRequest;		// 码表马上发送数据请求
	UART_PROTOCOL2_TX_AT_ONCE_CMD_TYPE txAtOnceCmdType;

	UART_PROTOCOL2_CMD cmdType;
	uint8 step;
	
}UART_PROTOCOL2_CB;

extern UART_PROTOCOL2_CB uartProtocolCB2;		

/******************************************************************************
* 【外部接口声明】
******************************************************************************/

// 协议初始化
void UART_PROTOCOL2_Init(void);

// 协议层过程处理
void UART_PROTOCOL2_Process(void);

// 向发送命令帧队列中添加数据
void UART_PROTOCOL2_TxAddData(uint8 data);

// 启动发送，函数会自动校正发送命令帧的数据长度，并计算和添加校验码
void UART_PROTOCOL2_TxAddFrame(void);

//==================================================================================
// 设置下发请求标志，按键设置参数也是调用此接口
void UART_PROTOCOL2_SetTxAtOnceRequest(uint32 param);

void UART_PROTOCOL2_SetTxAtOnceCmdType(UART_PROTOCOL2_TX_AT_ONCE_CMD_TYPE type);

void UART_PROTOCOL2_CALLBACK_SetTxPeriodRequest(uint32 param);

#endif



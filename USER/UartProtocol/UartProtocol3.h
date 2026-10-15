/********************************************************************************************************
		BigStone_V9.2协议
********************************************************************************************************/
#ifndef 	__UART_PROTOCOL3_H__
#define 	__UART_PROTOCOL3_H__

#include "common.h"

//==========与协议强相关，根据不同协议进行修改======================================
// UART总线通讯故障时间
#define UART_PROTOCOL3_CMD_SEND_TIME				500		// 命令发送时间，单位:ms

//=====================================================================================================
#define UART_PROTOCOL3_RX_TIME_OUT_CHECK_ENABLE		1		// 通讯单向接收超时检测功能开关:0禁止，1使能

#define UART_PROTOCOL3_RX_QUEUE_SIZE				3		// 接收命令队列尺寸
#define UART_PROTOCOL3_TX_QUEUE_SIZE				3		// 发送命令队列尺寸

#define UART_PROTOCOL3_CMD_HEAD						'F'		// 命令头

#define UART_PROTOCOL3_RX_CMD_HMI_CTRL_RUN_LENGTH_MAX			8		// 该命令帧最大接收长度
#define UART_PROTOCOL3_RX_CMD_WRITE_CONTROL_PARAM_LENGTH_MAX	18		// 该命令帧最大接收长度
#define UART_PROTOCOL3_RX_CMD_READ_CONTROL_PARAM_LENGTH_MAX		4		// 该命令帧最大接收长度

#define UART_PROTOCOL3_TX_CMD_FRAME_LENGTH_MAX 		13		// 最大发送命令帧长度
#define UART_PROTOCOL3_RX_CMD_FRAME_LENGTH_MAX 		150		// 最大接收命令帧长度

#define UART_PROTOCOL3_RX_FIFO_SIZE					200		// 接收一级缓冲区大小
#define UART_PROTOCOL3_CMD_FRAME_LENGTH_MIN			4		// 命令帧最小长度，包含:起始符、异或调整值、错误信息、异或校验


// 通讯超时时间，根据波特率和命令长度确定，同时留足余量，为2倍
#define UART_PROTOCOL3_BUS_UNIDIRECTIONAL_TIME_OUT	(uint32)((1000.0/UART_DRIVE_BAUD_RATE*10*UART_PROTOCOL3_RX_CMD_WRITE_CONTROL_PARAM_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)		// 单位:ms
#define UART_PROTOCOL3_BUS_BIDIRECTIONAL_TIME_OUT	(uint32)(((1000.0/UART_DRIVE_BAUD_RATE*10*UART_PROTOCOL3_RX_CMD_WRITE_CONTROL_PARAM_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)*2)	// 单位:ms

// UART发送协议定义	
typedef enum
{
	UART_PROTOCOL3_CMD_START_SYMBOL_CMD_INDEX = 0,		// 起始符或命令字索引

	UART_PROTOCOL3_CMD_DATA1_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA2_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA3_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA4_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA5_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA6_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA7_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA8_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA9_INDEX,						// 数据索引
	UART_PROTOCOL3_CMD_DATA10_INDEX,					// 数据索引
	UART_PROTOCOL3_CMD_DATA11_INDEX,					// 数据索引
	UART_PROTOCOL3_CMD_DATA12_INDEX,					// 数据索引
	UART_PROTOCOL3_CMD_DATA13_INDEX,					// 数据索引
	UART_PROTOCOL3_CMD_DATA14_INDEX,					// 数据索引

	UART_PROTOCOL3_CMD_INDEX_MAX
}UART_PROTOCOL3_DATE_FRAME;

// 被控命令
typedef enum{
	UART_PROTOCOL3_CMD_NULL = 0,								// 空命令

	UART_PROTOCOL3_CMD_HMI_CTRL_PARAM_SET = 'S',				// 参数设置

	UART_PROTOCOL3_CMD_HMI_CTRL_RUN = 'F',						// 正常运行状态

	UART_PROTOCOL3_CMD_WRITE_CONTROL_PARAM = 0xC0,				// 写控制器参数
	UART_PROTOCOL3_CMD_WRITE_CONTROL_PARAM_RESULT = 0xC1,		// 写控制器参数结果应答
	UART_PROTOCOL3_CMD_READ_CONTROL_PARAM = 0xC2,				// 读取控制器参数
	UART_PROTOCOL3_CMD_READ_CONTROL_PARAM_REPORT = 0xC3,		// 控制器参数上报
	
	UART_PROTOCOL3_CMD_MAX										// 总命令数
	
}UART_PROTOCOL3_CMD;

// UART_RX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[UART_PROTOCOL3_RX_CMD_WRITE_CONTROL_PARAM_LENGTH_MAX];	// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}UART_PROTOCOL3_RX_CMD_FRAME;

// UART_TX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[UART_PROTOCOL3_TX_CMD_FRAME_LENGTH_MAX];	// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}UART_PROTOCOL3_TX_CMD_FRAME;

// UART控制结构体定义
typedef struct
{
	// 一级接收缓冲区
	struct
	{
		volatile uint8	buff[UART_PROTOCOL3_RX_FIFO_SIZE];
		volatile uint16	head;
		volatile uint16	end;
		uint16 currentProcessIndex;					// 当前待处理的字节的位置下标

		uint16 curCmdFrameLength;					// 当前命令帧长度
	}rxFIFO;
	
	// 接收帧缓冲区数据结构
	struct{
		UART_PROTOCOL3_RX_CMD_FRAME	cmdQueue[UART_PROTOCOL3_RX_QUEUE_SIZE];
		uint16			head;						// 队列头索引
		uint16			end;						// 队列尾索引
	}rx;

	// 发送帧缓冲区数据结构
	struct{
		UART_PROTOCOL3_TX_CMD_FRAME	cmdQueue[UART_PROTOCOL3_TX_QUEUE_SIZE];
		uint16	head;						// 队列头索引
		uint16	end;						// 队列尾索引
		uint16	index;						// 当前待发送数据在命令帧中的索引号
		BOOL	txBusy;						// 发送请求，为TRUE时，从待发送队列中取出一个BYTE放入发送寄存器
	}tx;

	// 发送数据接口
	BOOL (*sendDataThrowService)(uint16 id, uint8 *pData, uint16 length);

	BOOL txPeriodRequest;		// 码表周期性发送数据请求
	BOOL txAtOnceRequest;		// 码表马上发送数据请求

	BOOL paramSetOK;

}UART_PROTOCOL3_CB;

extern UART_PROTOCOL3_CB uartProtocolCB3;		

/******************************************************************************
* 【外部接口声明】
******************************************************************************/

// 协议初始化
void UART_PROTOCOL3_Init(void);

// 协议层过程处理
void UART_PROTOCOL3_Process(void);

// 向发送命令帧队列中添加数据
void UART_PROTOCOL3_TxAddData(uint8 data);

// 启动发送，函数会自动校正发送命令帧的数据长度，并计算和添加校验码
void UART_PROTOCOL3_TxAddFrame(void);

//==================================================================================
// 设置下发请求标志，按键设置参数也是调用此接口
void UART_PROTOCOL3_SetTxAtOnceRequest(uint32 param);


// ■■ 码表下发至驱动器的系统参数 ■■
typedef struct
{
	uint8 assitLevel : 4;					// 档位值，0-9
	uint8 isPushModeOn : 1;					// 6km/h推行功能，0:表示关闭；1:表示打开
	uint8 reserveByte1Bit65 : 2;
	uint8 isLightOn : 1;					// 灯光控制

	uint8 wheelSizeCode : 3;				// 轮径代码
	uint8 speedLimitCode : 5;				// 限速值代码

}UART_PROTOCOL3_SET_PARAM_CB;
#define UART_PROTOCOL3_SET_PARAM_SIZE	(sizeof(UART_PROTOCOL3_SET_PARAM_CB))

// ■■ 码表下发至驱动器的实时参数 ■■
typedef struct
{
	uint8 assitLevel : 4;					// 档位值，0-9
	uint8 isPushModeOn : 1;					// 6km/h推行功能，0:表示关闭；1:表示打开
	uint8 reserveByte1Bit65 : 2;
	uint8 isLightOn : 1;					// 灯光控制

	uint8 wheelSizeCode : 3;				// 轮径代码
	uint8 speedLimitCode : 5;				// 限速值代码
}UART_PROTOCOL3_RUNNING_PARAM_CB;
#define UART_PROTOCOL3_RUNNING_PARAM_SIZE	(sizeof(UART_PROTOCOL3_RUNNING_PARAM_CB))

// ■■ 驱动器上报至码表的参数 ■■
typedef struct
{
	uint8 batteryVoltageLevel;				// 电压电量

	uint8 batteryCurrent;					// 当前电池电流，单位:1/3A

	uint8 oneCycleTimeH;					// 当前一圈时间，高字节
	uint8 oneCycleTimeL;					// 当前一圈时间，低字节

	uint8 xorAdjVal;						// 异或调整值
	
	uint8 ucErrorCode;						// 错误代码
	
}UART_PROTOCOL3_RX_PARAM_CB;
#define UART_PROTOCOL3_RX_PARAM_SIZE			(sizeof(UART_PROTOCOL3_RX_PARAM_CB))


// 码表与控制器数据交互结构体定义
typedef struct
{
	// 设置驱动器参数
	union
	{
		uint8 buff[UART_PROTOCOL3_SET_PARAM_SIZE];
		UART_PROTOCOL3_SET_PARAM_CB param;
	}set;

	// 驱动器运行参数
	union
	{
		uint8 buff[UART_PROTOCOL3_RUNNING_PARAM_SIZE];
		UART_PROTOCOL3_RUNNING_PARAM_CB param;
	}running;

	// 驱动器上报至码表的参数
	union
	{
		uint8 buff[UART_PROTOCOL3_RX_PARAM_SIZE];
		UART_PROTOCOL3_RX_PARAM_CB param;
	}rx;	
}UART_PROTOCOL3_PARAM_CB;

extern UART_PROTOCOL3_PARAM_CB hmiDriveCB3;


#endif



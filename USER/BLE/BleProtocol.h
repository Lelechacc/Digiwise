#ifndef __BLE_PROTOCOL_H__
#define __BLE_PROTOCOL_H__

#include "common.h"

//=====================================================================================================
#define BLE_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE		0		// 通讯单向接收超时检测功能开关:0禁止，1使能
#define BLE_PROTOCOL_TXRX_TIME_OUT_CHECK_ENABLE		0		// 通讯双向接收超时检测功能开关:0禁止，1使能

#define BLE_PROTOCOL_RX_QUEUE_SIZE					5		// 接收命令队列尺寸
#define BLE_PROTOCOL_TX_QUEUE_SIZE					5		// 发送命令队列尺寸

#define BLE_PROTOCOL_CMD_HEAD						0x55	// 命令头

#define BLE_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX 		150		// 最大发送命令帧长度
#define BLE_PROTOCOL_RX_CMD_FRAME_LENGTH_MAX 		150		// 最大接收命令帧长度
#define BLE_PROTOCOL_RX_FIFO_SIZE					200		// 接收一级缓冲区大小
#define BLE_PROTOCOL_CMD_FRAME_LENGTH_MIN			4			// 命令帧最小长度，包含:命令头、命令字、数据长度、校验和

// 通讯超时时间，根据波特率和命令长度确定，同时留足余量，为2倍
#define BLE_PROTOCOL_BUS_UNIDIRECTIONAL_TIME_OUT	(uint32)((1000.0/BLE_UART_BAUD_RATE*10*BLE_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)		// 单位:ms
#define BLE_PROTOCOL_BUS_BIDIRECTIONAL_TIME_OUT		(uint32)(((1000.0/BLE_UART_BAUD_RATE*10*BLE_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)*2)	// 单位:ms

// UART发送协议定义	
typedef enum
{
	BLE_PROTOCOL_CMD_HEAD_INDEX = 0,					// 帧头索引
	BLE_PROTOCOL_CMD_CMD_INDEX, 						// 命令字索引
	BLE_PROTOCOL_CMD_LENGTH_INDEX,						// 数据长度索引

	BLE_PROTOCOL_CMD_DATA1_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA2_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA3_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA4_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA5_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA6_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA7_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA8_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA9_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA10_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA11_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA12_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA13_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA14_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA15_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA16_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA17_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA18_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA19_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA20_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA21_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA22_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA23_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA24_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA25_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA26_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA27_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA28_INDEX,						// 数据索引
	BLE_PROTOCOL_CMD_DATA29_INDEX, 						// 数据索引
	BLE_PROTOCOL_CMD_DATA30_INDEX, 						// 数据索引

	BLE_PROTOCOL_CMD_INDEX_MAX
}BLE_PROTOCOL_DATE_FRAME;

// 下行命令
typedef enum{
	// 系统命令
	BLE_CMD_HEARTBEAT_DOWN = 0x00,						// 心跳命令，下行
	BLE_CMD_HEARTBEAT_UP = 0x01,						// 心跳命令应答，上行
	BLE_CMD_AUTHENTICATE_DOWN = 0x02,					// 鉴权命令，下行
	BLE_CMD_AUTHENTICATE_UP = 0x03,						// 鉴权命令应答，上行
	BLE_CMD_FIRMVERSION_DOWN = 0x04,					// 询问固件版本命令，下行
	BLE_CMD_FIRMVERSION_UP = 0x05,						// 询问固件版本命令应答，上行
	BLE_CMD_SETTIMER_DOWN = 0x06,						// 蓝牙授时命令，下行
	BLE_CMD_SETTIMER_UP = 0x07,							// 蓝牙授时命令应答，上行
	BLE_CMD_ERROR_UP = 0x08,							// 错误代码上报，上行
	BLE_CMD_ERROR_DOWN = 0x09,							// 错误代码应答接收，下行
	
	// IAP命令
	BLE_CMD_RESET_DOWN = 0x20,							// 复位命令，下行
	BLE_CMD_RESET_UP = 0x21,							// 复位命令应答，上行
	BLE_CMD_FIRMUPDATE_DOWN = 0x22,						// 固件升级命令，下行
	BLE_CMD_FIRMUPDATE_UP = 0x23,						// 固件升级命令应答，上行
	BLE_CMD_DATAIN_DOWN = 0x24,							// 数据包写入命令，下行
	BLE_CMD_DATAIN_UP = 0x25,							// 数据包写入命令应答，上行
	BLE_CMD_DATAIN_FINISH_DOWN = 0x26,					// 数据包写入完成命令，下行
	BLE_CMD_DATAIN_FINISH_UP = 0x27,					// 数据包写入完成命令应答，上行

	// 控制命令
	BLE_CMD_GETSTATUS_DOWN = 0x30,						// 获取仪表状态命令，下行
	BLE_CMD_GETSTATUS_UP = 0x31,						// 仪表状态参数回应，上行
	BLE_CMD_CONTROL_DOWN = 0x32,						// 仪表控制命令，下行
	BLE_CMD_CONTROL_UP = 0x33,							// 仪表控制命令应答，上行
	BLE_CMD_CHECK_SELF_DOWN = 0x34,						// 仪表开始自检，下行
	BLE_CMD_CHECK_SELF_UP = 0x35,						// 仪表自检结果上报，上行
	BLE_CMD_FINISH_UP = 0x36,							// 仪表结束上报，上行
	BLE_CMD_SELECT_GETSTATUS_DOWN = 0x37,				// 选择性获取参数上报，下行
	BLE_CMD_SELECT_GETSTATUS_UP = 0x38,					// 选择性获取参数上报，上行

	// 数据采集命令
	BLE_CMD_GETDATA_RUNNING_DOWN = 0x60,				// 获取实时运动数据，下行
	BLE_CMD_GETDATA_RUNNING_UP = 0x61,					// 获取实时运动数据应答，上行
	BLE_CMD_START_HISTORICAL_DATA_UP = 0x62,			// 启动同步历史运动数据，上行
	BLE_CMD_START_HISTORICAL_DATA_DOWN = 0x63,			// 启动同步历史运动数据应答命令，下行
	BLE_CMD_REPORT_HISTORICAL_DATA_UP = 0x64,			// 上报历史运动数据，上行
	BLE_CMD_REPORT_HISTORICAL_DATA_DOWN = 0x65,			// 上报历史运动数据命令，下行
	
	BLE_CMD_MAX,

}BLE_PROTOCOL_CMD;

// 控制命令控制项
typedef enum{
	CONTROL_CMD_NULL = 0,
	LIGHT,												// 控制大灯
	AUTO_SHUTDOWN_TIME,									// 设置自动关机时间
	SPEED_LIMIT,										// 限速值
	UNIT,												// 设置显示单位
	ASSIST,												// 设置助力档位
	LOCK,												// 设置锁车状态
	BACKLIGHT,											// 设置背光亮度
	CONTROL_CMD_MAX,
}BLE_PROTOCOL_CONTROL_CMD;


// UART_RX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[BLE_PROTOCOL_RX_CMD_FRAME_LENGTH_MAX];		// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}BLE_PROTOCOL_RX_CMD_FRAME;

// UART_TX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[BLE_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX];		// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}BLE_PROTOCOL_TX_CMD_FRAME;

// UART控制结构体定义
typedef struct
{
	// 一级接收缓冲区
	struct
	{
		volatile uint8	buff[BLE_PROTOCOL_RX_FIFO_SIZE];
		volatile uint16	head;
		volatile uint16	end;
		uint16 currentProcessIndex;											// 当前待处理的字节的位置下标
	}rxFIFO;
	
	// 接收帧缓冲区数据结构
	struct{
		BLE_PROTOCOL_RX_CMD_FRAME	cmdQueue[BLE_PROTOCOL_RX_QUEUE_SIZE];
		uint16			head;												// 队列头索引
		uint16			end;												// 队列尾索引
	}rx;

	// 发送帧缓冲区数据结构
	struct{
		BLE_PROTOCOL_TX_CMD_FRAME	cmdQueue[BLE_PROTOCOL_TX_QUEUE_SIZE];
		uint16	head;														// 队列头索引
		uint16	end;														// 队列尾索引
		uint16	index;														// 当前待发送数据在命令帧中的索引号
		BOOL	txBusy;														// 发送请求，为TRUE时，从待发送队列中取出一个BYTE放入发送寄存器
	}tx;

	// 发送数据接口
	BOOL (*sendDataThrowService)(uint16 id, uint8 *pData, uint16 length);

	uint32 ageFlag;
	BOOL isTimeCheck;

}BLE_PROTOCOL_CB;

extern BLE_PROTOCOL_CB bleProtocolCB;		

/******************************************************************************
* 【外部接口声明】
******************************************************************************/

// 协议初始化
void BLE_PROTOCOL_Init(void);

// 协议层过程处理
void BLE_PROTOCOL_Process(void);

// 向发送命令帧队列中添加数据
void BLE_PROTOCOL_TxAddData(uint8 data);

// 启动发送，函数会自动校正发送命令帧的数据长度，并计算和添加校验码
void BLE_PROTOCOL_TxAddFrame(void);

// 仪表错误信息上报
void BLE_PROTOCOL_TxErrorInfo(uint32 param);

// 仪表启动同步历史数据上报
void BLE_PROTOCOL_TxStartHistoryData(uint32 param);

// 仪表控制命令回复
void BLE_PROTOCOL_SendCmdControlAck(uint8 controlCmd, uint8 paramH, uint8 paramL);

// 发送获取MAC地址
void BLE_PROTOCOL_SendGetMacAddr(uint32 param);

// 仪表档位消息上报
void BLE_PROTOCOL_SendAssistInfor(uint8 param);

// 仪表状态信息回复
void BLE_PROTOCOL_SendCmdStatusAck(void);

// 发送广播数据包
void BLE_PROTOCOL_SendAdv(uint32 param);
#endif


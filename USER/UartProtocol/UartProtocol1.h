/********************************************************************************************************
	锂电2号通讯协议(常州新思维协议)
********************************************************************************************************/
#ifndef 	__UART_PROTOCOL1_H__
#define 	__UART_PROTOCOL1_H__

#include "common.h"


//==========与协议强相关，根据不同协议进行修改======================================
#define UART_PROTOCOL1_CMD_SEND_TIME				100		// 命令发送时间

//=====================================================================================================
#define UART_PROTOCOL1_RX_TIME_OUT_CHECK_ENABLE		0		// 通讯单向接收超时检测功能开关:0禁止，1使能

#define UART_PROTOCOL1_RX_QUEUE_SIZE				3		// 接收命令队列尺寸
#define UART_PROTOCOL1_TX_QUEUE_SIZE				3		// 发送命令队列尺寸

#define UART_PROTOCOL1_CMD_RX_DEVICE_ADDR			0x02	// 设备地址头
#define UART_PROTOCOL1_CMD_TX_DEVICE_ADDR			0x01	// 设备地址头

#define UART_PROTOCOL1_TX_CMD_FRAME_LENGTH_MAX 		150		// 最大发送命令帧长度
#define UART_PROTOCOL1_RX_CMD_FRAME_LENGTH_MAX 		150		// 最大接收命令帧长度
#define UART_PROTOCOL1_RX_FIFO_SIZE					200		// 接收一级缓冲区大小
#define UART_PROTOCOL1_CMD_FRAME_LENGTH_MIN			4		// 命令帧最小长度，包含:设备地址、数据长度、命令字、校验和


// 通讯超时时间，根据波特率和命令长度确定，同时留足余量，为2倍
#define UART_PROTOCOL1_BUS_UNIDIRECTIONAL_TIME_OUT	(uint32)((1000.0/UART_DRIVE_BAUD_RATE*10*UART_PROTOCOL1_TX_CMD_FRAME_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)		// 单位:ms
#define UART_PROTOCOL1_BUS_BIDIRECTIONAL_TIME_OUT	(uint32)(((1000.0/UART_DRIVE_BAUD_RATE*10*UART_PROTOCOL1_TX_CMD_FRAME_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)*2)	// 单位:ms

// UART发送协议定义	
typedef enum
{
	UART_PROTOCOL1_CMD_DEVICE_ADDR_INDEX = 0,			// 设备地址
	UART_PROTOCOL1_CMD_LENGTH_INDEX,					// 数据长度
	UART_PROTOCOL1_CMD_CMD_INDEX, 						// 命令字索引

	UART_PROTOCOL1_CMD_DATA1_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA2_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA3_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA4_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA5_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA6_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA7_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA8_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA9_INDEX,						// 数据索引
	UART_PROTOCOL1_CMD_DATA10_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA11_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA12_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA13_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA14_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA15_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA16_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA17_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA18_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA19_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA20_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA21_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA22_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA23_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA24_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA25_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA26_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA27_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA28_INDEX,					// 数据索引
	UART_PROTOCOL1_CMD_DATA29_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA30_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA31_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA32_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA33_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA34_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA35_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA36_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA37_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA38_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA39_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA40_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA41_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA42_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA43_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA44_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA45_INDEX, 					// 数据索引
	UART_PROTOCOL1_CMD_DATA46_INDEX, 					// 数据索引

	UART_PROTOCOL1_CMD_INDEX_MAX
}UART_PROTOCOL1_DATE_FRAME;

// 被控命令
typedef enum{
	UART_PROTOCOL1_CMD_NULL = 0,							// 空命令

	UART_PROTOCOL1_CMD_HMI_CTRL_RUN = 0x01,					// 正常运行状态

	UART_PROTOCOL1_CMD_MAX									// 总命令数
	
}UART_PROTOCOL1_CMD;

// UART_RX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[UART_PROTOCOL1_RX_CMD_FRAME_LENGTH_MAX];	// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}UART_PROTOCOL1_RX_CMD_FRAME;


// UART_TX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[UART_PROTOCOL1_TX_CMD_FRAME_LENGTH_MAX];	// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}UART_PROTOCOL1_TX_CMD_FRAME;

// UART控制结构体定义
typedef struct
{
	// 一级接收缓冲区
	struct
	{
		volatile uint8	buff[UART_PROTOCOL1_RX_FIFO_SIZE];
		volatile uint16	head;
		volatile uint16	end;
		uint16 currentProcessIndex;					// 当前待处理的字节的位置下标
	}rxFIFO;
	
	// 接收帧缓冲区数据结构
	struct{
		UART_PROTOCOL1_RX_CMD_FRAME	cmdQueue[UART_PROTOCOL1_RX_QUEUE_SIZE];
		uint16			head;						// 队列头索引
		uint16			end;						// 队列尾索引
	}rx;

	// 发送帧缓冲区数据结构
	struct{
		UART_PROTOCOL1_TX_CMD_FRAME	cmdQueue[UART_PROTOCOL1_TX_QUEUE_SIZE];
		uint16	head;						// 队列头索引
		uint16	end;						// 队列尾索引
		uint16	index;						// 当前待发送数据在命令帧中的索引号
		BOOL	txBusy;						// 发送请求，为TRUE时，从待发送队列中取出一个BYTE放入发送寄存器
	}tx;

	// 发送数据接口
	BOOL (*sendDataThrowService)(uint16 id, uint8 *pData, uint16 length);

	BOOL txPeriodRequest;		// 码表周期性发送数据请求
	BOOL txAtOnceRequest;		// 码表马上发送数据请求
	

}UART_PROTOCOL1_CB;

extern UART_PROTOCOL1_CB uartProtocolCB1;		

/******************************************************************************
* 【外部接口声明】
******************************************************************************/

// 协议初始化
void UART_PROTOCOL1_Init(void);

// 协议层过程处理
void UART_PROTOCOL1_Process(void);

// 向发送命令帧队列中添加数据
void UART_PROTOCOL1_TxAddData(uint8 data);

// 启动发送，函数会自动校正发送命令帧的数据长度，并计算和添加校验码
void UART_PROTOCOL1_TxAddFrame(void);

//==================================================================================
// 设置下发请求标志，按键设置参数也是调用此接口
void UART_PROTOCOL1_SetTxAtOnceRequest(uint32 param);

// 发送老化累加次数
void UART_PROTOCOL1_SendCmdAging(uint32 param);

void UART_PROTOCOL1_CALLBACK_SetTxPeriodRequest(uint32 param);



// ■■ 码表下发至驱动器的控制器控制设定1 ■■
// bit0 bit1 ...bit7
typedef struct
{
	uint8 switchCruiseMode: 1; 			// 巡航切换模式
	uint8 isPushModeOn: 1;				// 6km/h推行功能，0:表示关闭；1:表示打开
	uint8 speedLimitState: 1;			// 限速状态
	uint8 switchCruiseWay: 1; 			// 切换巡航的方式
	uint8 comunicationError: 1;			// 通讯故障
	uint8 isLightOn : 1;				// 灯光控制
	uint8 zeroStartOrNot : 1;			// 零启动，非零启动
	uint8 driverState : 1;				// 控制器开关状态

}UART_PROTOCOL1_DRIVER_SET1_CB;
#define UART_PROTOCOL1_DRIVER_SET1_SIZE	(sizeof(UART_PROTOCOL1_DRIVER_SET1_CB))

// ■■ 码表下发至驱动器的控制器控制设定2 ■■
// bit0 bit1 ...bit7
typedef struct
{
	uint8 assistSteelType : 4;			// 助力磁钢盘类型
	uint8 reserveBit4 : 1;
	uint8 reserveBit5 : 1;
	uint8 autoCruise : 1;				// 自动巡航标志
	uint8 astern : 1;					// 倒车标志

}UART_PROTOCOL1_DRIVER_SET2_CB;
#define UART_PROTOCOL1_DRIVER_SET2_SIZE	(sizeof(UART_PROTOCOL1_DRIVER_SET2_CB))

// ■■ 驱动器上报至码表的参数 ■■
typedef struct
{
	// 控制器状态1
	uint8 motorPhaseError : 1;			// 电机缺相，1: 电机缺相故障 0: 电机不故障
	uint8 breakError : 1;				// 刹把故障，1: 刹把有故障 0：刹把无故障
	uint8 cruiseState : 1;				// 巡航状态，1: 正在巡航 0: 不在巡航
	uint8 underVoltageProtectionError : 1;// 欠压保护状态，1: 正在欠压保护 0: 不在欠压保护
	uint8 driverError : 1;				// 控制器故障状态，1: 控制器故障 0: 控制器不故障
	uint8 turnBarError : 1;				// 转把故障状态，1: 转把故障 0: 转把不故障
	uint8 holzerError : 1;				// 霍尔故障状态，1: 霍尔故障 0: 霍尔不故障
	uint8 cruise6kmState : 1;			// 6km巡航状态，1: 正在6Km 巡航 0: 不在6Km 巡航

	// 控制器状态2
	uint8 reserveBit10 : 2;				// 蓝牙控制档位(预留)
	uint8 DriverSpeedLimitState : 1;	// 控制器限速状态，0不限速，1限速中
	uint8 chargeState : 1;				// 充电状态，0不充电，1充电中
	uint8 communicationError : 1;		// 通讯故障，0正常，1故障RX
	uint8 breakState : 1;				// 断电刹把(状态),0没有启动，1启动
	uint8 assistSensorState : 1;		// 助力传感器状态，0正常，1故障
	uint8 bicycleHorizontalState : 1;	// 车是否处于水平状态，0水平，1非水平

	// 当高字节的第6 位为1 时，表示单位为0.1A， 
	// 如果高字节的第6 位为0，电流的单位依然是1A
	uint8 ucCurBatteryCurrentH;			// 当前电池电流，高字节	
	uint8 ucCurBatteryCurrentL;			// 当前电池电流，低字节

	uint8 currentRatio;					// 电流比例值

	uint8 oneCycleTimeH;				// 当前一圈时间，高字节
	uint8 oneCycleTimeL;				// 当前一圈时间，低字节	
	
	uint8 batteryCapacity;				// 电池容量

	uint8 remainderMileageH;			// 剩余里程，高字节
	uint8 remainderMileageL;			// 剩余里程，低字节
	
}UART_PROTOCOL1_RX_PARAM_CB;
#define UART_PROTOCOL1_RX_PARAM_SIZE			(sizeof(UART_PROTOCOL1_RX_PARAM_CB))


// 码表与控制器数据交互结构体定义
typedef struct
{
	// 设置驱动器参数
	union
	{
		uint8 val;
		UART_PROTOCOL1_DRIVER_SET1_CB param;
	}DriverSet1;

	// 设置驱动器参数
	union
	{
		uint8 val;
		UART_PROTOCOL1_DRIVER_SET2_CB param;
	}DriverSet2;

	// 驱动器上报至码表的参数
	union
	{
		uint8 buff[UART_PROTOCOL1_RX_PARAM_SIZE];
		UART_PROTOCOL1_RX_PARAM_CB param;
	}rx;	
}UART_PROTOCOL1_PARAM_CB;

extern UART_PROTOCOL1_PARAM_CB hmiDriveCB1;


#endif



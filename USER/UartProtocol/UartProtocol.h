/********************************************************************************************************
		KM5S与控制器通讯协议
********************************************************************************************************/
#ifndef 	__UART_PROTOCOL_H__
#define 	__UART_PROTOCOL_H__

#include "common.h"

//==========与协议强相关，根据不同协议进行修改======================================
#define UART_PROTOCOL_CMD_SEND_TIME					500		// 命令发送周期

//=====================================================================================================
#define UART_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE		0		// 通讯单向接收超时检测功能开关:0禁止，1使能

#define UART_PROTOCOL_RX_QUEUE_SIZE					3		// 接收命令队列尺寸
#define UART_PROTOCOL_TX_QUEUE_SIZE					3		// 发送命令队列尺寸

#define UART_PROTOCOL_CMD_HEAD						0x3A	// 命令头
#define UART_PROTOCOL_CMD_DEVICE_ADDR				0x1A	// 设备地址

#define UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX 		80		// 最大发送命令帧长度
#define UART_PROTOCOL_RX_CMD_FRAME_LENGTH_MAX 		80		// 最大接收命令帧长度
#define UART_PROTOCOL_RX_FIFO_SIZE					200		// 接收一级缓冲区大小
#define UART_PROTOCOL_CMD_FRAME_LENGTH_MIN			8		// 命令帧最小长度，包含:命令头、设备地址、命令字、数据长度、校验和L、校验和H、结束标识0xD、结束标识OxA


// 通讯超时时间，根据波特率和命令长度确定，同时留足余量，为2倍
#define UART_PROTOCOL_BUS_UNIDIRECTIONAL_TIME_OUT	(uint32)((1000.0/UART_DRIVE_BAUD_RATE*10*UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)		// 单位:ms
#define UART_PROTOCOL_BUS_BIDIRECTIONAL_TIME_OUT	(uint32)(((1000.0/UART_DRIVE_BAUD_RATE*10*UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX*2.0+0.9)+2*TIMER_TIME)*2)	// 单位:ms

// UART发送协议定义	
typedef enum
{
	UART_PROTOCOL_CMD_HEAD_INDEX = 0,					// 帧头索引
	UART_PROTOCOL_CMD_DEVICE_ADDR_INDEX,				// 设备地址
	UART_PROTOCOL_CMD_CMD_INDEX, 						// 命令字索引
	UART_PROTOCOL_CMD_LENGTH_INDEX,						// 数据长度

	UART_PROTOCOL_CMD_DATA1_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA2_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA3_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA4_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA5_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA6_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA7_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA8_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA9_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA10_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA11_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA12_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA13_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA14_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA15_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA16_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA17_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA18_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA19_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA20_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA21_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA22_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA23_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA24_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA25_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA26_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA27_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA28_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA29_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA30_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA31_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA32_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA33_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA34_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA35_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA36_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA37_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA38_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA39_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA40_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA41_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA42_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA43_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA44_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA45_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA46_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA47_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA48_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA49_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA50_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA51_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA52_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA53_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA54_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA55_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA56_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA57_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA58_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA59_INDEX,						// 数据索引
	UART_PROTOCOL_CMD_DATA60_INDEX,						// 数据索引

	UART_PROTOCOL_CMD_INDEX_MAX
}UART_PROTOCOL_DATE_FRAME;

// 被控命令
typedef enum{
	UART_PROTOCOL_CMD_NULL = 0,								// 空命令
	
	UART_ECO_CMD_ECO_MCU_UID = 0X08,						// 比较MCU_UID
	
	UART_PROTOCOL_CMD_HMI_CTRL_RUN = 0x52,					// 正常运行状态

	UART_PROTOCOL_CMD_HMI_CTRL_PARAM_SET = 0x53,			// 参数设置

	UART_PROTOCOL_CMD_WRITE_CONTROL_PARAM = 0xC0,			// 写控制器参数
	UART_PROTOCOL_CMD_WRITE_CONTROL_PARAM_RESULT = 0xC1,	// 写控制器参数结果应答
	UART_PROTOCOL_CMD_READ_CONTROL_PARAM = 0xC2,			// 读取控制器参数
	UART_PROTOCOL_CMD_READ_CONTROL_PARAM_REPORT = 0xC3,		// 控制器参数上报
	
	// BIST 指令
	UART_PROTOCOL_CMD_TEST_LED = 0xB1,						// 数码管显示指令
	UART_PROTOCOL_CMD_TEST_BUZZER = 0xB2,					// 蜂鸣器开关指令
	UART_PROTOCOL_CMD_TEST_KEY = 0xB3,						// 按键测试指令
	UART_PROTOCOL_CMD_TEST_LCD = 0xB4,						// LCD测试指令
	
	// 特权指令
	UART_PROTOCOL_CMD_READ_FLAG_DATA = 0xA6,				// 读标志区数据
	UART_PROTOCOL_CMD_WRITE_FLAG_DATA = 0xA7,				// 写标志区数据
	
	UART_PROTOCOL_CMD_WRITE_VERSION_TYPE = 0xA8,			// 写入版本信息
	UART_PROTOCOL_CMD_READ_VERSION_TYPE = 0xA9,				// 读取版本信息
	
	UART_PROTOCOL_CMD_PROTOCOL_SWITCH = 0xAB,				// 协议切换命令
	UART_PROTOCOL_CMD_KEY_CHECK_TEST = 0xAD,				// 按键检测测试
	UART_PROTOCOL_CMD_BLUE_MACADDR_TEST = 0xB0,				// 蓝牙MAC地址读取
	UART_PROTOCOL_CMD_CLEAR_MILEAGE = 0xB5,					// 清除里程数据
	
	UART_PROTOCOL_CMD_MAX									// 总命令数
}UART_PROTOCOL_CMD;

// UART_RX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[UART_PROTOCOL_RX_CMD_FRAME_LENGTH_MAX];	// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}UART_PROTOCOL_RX_CMD_FRAME;

// UART_TX命令帧定义
typedef struct
{
	uint16  deviceID;
	uint8	buff[UART_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX];	// 命令帧缓冲区
	uint16	length; 										// 命令帧有效数据个数
}UART_PROTOCOL_TX_CMD_FRAME;

// UART控制结构体定义
typedef struct
{
	// 一级接收缓冲区
	struct
	{
		volatile uint8	buff[UART_PROTOCOL_RX_FIFO_SIZE];
		volatile uint16	head;
		volatile uint16	end;
		uint16 currentProcessIndex;					// 当前待处理的字节的位置下标
	}rxFIFO;
	
	// 接收帧缓冲区数据结构
	struct{
		UART_PROTOCOL_RX_CMD_FRAME	cmdQueue[UART_PROTOCOL_RX_QUEUE_SIZE];
		uint16			head;						// 队列头索引
		uint16			end;						// 队列尾索引
	}rx;

	// 发送帧缓冲区数据结构
	struct{
		UART_PROTOCOL_TX_CMD_FRAME	cmdQueue[UART_PROTOCOL_TX_QUEUE_SIZE];
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
	BOOL readMcuUid;
	

}UART_PROTOCOL_CB;

extern UART_PROTOCOL_CB uartProtocolCB;		

/******************************************************************************
* 【外部接口声明】
******************************************************************************/

// 协议初始化
void UART_PROTOCOL_Init(void);

// 协议层过程处理
void UART_PROTOCOL_Process(void);

// 向发送命令帧队列中添加数据
void UART_PROTOCOL_TxAddData(uint8 data);

// 启动发送，函数会自动校正发送命令帧的数据长度，并计算和添加校验码
void UART_PROTOCOL_TxAddFrame(void);

// 发送老化累加次数
void UART_PROTOCOL_SendCmdAging(uint32 param);

void UART_PROTOCOL_SendMcuUid(uint32 param);

// UART报文接收处理函数(注意根据具体模块修改)
void UART_PROTOCOL_MacProcess(uint16 standarID, uint8* pData, uint16 length);

//==================================================================================
// 设置下发请求标志，按键设置参数也是调用此接口
void UART_PROTOCOL_SetTxAtOnceRequest(uint32 param);


// ■■ 码表下发至驱动器的系统参数 ■■
typedef struct
{
	uint8 assistStartOfSteelNum : 6;	// 助力开始的磁钢数
	uint8 assistDirection : 1;			// 助力正反
	uint8 reserve : 1;

	uint8 ucAssistPercent;				// 助力比例

	uint8 steelNumOfSpeedSensor : 4;	// 速度传感器一圈磁钢数
	uint8 ucSlowStart : 2;				// 缓启动参数
	uint8 ucTurnBarSpeed6kmphLimit : 1;	// 转把限速6km/h
	uint8 ucTurnBarLevel : 1;			// 转把分档
		
	uint8 lowVoltageThresholdOffset;	// 欠压门限偏差值

	uint8 currentThreshold : 6;			// 电流门限
	uint8 lowVoltageThreshold : 2;		// 欠压门限

	uint8 shakeHandFlag;				// 握手字节

	uint8 wheelSize : 3;				// 轮径
	uint8 speedLimit : 5;				// 限速门限
}UART_PROTOCOL_SET_PARAM_CB;
#define UART_PROTOCOL_SET_PARAM_SIZE	(sizeof(UART_PROTOCOL_SET_PARAM_CB))

// ■■ 码表下发至驱动器的实时参数 ■■
typedef struct
{
	uint8 pwm;							// Pwm门限	
	
	uint8 isOverSpeedAlarm : 1; 		// 超速告警，1:表示超速 
	uint8 reserve1 : 1;
	uint8 isCruiseOn : 1;				// 巡航开关，0:表示关闭；1:表示打开 
	uint8 reserve2 : 1;
	uint8 isPushModeOn : 1; 			// 6km/h推行功能，0:表示关闭；1:表示打开
	uint8 isLowVoltageAlarm : 1;		// 电池容量低告警
	uint8 reserve3 : 1;
	uint8 isLightOn : 1;				// 大灯开关
}UART_PROTOCOL_RUNNING_PARAM_CB;
#define UART_PROTOCOL_RUNNING_PARAM_SIZE	(sizeof(UART_PROTOCOL_RUNNING_PARAM_CB))

// ■■ 驱动器上报至码表的参数 ■■
typedef struct
{
	uint8 ucCurBatteryVoltage :6;		// 当前电池电压，单位:V
	uint8 ucCurCruiseState : 1;			// 当前巡航状态，1:表示进入巡航状态	
	uint8 ucUnderVoltageAlarm : 1;		// 欠压告警，0:表示不欠压；1:表示欠压
	
	uint8 ucCurBatteryCurrent;			// 当前电池电流，单位:A	
	
	uint8 oneCycleTimeH;				// 当前一圈时间，高字节
	uint8 oneCycleTimeL;				// 当前一圈时间，低字节

	uint8 ucErrorCode;					// 错误代码
	
}UART_PROTOCOL_RX_PARAM_CB;
#define UART_PROTOCOL_RX_PARAM_SIZE			(sizeof(UART_PROTOCOL_RX_PARAM_CB))


// 码表与控制器数据交互结构体定义
typedef struct
{
	// 设置驱动器参数
	union
	{
		uint8 buff[UART_PROTOCOL_SET_PARAM_SIZE];
		UART_PROTOCOL_SET_PARAM_CB param;
	}set;

	// 驱动器运行参数
	union
	{
		uint8 buff[UART_PROTOCOL_RUNNING_PARAM_SIZE];
		UART_PROTOCOL_RUNNING_PARAM_CB param;
	}running;

	// 驱动器上报至码表的参数
	union
	{
		uint8 buff[UART_PROTOCOL_RX_PARAM_SIZE];
		UART_PROTOCOL_RX_PARAM_CB param;
	}rx;	
}UART_PROTOCOL_PARAM_CB;

extern UART_PROTOCOL_PARAM_CB hmiDriveCB;


#endif





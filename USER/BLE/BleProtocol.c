#include "common.h"
#include "delay.h"
#include "BleUart.h"
#include "BleProtocol.h"
#include "timer.h"
#include "param.h"
#include "Ds1302.h"
#include "State.h"
#include "Record.h"
#include "system.h"
#include "iap.h"
#include "backlight.h"

// 蓝牙权鉴表
const uint8 Authentication_Table[5][256] = {
 {0x09, 0x4c, 0xd7, 0x6e, 0x2e, 0x6d, 0x28, 0x5f, 0x03, 0xc6, 0xd1, 0x59, 0xab, 0xe3, 0xd5, 0x97, 
  0x99, 0x37, 0x3d, 0x3d, 0x37, 0x63, 0xe7, 0x80, 0x69, 0x53, 0xe4, 0xc9, 0x1c, 0x60, 0x89, 0x5c, 
  0xe8, 0x19, 0xce, 0x51, 0x8d, 0x9b, 0xdb, 0xae, 0xa4, 0x3c, 0xf7, 0xf1, 0x7c, 0xee, 0xe0, 0xae, 
  0x7b, 0xcb, 0x07, 0xb2, 0xb6, 0x44, 0x9f, 0xd4, 0xbf, 0xc7, 0xaf, 0x9d, 0xcb, 0x00, 0xbf, 0x09, 
  0x56, 0x83, 0x9f, 0xfc, 0x82, 0x5e, 0x22, 0x52, 0x32, 0xf3, 0x42, 0x37, 0xbe, 0x8e, 0x5d, 0x06,
  0xc6, 0x56, 0xac, 0xfc, 0x3a, 0x95, 0x08, 0xf1, 0x51, 0xdc, 0x69, 0xba, 0x67, 0x2e, 0x44, 0xec, 
  0x1b, 0x2c, 0x15, 0x91, 0x5c, 0x3f, 0xf2, 0xc1, 0xfd, 0xeb, 0xee, 0x30, 0xaa, 0x94, 0x7c, 0x79, 
  0xde, 0xb8, 0xcb, 0x63, 0x6e, 0xf8, 0x28, 0x5e, 0xf5, 0x32, 0x2b, 0x34, 0x0f, 0xe4, 0x56, 0x4e, 
  0xf3, 0xf0, 0x3e, 0xe1, 0x68, 0x53, 0x26, 0x2d, 0x70, 0x98, 0x1e, 0x13, 0x61, 0x6f, 0x5f, 0x91, 
  0x39, 0x75, 0xa2, 0x11, 0x2c, 0x34, 0x96, 0xfd, 0x5e, 0x2f, 0x76, 0xe6, 0xa7, 0x3c, 0xf7, 0x37, 
  0x17, 0x88, 0x6e, 0xa5, 0x9f, 0x3d, 0x24, 0x9b, 0xe6, 0x54, 0x2d, 0x49, 0xf1, 0x00, 0x0d, 0x77, 
  0x87, 0xf0, 0xaa, 0xe6, 0xca, 0xdb, 0xc1, 0xcc, 0x97, 0x08, 0x22, 0x16, 0x78, 0xe5, 0x83, 0xf8, 
  0x0a, 0x63, 0x7c, 0xe6, 0x9a, 0x76, 0xc3, 0x3d, 0xc9, 0x06, 0x3b, 0xae, 0x98, 0xb4, 0xae, 0x18, 
  0x1e, 0xeb, 0x7a, 0x84, 0xb1, 0x30, 0x73, 0xd8, 0xbe, 0x14, 0x8b, 0x54, 0x15, 0xdb, 0x88, 0xf5, 
  0xa0, 0xc4, 0x43, 0xb6, 0xc8, 0xd2, 0x75, 0x0c, 0xeb, 0x15, 0xfd, 0x10, 0x45, 0xd1, 0xfc, 0x90, 
  0xa6, 0x41, 0xe0, 0xa7, 0x30, 0x48, 0x9f, 0x7c, 0x01, 0x54, 0x03, 0x9f, 0x7d, 0x58, 0xde, 0xac},

 {0xC8, 0xed, 0x36, 0xab, 0xa6, 0x36, 0x8e, 0x68, 0x26,	0x2f, 0x42, 0x30, 0xb0, 0x79, 0x56, 0xbc,
  0xfd, 0xbe, 0x7e, 0x2a, 0xb8, 0xea, 0xcf, 0x3e, 0x78, 0xb4, 0x22, 0x4f, 0x75, 0xb9, 0xfd, 0x31,
  0x0a, 0xbe, 0x08, 0x81, 0x87, 0xe8, 0xec, 0x2d, 0xbd, 0x96, 0xb1, 0xb7, 0x11, 0x5b, 0x8a, 0xc8,
  0xe7, 0xf3, 0x53, 0xcb, 0xa0, 0x7b, 0xf6, 0xfa, 0x93, 0x2e, 0x41, 0xf2, 0xbb, 0xbf, 0xb6, 0x2b,
  0x31, 0x1b, 0x5f, 0xea, 0xe3, 0xb4, 0x9a, 0x14, 0x5c, 0xb6, 0x78, 0x26, 0xe8, 0x65, 0x4c, 0xd9,
  0xc9, 0xc6, 0x9a, 0x84, 0xc3, 0x2d, 0xd6, 0x09, 0xae, 0x84, 0xdd, 0x4c, 0x71, 0x64, 0x8b, 0x64,
  0x36, 0xdb, 0x2c, 0x8e, 0x72, 0x5f, 0x30, 0xa0, 0xce, 0xf6, 0xfd, 0x23, 0xe7, 0xbd, 0x49, 0x85,
  0x5b, 0x08, 0x12, 0xcf, 0x7c, 0x07, 0xe1, 0x13, 0xb5, 0x94, 0xa6, 0x67, 0x8c, 0xf8, 0x47, 0x9a,
  0xe7, 0xae, 0xa1, 0xe4, 0x3e, 0x1f, 0x78, 0xe7, 0x1a, 0xdf, 0xc1, 0xc8, 0x73, 0x91, 0x48, 0xfc,
  0x1f, 0xba, 0xd0, 0x4a, 0xd3, 0xe6, 0x79, 0xe8, 0x0b, 0x4d, 0x41, 0x1f, 0x50, 0xbe, 0x54, 0xc0,
  0x5f, 0xea, 0xec, 0xe4, 0xe4, 0x7a, 0x72, 0xca, 0x8b, 0x02, 0xac, 0x6b, 0x6c, 0xfb, 0xc2, 0x53,
  0xf5, 0x0b, 0x1a, 0x86, 0xde, 0x7f, 0x0c, 0xdd, 0xb7, 0xac, 0xba, 0x06, 0x40, 0xf3, 0x6e, 0x75,
  0xba, 0xa0, 0x36, 0xff, 0x17, 0x5e, 0x95, 0x77, 0xfa, 0x19, 0x96, 0xa2, 0x40, 0x2f, 0xb4, 0x1a,
  0xf8, 0x83, 0x89, 0x23, 0x5c, 0x91, 0x7d, 0x6a, 0xc5, 0xfd, 0x2b, 0x87, 0x4a, 0x1d, 0x98, 0xa9,
  0x16, 0xec, 0xdc, 0x58, 0x5f, 0x7e, 0x57, 0x29, 0x53, 0x67, 0x17, 0x8b, 0x3e, 0xe5, 0xb3, 0x20,
  0x93, 0x76, 0x5f, 0x22, 0x9b, 0x66, 0xc6, 0x0a, 0xf4, 0x64, 0xac, 0x52, 0x3f, 0x91, 0x4e, 0x92},

 {0xf5, 0x8e, 0xd1, 0x99, 0x36, 0x25, 0xc5, 0x46, 0x48, 0xde, 0x34, 0x7f, 0xf8, 0x5b, 0x22, 0xd2,
  0x13, 0x57, 0x81, 0xd6, 0x2d, 0x63, 0x86, 0xa9, 0x1f, 0xe2, 0xe4, 0xad, 0x69, 0x07, 0xd7, 0xb2, 
  0x8d, 0x17, 0x13, 0x7c, 0xfa, 0xe2, 0x0c, 0x51, 0xb8, 0x21, 0xea, 0x54, 0x72, 0x63, 0xcf, 0x86, 
  0xd9, 0x4e, 0xb4, 0xa4, 0x8f, 0x38, 0x3f, 0x7f, 0x00, 0x8f, 0x6f, 0x24, 0xa6, 0x0c, 0xda, 0xce, 
  0x08, 0x79, 0x99, 0xb8, 0xe7, 0xb0, 0x9b, 0xc1, 0x8e, 0x75, 0x47, 0x63, 0x1d, 0x1e, 0xd4, 0x86, 
  0x30, 0x39, 0xa7, 0xa4, 0xbe, 0xf9, 0x68, 0x1e, 0x7f, 0xcc, 0x2d, 0xc3, 0x75, 0x6c, 0x4c, 0x47, 
  0x6d, 0x8a, 0xf4, 0xf5, 0x75, 0x8f, 0x84, 0xd6, 0x35, 0x00, 0x31, 0x63, 0x3e, 0xb8, 0x04, 0x3d, 
  0xf1, 0x9d, 0xb2, 0x62, 0xac, 0x5c, 0x26, 0x27, 0x62, 0x7e, 0xc3, 0x84, 0x66, 0x64, 0xa2, 0x66, 
  0xab, 0xd2, 0x01, 0x42, 0x1a, 0x1e, 0x2d, 0xa7, 0xea, 0xb0, 0xe6, 0x64, 0x12, 0x27, 0x36, 0x9f, 
  0x08, 0x49, 0x22, 0x72, 0x0d, 0x03, 0x6f, 0xa7, 0x1c, 0xd0, 0x05, 0xc9, 0x6f, 0x35, 0xfd, 0xff, 
  0x42, 0x93, 0x91, 0x26, 0x29, 0x08, 0x96, 0x25, 0xbe, 0x10, 0xf3, 0xbb, 0xf8, 0x6c, 0xe7, 0xfe, 
  0xc9, 0x00, 0x84, 0x28, 0xd6, 0x8d, 0xf6, 0xc3, 0x6d, 0xae, 0x89, 0xe6, 0xb4, 0xfd, 0x6b, 0xe5, 
  0x3d, 0x0a, 0x4f, 0x00, 0xe8, 0xaf, 0xf1, 0x5b, 0xd7, 0x58, 0x6d, 0x2c, 0xdf, 0x0c, 0x15, 0x17, 
  0x81, 0x5d, 0x2c, 0x91, 0xf7, 0xdb, 0x5b, 0x8f, 0x45, 0x75, 0x7d, 0xe6, 0x97, 0xd7, 0x65, 0x96, 
  0x62, 0xf9, 0xe9, 0x9c, 0xef, 0x1c, 0x6a, 0xf2, 0xfa, 0xc8, 0x63, 0x4e, 0xef, 0xcf, 0x64, 0x67, 
  0x4d, 0xf7, 0xfa, 0xb0, 0x57, 0xa7, 0x9d, 0x36, 0xeb, 0xed, 0xc3, 0x99, 0x02, 0x38, 0x8f, 0x31},

 {0x26, 0x2e, 0x92, 0x04, 0x9e, 0x3b, 0x51, 0xaf, 0xc9, 0x66, 0x48, 0x25, 0x88, 0x1b, 0x24, 0xba,
  0x97, 0xae, 0x88, 0x05, 0x6e, 0x16, 0xb5, 0x59, 0x6c, 0x20, 0xf3, 0x07, 0xb3, 0x86, 0x28, 0xe0, 
  0xf7, 0x28, 0xa1, 0x7c, 0xed, 0xf7, 0x7c, 0xf3, 0xec, 0x65, 0xd5, 0x2c, 0xd4, 0x32, 0x51, 0xb7, 
  0x42, 0x2a, 0x69, 0x29, 0xaa, 0x04, 0x5a, 0x43, 0xef, 0xe6, 0x70, 0xd8, 0xb9, 0x31, 0x16, 0x40, 
  0xd5, 0x1b, 0xc2, 0x47, 0xa6, 0x62, 0xc1, 0x1b, 0xc8, 0xb8, 0x91, 0x51, 0xc2, 0xf1, 0x88, 0x41, 
  0xf2, 0xe6, 0x21, 0x29, 0x8c, 0xd5, 0x76, 0x32, 0xe5, 0xb4, 0x41, 0x38, 0x11, 0x2b, 0xa8, 0x75, 
  0x35, 0x2c, 0x43, 0x7a, 0xec, 0x0e, 0xb9, 0xba, 0x64, 0xab, 0x61, 0xc5, 0xf8, 0xb9, 0x82, 0xad, 
  0x6c, 0x9a, 0xe4, 0x97, 0x08, 0x30, 0x84, 0x3f, 0x50, 0xe5, 0x89, 0x66, 0x39, 0xe2, 0xb2, 0x7f, 
  0x58, 0xba, 0xde, 0x8d, 0xa2, 0x05, 0x68, 0x3a, 0x08, 0x78, 0x9f, 0x41, 0x88, 0x9a, 0xbb, 0xf1, 
  0xda, 0xe3, 0x51, 0x3d, 0x53, 0x60, 0x81, 0xf3, 0x52, 0xed, 0xa2, 0x0c, 0xdd, 0x26, 0xb2, 0xa1, 
  0x05, 0xb9, 0x3d, 0x16, 0xf3, 0x33, 0x01, 0x1d, 0x8f, 0xb1, 0x33, 0xd7, 0x09, 0xc5, 0xcf, 0xf9, 
  0xa1, 0xb8, 0x13, 0xfb, 0x83, 0xd8, 0xda, 0xaf, 0xa2, 0xe4, 0x55, 0x29, 0x16, 0xb7, 0x4a, 0xd8, 
  0x8c, 0x53, 0xc3, 0xc6, 0x1e, 0x05, 0xf8, 0x88, 0xf7, 0xea, 0xe7, 0x02, 0xf0, 0x41, 0x18, 0x41, 
  0x8b, 0x27, 0xc3, 0xe7, 0x7d, 0xf7, 0x9c, 0x4d, 0x3a, 0x55, 0x52, 0x43, 0xda, 0x1c, 0xff, 0x8f, 
  0x05, 0xba, 0x8b, 0xac, 0x79, 0x4a, 0x4b, 0x08, 0x32, 0xa1, 0xf9, 0xef, 0x27, 0xd8, 0x84, 0xa3, 
  0x22, 0x4e, 0x10, 0xa6, 0x1d, 0x06, 0xda, 0x0c, 0x46, 0x3e, 0xdb, 0xd3, 0xbd, 0xb1, 0x33, 0x99},

 {0xd7, 0x0e, 0xb3, 0xef, 0xd0, 0x7e, 0xe3, 0x38, 0x1b, 0xed, 0xa6, 0x78, 0xa0, 0x33, 0x07, 0x65, 
  0xf3, 0x84, 0x8c, 0x16, 0x8a, 0x5a, 0xeb, 0x58, 0x91, 0x86, 0x5b, 0x83, 0x65, 0x58, 0x16, 0xb1, 
  0xfb, 0x8d, 0xd0, 0x2f, 0xda, 0xb6, 0x73, 0x42, 0x3c, 0xf5, 0x6c, 0x92, 0x20, 0x26, 0x2c, 0x3e, 
  0xf6, 0x12, 0x3c, 0x07, 0x5a, 0xd3, 0x90, 0x0a, 0x32, 0x63, 0x29, 0x3e, 0x14, 0x32, 0x49, 0x51, 
  0x77, 0xab, 0xb6, 0x6a, 0xa0, 0xea, 0x6e, 0x30, 0xab, 0x14, 0x5f, 0xa2, 0x4d, 0x8e, 0x48, 0x78, 
  0x2f, 0xc2, 0x00, 0x2f, 0xb4, 0x31, 0xc2, 0x82, 0x3b, 0x77, 0xed, 0xb8, 0x98, 0x17, 0xef, 0x08, 
  0x79, 0x38, 0xb9, 0x84, 0x1a, 0x09, 0x16, 0xe5, 0x49, 0x00, 0xc8, 0x42, 0x3e, 0x18, 0x23, 0xd6, 
  0x54, 0x0b, 0x1d, 0x11, 0xf2, 0xc1, 0x84, 0x88, 0x33, 0x36, 0x18, 0xbe, 0x26, 0xd1, 0xc1, 0x8b, 
  0x5c, 0x76, 0x0a, 0x63, 0xa8, 0x78, 0x53, 0x07, 0xae, 0x7f, 0xcd, 0xec, 0xbf, 0x59, 0x9b, 0x33, 
  0x2b, 0x06, 0xce, 0x25, 0xc5, 0x9f, 0xf1, 0x00, 0xd7, 0x31, 0x3b, 0x59, 0x44, 0x57, 0x1f, 0x71, 
  0xba, 0x3c, 0x34, 0xa4, 0x44, 0x94, 0xe0, 0x8c, 0x7d, 0x53, 0x41, 0x7f, 0xd6, 0x24, 0x1b, 0xe5, 
  0x31, 0x20, 0x5e, 0x27, 0x0d, 0xdc, 0xfd, 0x2b, 0x1b, 0xa6, 0x6e, 0xed, 0xea, 0xbd, 0x2c, 0x3b, 
  0xb0, 0x52, 0xdc, 0x89, 0xf6, 0x79, 0xb0, 0x9b, 0x0a, 0x66, 0xb2, 0xfe, 0x88, 0x1c, 0x51, 0x69, 
  0x88, 0x20, 0x8b, 0xab, 0xe8, 0xe0, 0x81, 0x1c, 0x65, 0x4a, 0x09, 0x9b, 0xe7, 0x6b, 0x1f, 0x9d, 
  0x6d, 0x0e, 0xbe, 0x2b, 0x89, 0x07, 0x94, 0xa2, 0x29, 0x3d, 0xb9, 0x90, 0xd8, 0x8e, 0x24, 0x4f,
  0x1f, 0x63, 0x21, 0xec, 0x09, 0x14, 0x82, 0x74, 0x04, 0x59, 0x8a, 0xe4, 0x86, 0x94, 0xe1, 0x0a}
};


/******************************************************************************
* 【内部接口声明】
******************************************************************************/

// 数据结构初始化
void BLE_PROTOCOL_DataStructInit(BLE_PROTOCOL_CB* pCB);

// UART报文接收处理函数(注意根据具体模块修改)
void BLE_PROTOCOL_MacProcess(uint16 standarID, uint8* pData, uint16 length);

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void BLE_PROTOCOL_RxFIFOProcess(BLE_PROTOCOL_CB* pCB);

// UART命令帧缓冲区处理
void BLE_PROTOCOL_CmdFrameProcess(BLE_PROTOCOL_CB* pCB);

// 对传入的命令帧进行校验，返回校验结果
BOOL BLE_PROTOCOL_CheckSUM(BLE_PROTOCOL_RX_CMD_FRAME* pCmdFrame);

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL BLE_PROTOCOL_ConfirmTempCmdFrameBuff(BLE_PROTOCOL_CB* pCB);

// 通讯超时处理-单向
void BLE_PROTOCOL_CALLBACK_RxTimeOut(uint32 param);

// 停止RX通讯超时检测任务
void BLE_PROTOCOL_StopRxTimeOutCheck(void);

// TXRX通讯超时处理-双向
void BLE_PROTOCOL_CALLBACK_TxRxTimeOut(uint32 param);

// 停止TXRX通讯超时检测任务
void BLE_PROTOCOL_StopTxRxTimeOutCheck(void);

// 协议层发送处理过程
void BLE_PROTOCOL_TxStateProcess(void);

// UART协议层向驱动层注册数据发送接口
void BLE_PROTOCOL_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length));

//===============================================================================================================
// 发送命令回复
void BLE_PROTOCOL_SendCmdAck(uint8 ackCmd);

// 发送带应答的回复
void BLE_PROTOCOL_SendCmdParamAck(uint8 ackCmd, uint8 ackParam);

// 固件版本回复
void BLE_PROTOCOL_SendCmdFirmVersionAck(void);

// 仪表状态信息回复
void BLE_PROTOCOL_SendCmdStatusAck(void);

// 仪表控制命令回复
//void BLE_PROTOCOL_SendCmdControlAck(uint8 controlCmd, uint8 paramH, uint8 paramL);

// 仪表获取实时运动状态回复
void BLE_PROTOCOL_SendCmdGetRuningStateAck(void);

// 仪表上报历史数据
void BLE_PROTOCOL_SendHistoryData(uint32 param);

// 仪表状态组1信息回复
void BLE_PROTOCOL_SendCmdSelectStatus1Ack(void);

// 仪表状态组2信息回复
void BLE_PROTOCOL_SendCmdSelectStatus2Ack(void);

// 仪表状态组3信息回复
void BLE_PROTOCOL_SendCmdSelectStatus3Ack(void);

//===============================================================================================================
// 启动通讯超时判断任务
void BLE_PROTOCOL_StartTimeoutCheckTask(void);

// UART总线超时错误处理
void BLE_PROTOCOL_CALLBACK_UartBusError(uint32 param);


void BLE_PROTOCOL_RecordSave(TIME_E time_t, RECORD_ITEM_TMP_CB *recordTmpCB_t, uint32 timeTmpStart_t);

//=======================================================================================
// 全局变量定义
BLE_PROTOCOL_CB bleProtocolCB;

// ■■函数定义区■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// 启动通讯超时判断任务
void BLE_PROTOCOL_StartTimeoutCheckTask(void)
{
}

// 发送获取MAC地址
void BLE_UART_SendCmdGetBleMacAddr(uint32 param)
{
	uint8 i;
	uint8 bleMacStr[] = "TTM:MAC-?";
	
	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	// 查询蓝牙mac指令,启动阻塞式发送
	for (i = 0; bleMacStr[i] != '\0'; i++)
	{
		// 填充数据，启动发送
		// 防止丢失第一个字节(清除空标志位,务必加) 
		USART_ClrFlag(BLE_UART_TYPE_DEF, USART_FLAG_TXC);		

		// 填充数据
		USART_SendData(BLE_UART_TYPE_DEF, bleMacStr[i]);

		// 未发送完，持续等待
		while(USART_GetFlagStatus(BLE_UART_TYPE_DEF, USART_FLAG_TXC) != SET);
	}	
}

// 定时器回调发送获取蓝牙地址命令
void BLE_UART_CALLBALL_GetBleMacAddrRequest(uint32 param)
{
	// 发送获取MAC地址
	BLE_UART_SendCmdGetBleMacAddr(TRUE);
}

// 协议初始化
void BLE_PROTOCOL_Init(void)
{
	// 协议层数据结构初始化
	BLE_PROTOCOL_DataStructInit(&bleProtocolCB);

	// 向驱动层注册数据接收接口
	BLE_UART_RegisterDataSendService(BLE_PROTOCOL_MacProcess);

	// 向驱动层注册数据发送接口
	BLE_PROTOCOL_RegisterDataSendService(BLE_UART_AddTxArray);

	// 注册查询BLE的MAC地址
	TIMER_AddTask(TIMER_ID_BLE_CONTROL,
					200,
					BLE_UART_CALLBALL_GetBleMacAddrRequest,
					TRUE,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);

	// 上电默认出厂广播设置
	TIMER_AddTask(TIMER_ID_BLE_ADV_RESET,
					2000,
					BLE_PROTOCOL_SendAdv,
					1,
					1,
					ACTION_MODE_ADD_TO_QUEUE);
}

// BLE协议层过程处理
void BLE_PROTOCOL_Process(void)
{
	// BLE接收FIFO缓冲区处理
	BLE_PROTOCOL_RxFIFOProcess(&bleProtocolCB);

	// BLE接收命令缓冲区处理
	BLE_PROTOCOL_CmdFrameProcess(&bleProtocolCB);
	
	// BLE协议层发送处理过程
	BLE_PROTOCOL_TxStateProcess();
}

// 向发送命令帧队列中添加数据
void BLE_PROTOCOL_TxAddData(uint8 data)
{
	uint16 head = bleProtocolCB.tx.head;
	uint16 end =  bleProtocolCB.tx.end;
	BLE_PROTOCOL_TX_CMD_FRAME* pCmdFrame = &bleProtocolCB.tx.cmdQueue[bleProtocolCB.tx.end];

	// 发送缓冲区已满，不予接收
	if((end + 1) % BLE_PROTOCOL_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 队尾命令帧已满，退出
	if(pCmdFrame->length >= BLE_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX)
	{
		return;
	}

	// 数据添加到帧末尾，并更新帧长度
	pCmdFrame->buff[pCmdFrame->length] = data;
	pCmdFrame->length ++;
}

// 确认添加命令帧，即承认之前填充的数据为命令帧，将其添加到发送队列中，由main进行调度发送，本函数内会自动校正命令长度，并添加校验码
void BLE_PROTOCOL_TxAddFrame(void)
{
	uint16 cc = 0;
	uint16 i = 0;
	uint16 head = bleProtocolCB.tx.head;
	uint16 end  = bleProtocolCB.tx.end;
	BLE_PROTOCOL_TX_CMD_FRAME* pCmdFrame = &bleProtocolCB.tx.cmdQueue[bleProtocolCB.tx.end];
	uint16 length = pCmdFrame->length;

	// 发送缓冲区已满，不予接收
	if((end + 1) % BLE_PROTOCOL_TX_QUEUE_SIZE == head)
	{
		return;
	}
	
	// 命令帧长度不足，清除已填充的数据，退出
	if(BLE_PROTOCOL_CMD_FRAME_LENGTH_MIN-1 > length)	// 减去"校验和"1个字节
	{
		pCmdFrame->length = 0;
		
		return;
	}

	// 队尾命令帧已满，退出
	if(length >= BLE_PROTOCOL_TX_CMD_FRAME_LENGTH_MAX)
	{
		return;
	}

	// 重新设置数据长度，系统在准备数据时，填充的"数据长度"可以为任意值，并且不需要添加校验码，在这里重新设置为正确的值
	pCmdFrame->buff[BLE_PROTOCOL_CMD_LENGTH_INDEX] = length - 3;	// 重设数据长度

	for(i=0; i<length; i++)
	{
		cc ^= pCmdFrame->buff[i];
	}
	pCmdFrame->buff[pCmdFrame->length++] = ~cc ;

	bleProtocolCB.tx.end ++;
	bleProtocolCB.tx.end %= BLE_PROTOCOL_TX_QUEUE_SIZE;
	//pCB->tx.cmdQueue[pCB->tx.end].length = 0;   //2015.12.2修改
}

void BLE_PROTOCOL_TxErrorInfo(uint32 param)
{
	uint8 val1,val2;
	uint16 valTemp = (uint16)param;
	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_ERROR_UP); 	
	BLE_PROTOCOL_TxAddData(0x02);
	
	val1 = (uint8)((valTemp >> 8) & 0xFF);
	val2 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	BLE_PROTOCOL_TxAddFrame();	
}

// 仪表启动同步历史数据
void BLE_PROTOCOL_TxStartHistoryData(uint32 param)
{
	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_START_HISTORICAL_DATA_UP); 	
	BLE_PROTOCOL_TxAddData(0x00);
	BLE_PROTOCOL_TxAddFrame();	
}


// 数据结构初始化
void BLE_PROTOCOL_DataStructInit(BLE_PROTOCOL_CB* pCB)
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
	for(i = 0; i < BLE_PROTOCOL_TX_QUEUE_SIZE; i++)
	{
		pCB->tx.cmdQueue[i].length = 0;
	}

	pCB->rxFIFO.head = 0;
	pCB->rxFIFO.end = 0;
	pCB->rxFIFO.currentProcessIndex = 0;

	pCB->rx.head = 0;
	pCB->rx.end  = 0;
	for(i=0; i<BLE_PROTOCOL_RX_QUEUE_SIZE; i++)
	{
		pCB->rx.cmdQueue[i].length = 0;
	}

	pCB->isTimeCheck = FALSE;
}

// UART报文接收处理函数(注意根据具体模块修改)
void BLE_PROTOCOL_MacProcess(uint16 standarID, uint8* pData, uint16 length)
{
	uint16 end = bleProtocolCB.rxFIFO.end;
	uint16 head = bleProtocolCB.rxFIFO.head;
	uint8 rxdata = 0x00;
	
	// 接收数据
	rxdata = *pData;

	// 一级缓冲区已满，不予接收
	if((end + 1)%BLE_PROTOCOL_RX_FIFO_SIZE == head)
	{
		return;
	}
	// 一级缓冲区未满，接收 
	else
	{
		// 将接收到的数据放到临时缓冲区中
		bleProtocolCB.rxFIFO.buff[end] = rxdata;
		bleProtocolCB.rxFIFO.end ++;
		bleProtocolCB.rxFIFO.end %= BLE_PROTOCOL_RX_FIFO_SIZE;
	}	
}

// UART协议层向驱动层注册数据发送接口
void BLE_PROTOCOL_RegisterDataSendService(BOOL (*service)(uint16 id, uint8 *pData, uint16 length))
{		
	bleProtocolCB.sendDataThrowService = service;
}

// 将临时缓冲区添加到命令帧缓冲区中，其本质操作是承认临时缓冲区数据有效
BOOL BLE_PROTOCOL_ConfirmTempCmdFrameBuff(BLE_PROTOCOL_CB* pCB)
{
	BLE_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;
	
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
	pCB->rx.end %= BLE_PROTOCOL_RX_QUEUE_SIZE;
	pCB->rx.cmdQueue[pCB->rx.end].length = 0;	// 该行的作用是将新的添加位置有效数据个数清零，以便将这个位置当做临时帧缓冲区
	
	return TRUE;
}

// 协议层发送处理过程
void BLE_PROTOCOL_TxStateProcess(void)
{
	uint16 head = bleProtocolCB.tx.head;
	uint16 end =  bleProtocolCB.tx.end;
	uint16 length = bleProtocolCB.tx.cmdQueue[head].length;
	uint8* pCmd = bleProtocolCB.tx.cmdQueue[head].buff;
	uint16 localDeviceID = bleProtocolCB.tx.cmdQueue[head].deviceID;

	// 发送缓冲区为空，说明无数据
	if (head == end)
	{
		return;
	}

	// 发送函数没有注册直接返回
	if (NULL == bleProtocolCB.sendDataThrowService)
	{
		return;
	}

	// 协议层有数据需要发送到驱动层		
	if (!(*bleProtocolCB.sendDataThrowService)(localDeviceID, pCmd, length))
	{
		return;
	}

	// 发送环形队列更新位置
	bleProtocolCB.tx.cmdQueue[head].length = 0;
	bleProtocolCB.tx.head ++;
	bleProtocolCB.tx.head %= BLE_PROTOCOL_TX_QUEUE_SIZE;
}

// 一级接收缓冲区处理，从一级接收缓冲区中取出一个字节添加到命令帧缓冲区中
void BLE_PROTOCOL_RxFIFOProcess(BLE_PROTOCOL_CB* pCB)
{
	uint16 end = pCB->rxFIFO.end;
	uint16 head = pCB->rxFIFO.head;
	BLE_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;
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
		if(BLE_PROTOCOL_CMD_HEAD != currentData)
		{
			pCB->rxFIFO.head ++;
			pCB->rxFIFO.head %= BLE_PROTOCOL_RX_FIFO_SIZE;
			pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;

			return;
		}
		
		// 命令头正确，但无临时缓冲区可用，退出
		if((pCB->rx.end + 1)%BLE_PROTOCOL_RX_QUEUE_SIZE == pCB->rx.head)
		{
			return;
		}

		// 添加UART通讯超时时间设置-2016.1.5增加
#if BLE_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
		TIMER_AddTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL,
						BLE_PROTOCOL_BUS_UNIDIRECTIONAL_TIME_OUT,
						BLE_PROTOCOL_CALLBACK_RxTimeOut,
						0,
						1,
						ACTION_MODE_ADD_TO_QUEUE);
#endif
		
		// 命令头正确，有临时缓冲区可用，则将其添加到命令帧临时缓冲区中
		pCmdFrame->buff[pCmdFrame->length++]= currentData;
		pCB->rxFIFO.currentProcessIndex ++;
		pCB->rxFIFO.currentProcessIndex %= BLE_PROTOCOL_RX_FIFO_SIZE;
	}
	// 非首字节，将数据添加到命令帧临时缓冲区中，但暂不删除当前数据
	else
	{
		// 临时缓冲区溢出，说明当前正在接收的命令帧是错误的，正确的命令帧不会出现长度溢出的情况
		if(pCmdFrame->length >= BLE_PROTOCOL_RX_CMD_FRAME_LENGTH_MAX)
		{
#if BLE_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
			// 停止RX通讯超时检测
			BLE_PROTOCOL_StopRxTimeOutCheck();
#endif

			// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
			pCmdFrame->length = 0;	// 2016.1.5增加
			// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
			pCB->rxFIFO.head ++;
			pCB->rxFIFO.head %= BLE_PROTOCOL_RX_FIFO_SIZE;
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
			pCB->rxFIFO.currentProcessIndex %= BLE_PROTOCOL_RX_FIFO_SIZE;

			// ■■接下来，需要检查命令帧是否完整，如果完整，则将命令帧临时缓冲区扶正 ■■
						
			// 首先判断命令帧最小长度，一个完整的命令字至少包括4个字节: 命令帧最小长度，包含:命令头、命令字、数据长度、校验和，因此不足4个字节的必定不完整
			if(pCmdFrame->length < BLE_PROTOCOL_CMD_FRAME_LENGTH_MIN)
			{
				// 继续接收
				continue;
			}

			// 命令帧长度数值越界，说明当前命令帧错误，停止接收
			if(pCmdFrame->buff[BLE_PROTOCOL_CMD_LENGTH_INDEX] > (BLE_PROTOCOL_RX_CMD_FRAME_LENGTH_MAX - BLE_PROTOCOL_CMD_FRAME_LENGTH_MIN))
			{
#if BLE_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
				// 停止RX通讯超时检测
				BLE_PROTOCOL_StopRxTimeOutCheck();
#endif
			
				// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
				pCmdFrame->length = 0;
				// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
				pCB->rxFIFO.head ++;
				pCB->rxFIFO.head %= BLE_PROTOCOL_RX_FIFO_SIZE;
				pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;

				return;
			}

			// 命令帧长度校验
			length = pCmdFrame->length;
			if(length < pCmdFrame->buff[BLE_PROTOCOL_CMD_LENGTH_INDEX] + BLE_PROTOCOL_CMD_FRAME_LENGTH_MIN)
			{
				// 长度要求不一致，说明未接收完毕，退出继续
				continue;
			}

			// 命令帧长度OK，则进行校验，失败时删除命令头
			if(!BLE_PROTOCOL_CheckSUM(pCmdFrame))
			{
#if BLE_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
				// 停止RX通讯超时检测
				BLE_PROTOCOL_StopRxTimeOutCheck();
#endif
				
				// 校验失败，将命令帧长度清零，即认为抛弃该命令帧
				pCmdFrame->length = 0;
				// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
				pCB->rxFIFO.head ++;
				pCB->rxFIFO.head %= BLE_PROTOCOL_RX_FIFO_SIZE;
				pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;
				
				return;
			}

#if BLE_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
			// 停止RX通讯超时检测
			BLE_PROTOCOL_StopRxTimeOutCheck();
#endif
			
			// 执行到这里，即说明接收到了一个完整并且正确的命令帧，此时需将处理过的数据从一级缓冲区中删除，并将该命令帧扶正
			pCB->rxFIFO.head += length;
			pCB->rxFIFO.head %= BLE_PROTOCOL_RX_FIFO_SIZE;
			pCB->rxFIFO.currentProcessIndex = pCB->rxFIFO.head;
			BLE_PROTOCOL_ConfirmTempCmdFrameBuff(pCB);

			return;
		}
	}
}

// 对传入的命令帧进行校验，返回校验结果
BOOL BLE_PROTOCOL_CheckSUM(BLE_PROTOCOL_RX_CMD_FRAME* pCmdFrame)
{
	uint8 cc = 0;
	uint16 i = 0;
	
	if(NULL == pCmdFrame)
	{
		return FALSE;
	}

	// 从命令头开始，到校验码之前的一个字节，依次进行异或运算
	for(i=0; i<pCmdFrame->length-1; i++)
	{
		cc ^= pCmdFrame->buff[i];
	}

	cc = ~cc;
	
	// 判断计算得到的校验码与命令帧中的校验码是否相同
	if(pCmdFrame->buff[pCmdFrame->length-1] != cc)
	{
		return FALSE;
	}
	
	return TRUE;
}

// BLE命令帧缓冲区处理
void BLE_PROTOCOL_CmdFrameProcess(BLE_PROTOCOL_CB* pCB)
{
//	BLE_PROTOCOL_CMD cmd = BLE_CMD_MAX;
//	BLE_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;
//	TIME_E time;
//	TIME_E record_d_time;	// 已经记录时间
//	TIME_E record_c_time;	// DS1302现在记录的时间
//	RECORD_ITEM_CB track;
//	
//	uint32 bleCheckTime;	// 用于校准的时间戳
//	uint32 d_time;	// 已经记录的时间戳
//	uint32 c_time;	// DS1302现在记录的时间戳

//	uint16 count;
//	uint16 i;
//	uint32 startTimeAddr;
	
	BLE_PROTOCOL_CMD cmd = BLE_CMD_MAX;
	BLE_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;
	TIME_E time;
	TIME_E timeBk;	// 时间备份临时变量

	uint32 timeCurrent;		// 当前时间点
	uint32 timeTmpStart;	// 临时起点时间
	uint32 timeTmp;			// 时间戳转换临时变量暂存
	uint32 timeTmpBefore;	// 零点之前的时间
	uint16 tirpNumBk;		// 骑行条数备份
	uint16 i;
	uint32 bBuff[1];

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
	if(BLE_PROTOCOL_CMD_HEAD != pCmdFrame->buff[BLE_PROTOCOL_CMD_HEAD_INDEX])
	{
		// 删除命令帧
		pCB->rx.head ++;
		pCB->rx.head %= BLE_PROTOCOL_RX_QUEUE_SIZE;
		return;
	}

	// 命令头合法，则提取命令
	cmd = (BLE_PROTOCOL_CMD)pCmdFrame->buff[BLE_PROTOCOL_CMD_CMD_INDEX];
	
	// 执行命令帧
	switch (cmd)
	{
		// 心跳命令
		case BLE_CMD_HEARTBEAT_DOWN:
			// APP心跳命令应答:0x01
			BLE_PROTOCOL_SendCmdParamAck(BLE_CMD_HEARTBEAT_UP,0x01);
			break;

		// 权鉴命令
		case BLE_CMD_AUTHENTICATE_DOWN:
			if ((Authentication_Table[pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA1_INDEX] - 1][pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA2_INDEX]] \
			== pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA3_INDEX]) && \
					(Authentication_Table[pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA4_INDEX] - 1][pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA5_INDEX]] \
			== pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA6_INDEX]) && \
					(Authentication_Table[pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA7_INDEX] - 1][pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA8_INDEX]] \
			== pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA9_INDEX]))
			{
				// 权鉴判断成功
				PARAM_SetBleConnectState(TRUE);

				// 上报系统错误信息
				TIMER_AddTask(TIMER_ID_BLE_ERROR_INFO,
							1000,
							BLE_PROTOCOL_TxErrorInfo,
							(uint32)PARAM_GetErrorCode(),
							TIMER_LOOP_FOREVER,
							ACTION_MODE_ADD_TO_QUEUE);

				BLE_PROTOCOL_SendCmdParamAck(BLE_CMD_AUTHENTICATE_UP,0x00);
			}
			else
			{
				// 权鉴判断失败
				BLE_PROTOCOL_SendCmdParamAck(BLE_CMD_AUTHENTICATE_UP,0x01);
				// 断开蓝牙连接处理
				TIMER_AddTask(TIMER_ID_BLE_DISCONNECT,
								1000,
								BLE_MODULE_Reset,
								UINT32_NULL,
								1,
								ACTION_MODE_ADD_TO_QUEUE);
			}
			TIMER_KillTask(TIMER_ID_BLE_CONNECT_VERIFICATE);
			break;

		// 询问固件版本命令
		case BLE_CMD_FIRMVERSION_DOWN:
			// 固件版本查询回包
			BLE_PROTOCOL_SendCmdFirmVersionAck();
			break;

		// 蓝牙授时命令
		case BLE_CMD_SETTIMER_DOWN:
			time.year = 2000 + pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA2_INDEX];
			time.month= pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA3_INDEX];
			time.day= pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA4_INDEX];
			time.hour= pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA5_INDEX];
			time.minute= pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA6_INDEX];
			time.second= pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA7_INDEX];
			
			// 设置的时间戳年份超出阈值，则设置失败	
			//DS1302_GetDateCB(&record_c_time);

			if (UINT32_NULL == DS1302_SetDate(time.year, time.month, time.day, time.hour, time.minute, time.second))
			{
				BLE_PROTOCOL_SendCmdParamAck(BLE_CMD_SETTIMER_UP,0x01);
				break;
			}


			// RECORD结束当前记录，进入待机状态，同时将当前RAM数据保存到NVM
			if ( (RECORD_STATE_RECORDING == RECORD_GetState()) && (bleProtocolCB.isTimeCheck != TRUE) )
			{
				RECORD_EnterState(RECORD_STATE_STANDBY);
			}
			
			if (bleProtocolCB.isTimeCheck == FALSE)
			{
				// 关闭非授时数据存储定时器
				TIMER_KillTask(TIMER_ID_RECORD_COUNT);
				
				// 将非授时数据整理暂存到flash中去，然后清空非授时数据
				// 计算非授时记录起点时间(注意骑行时间经过0点处理)
				timeCurrent = (uint32)time.hour * 3600 + (uint32)time.minute * 60 + (uint32)time.second;
				if (timeCurrent > recordTmpCB.tirpTime)
				{
					timeTmpStart = timeCurrent - recordTmpCB.tirpTime;
					
					// 存储暂存骑行数据
					BLE_PROTOCOL_RecordSave(time, &recordTmpCB, timeTmpStart);
				}
				else
				{
					// 骑行过零点处理（分两节）
					// 零点之前的数据暂存
					
					timeTmpBefore = (recordTmpCB.tirpTime - timeCurrent)/60;
					for (i = 0; i < recordTmpCB.tirpNum; i++)
					{
						if (recordTmpCB.recordItemTmp[i].recordparam.min >= timeTmpBefore)
						{
							break;
						}
					}
					// 备份原始骑行数据条数
					tirpNumBk = recordTmpCB.tirpNum;
					recordTmpCB.tirpNum = i - 1;
					
					// 时间戳减去24小时然后转化为时间
					timeTmp = DS1302_GetTimeStampFromDateTime(time) - 86400;
					DS1302_TimeStampToDateTime(timeTmp, &timeBk);

					timeTmpStart = 86400 - (recordTmpCB.tirpTime - timeCurrent);
					
					BLE_PROTOCOL_RecordSave(timeBk, &recordTmpCB, timeTmpStart);

					// 清空已经存入flash的数据
					memset(&(recordTmpCB.recordItemTmp[0]), 0, (recordTmpCB.tirpNum + 1) * sizeof(RECORD_ITEM_TMP));
					
					// 剩下的数据累计分钟需要减去0点之前的数据
					for (i = recordTmpCB.tirpNum; i < tirpNumBk; i++)
					{
						if (recordTmpCB.recordItemTmp[i].recordparam.min > timeTmpBefore)
						{
							recordTmpCB.recordItemTmp[i].recordparam.min -= timeTmpBefore;
						}
						else
						{
							recordTmpCB.recordItemTmp[i].recordparam.min = 0;
						}
						
					}

					// 零点之后的数据暂存
					recordTmpCB.tirpNum = tirpNumBk;
					timeTmpStart = 0;

					BLE_PROTOCOL_RecordSave(time, &recordTmpCB, timeTmpStart);
				}
				memset(&recordTmpCB, 0, sizeof(RECORD_ITEM_TMP_CB));
			}
			
			// 蓝牙校准成功
			bleProtocolCB.isTimeCheck = TRUE;

			BLE_PROTOCOL_SendCmdParamAck(BLE_CMD_SETTIMER_UP,0x00);
				
			if (PARAM_GetSpeed() == 0)
			{
				// 启动同步历史数据
				if(RECORD_IsHistoricalData())
				{
					// 启动同步历史数据，如果没有接收到应答，五秒中循环发送请求同步数据
					TIMER_AddTask(TIMER_ID_BLE_START_HISTORICAL_DATA_UP,
									5000,
									BLE_PROTOCOL_TxStartHistoryData,
									UINT32_NULL,
									TIMER_LOOP_FOREVER,
									ACTION_MODE_ADD_TO_QUEUE);
				}
			}
			break;

		// 错误代码接收应答命令
		case BLE_CMD_ERROR_DOWN:
			// 停止继续上报错误信息
			TIMER_KillTask(TIMER_ID_BLE_ERROR_INFO);
			break;

		// 复位命令
		case BLE_CMD_RESET_DOWN:
		
			// 在APP端复位，写两个字节flash标志位，第一个字节表示蓝牙无线升级，第二个字节表示app跳转boot的标志位
			bBuff[0] = 0xAAAA0000;
			IAP_FlashWriteWordArrayWithErase(PARAM_MCU_BLE_UPDATA_FLAG_ADDEESS, bBuff, 1);
		
			// 跳转到boot进行蓝牙无线升级
			IAP_JumpToAppFun(IAP_N32_FLASH_BASE);
		
			break;

		// 固件升级命令
		case BLE_CMD_FIRMUPDATE_DOWN:
			break;

		// 数据包写入命令
		case BLE_CMD_DATAIN_DOWN:
			break;

		// 数据包写入完成命令
		case BLE_CMD_DATAIN_FINISH_DOWN:
			break;

		// 获取仪表状态命令
		case BLE_CMD_GETSTATUS_DOWN:
			BLE_PROTOCOL_SendCmdStatusAck();
			break;

		// 控制仪表命令
		case BLE_CMD_CONTROL_DOWN:
			BLE_PROTOCOL_SendCmdControlAck(pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA1_INDEX],\
											pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA2_INDEX],\
											pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA3_INDEX]);
			break;
		
		// 选择性获取参数上报
		case BLE_CMD_SELECT_GETSTATUS_DOWN:
			if (pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA1_INDEX] == 0x01)
			{
				// 仪表状态组1信息回复
				BLE_PROTOCOL_SendCmdSelectStatus1Ack();
			}
			else if (pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA1_INDEX] == 0x02)
			{
				// 仪表状态组2信息回复
				BLE_PROTOCOL_SendCmdSelectStatus2Ack();
			}
			else if (pCmdFrame->buff[BLE_PROTOCOL_CMD_DATA1_INDEX] == 0x03)
			{
				// 仪表状态组3信息回复
				BLE_PROTOCOL_SendCmdSelectStatus3Ack();
			}
			break;

		// 获取实时运动数据命令
		case BLE_CMD_GETDATA_RUNNING_DOWN:
			BLE_PROTOCOL_SendCmdGetRuningStateAck();
			break;

		// 启动同步历史运动数据应答命令
		case BLE_CMD_START_HISTORICAL_DATA_DOWN:
			// 关闭周期性发送启动历史数据同步函数
			TIMER_KillTask(TIMER_ID_BLE_START_HISTORICAL_DATA_UP);
			
			// 速度不为0，不允许上报历史数据
			if (PARAM_GetSpeed() == 0)
			{
				// 发送第一条历史上报数据
				BLE_PROTOCOL_SendHistoryData(UINT32_NULL);

				// 历史数据没有同步完成，继续发送同步历史数据，五秒中循环发送请求同步数据
				if(RECORD_IsHistoricalData())
				{
					TIMER_AddTask(TIMER_ID_BLE_START_HISTORICAL_DATA_UP,
									5000,
									BLE_PROTOCOL_TxStartHistoryData,
									UINT32_NULL,
									TIMER_LOOP_FOREVER,
									ACTION_MODE_ADD_TO_QUEUE);
				}
			}
			break;

		// 上报历史运动数据应答
		case BLE_CMD_REPORT_HISTORICAL_DATA_DOWN:
			// 关闭周期性发送启动历史数据同步函数
			TIMER_KillTask(TIMER_ID_BLE_START_HISTORICAL_DATA_UP);

			// 速度不为0，不允许上报历史数据
			if (PARAM_GetSpeed() == 0)
			{
				// 上传成功，刷新读取已发送历史数据值
				// 刷新已发送历史数据条数
				RECORD_RefreashSendCount();
				
				// 接着发下一条历史数据
				BLE_PROTOCOL_SendHistoryData(UINT32_NULL);

				// 历史数据没有同步完成，继续发送同步历史数据
				if(RECORD_IsHistoricalData())
				{
					// 启动同步历史数据，如果没有接收到应答，五秒中循环发送请求同步数据
					TIMER_AddTask(TIMER_ID_BLE_START_HISTORICAL_DATA_UP,
									5000,
									BLE_PROTOCOL_TxStartHistoryData,
									UINT32_NULL,
									TIMER_LOOP_FOREVER,
									ACTION_MODE_ADD_TO_QUEUE);
				}
			}
			break;
			
		default:
			break;
	}

	// 启动UART总线通讯超时判断
	BLE_PROTOCOL_StartTimeoutCheckTask();
	
	// 删除命令帧
	pCB->rx.head ++;
	pCB->rx.head %= BLE_PROTOCOL_RX_QUEUE_SIZE;
}

void BLE_PROTOCOL_RecordSave(TIME_E time_t, RECORD_ITEM_TMP_CB *recordTmpCB_t, uint32 timeTmpStart_t)
{	
	uint16 i;
	//uint32 timeTmpStart;	// 临时起点时间
	uint8 timeTmpHour;		// 临时小时时间段
	uint32 startTimeAddr;
	RECORD_ITEM_CB track[24] = {0};
	
	uint16 totalCount;
	uint8 bBuff[2];
	
	// 读取已有记录条数和已读出条数
	IAP_FlashReadByteArray(RECORD_ADDR_TOTAL, bBuff, 2);
	totalCount = (uint16)(bBuff[1] << 8) + (uint16)bBuff[0];
	
	// 获取存储非授时记录起始地址
	startTimeAddr = RECORD_ADDR_DATA_START + totalCount * sizeof(RECORD_PARAM_CB);
	
	// 扫描查询暂存记录
	for (i = 0; i <= recordTmpCB_t->tirpNum ; i++)
	{
		// 校准小时时间
		recordTmpCB_t->recordItemTmp[i].recordparam.hour = (timeTmpStart_t + recordTmpCB_t->recordItemTmp[i].recordparam.min * 60)/3600;
		
		timeTmpHour = recordTmpCB_t->recordItemTmp[i].recordparam.hour;
		// 合并时间段数据
		track[timeTmpHour].param.calories += (uint16)recordTmpCB_t->recordItemTmp[i].recordparam.calories;
		
		track[timeTmpHour].param.dateTime.year = (uint8)(time_t.year - 2000);
		track[timeTmpHour].param.dateTime.month = time_t.month;
		track[timeTmpHour].param.dateTime.day = time_t.day;
		track[timeTmpHour].param.dateTime.hour = recordTmpCB_t->recordItemTmp[i].recordparam.hour;
		track[timeTmpHour].param.dateTime.min = (timeTmpStart_t + recordTmpCB_t->recordItemTmp[i].recordparam.min * 60)%60;
		track[timeTmpHour].param.dateTime.sec = 0;
		
		track[timeTmpHour].param.isTimeCheck = 0x01;
		
		if (track[timeTmpHour].param.speedMax < recordTmpCB_t->recordItemTmp[i].recordparam.speedMax)
		{
			track[timeTmpHour].param.speedMax = recordTmpCB_t->recordItemTmp[i].recordparam.speedMax;
		}
		
		track[timeTmpHour].param.timeSlot = timeTmpHour;
		
		track[timeTmpHour].param.trip += (uint16)recordTmpCB_t->recordItemTmp[i].recordparam.trip;
		
		track[timeTmpHour].param.tripTimer += recordTmpCB_t->recordItemTmp[i].recordparam.ridingTime;
	}
	
	// 将合并后的数据存入flash中
	for (i = 0; i < 24; i++)
	{
		// 里程大于0说明有骑行记录，需要保存起来
		if (track[i].param.trip > 0)
		{
			// 增加校验位
			track[i].param.packCheck = RECORD_Check(track[i].array, sizeof(RECORD_PARAM_CB) - 1);
			
			// 存入flash
			IAP_FLASH_WriteByteWithErase(startTimeAddr, (uint8*)track[i].array, sizeof(RECORD_PARAM_CB));
			startTimeAddr += sizeof(RECORD_PARAM_CB);
			// 刷新历史数据条数
			RECORD_RefreashSaveCount();
		}
	}
}

// RX通讯超时处理-单向
#if BLE_PROTOCOL_RX_TIME_OUT_CHECK_ENABLE
void BLE_PROTOCOL_CALLBACK_RxTimeOut(uint32 param)
{
	BLE_PROTOCOL_RX_CMD_FRAME* pCmdFrame = NULL;

	pCmdFrame = &bleProtocolCB.rx.cmdQueue[bleProtocolCB.rx.end];

	// 超时错误，将命令帧长度清零，即认为抛弃该命令帧
	pCmdFrame->length = 0;	// 2016.1.6增加
	// 删除当前的命令头，而不是删除已分析完的所有数据，因为数据中可能会有命令头
	bleProtocolCB.rxFIFO.head ++;
	bleProtocolCB.rxFIFO.head %= BLE_PROTOCOL_RX_FIFO_SIZE;
	bleProtocolCB.rxFIFO.currentProcessIndex = bleProtocolCB.rxFIFO.head;
}

// 停止Rx通讯超时检测任务
void BLE_PROTOCOL_StopRxTimeOutCheck(void)
{
	TIMER_KillTask(TIMER_ID_UART_RX_TIME_OUT_CONTROL);
}
#endif

#if BLE_PROTOCOL_TXRX_TIME_OUT_CHECK_ENABLE
// TXRX通讯超时处理-双向
void BLE_PROTOCOL_CALLBACK_TxRxTimeOut(uint32 param)
{
	
}

// 停止TxRX通讯超时检测任务
void BLE_PROTOCOL_StopTxRxTimeOutCheck(void)
{
	TIMER_KillTask(TIMER_ID_UART_TXRX_TIME_OUT_CONTROL);
}
#endif

// 发送命令回复
void BLE_PROTOCOL_SendCmdAck(uint8 ackCmd)
{
	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(ackCmd); 	
	BLE_PROTOCOL_TxAddData(0x00);	
	BLE_PROTOCOL_TxAddFrame();	
}

// 发送命令回复，带一个参数
void BLE_PROTOCOL_SendCmdParamAck(uint8 ackCmd, uint8 ackParam)
{
	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(ackCmd); 	
	BLE_PROTOCOL_TxAddData(0x01);	
	
	BLE_PROTOCOL_TxAddData(ackParam);	
	BLE_PROTOCOL_TxAddFrame();	
}

// 查询固件版本回复
void BLE_PROTOCOL_SendCmdFirmVersionAck(void)
{
	uint8 val1, val2, val3, val4;
	uint16 valTmp;

	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();
	
	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_FIRMVERSION_UP); 	
	BLE_PROTOCOL_TxAddData(0x07);	

	// 仪表型号编码
	BLE_PROTOCOL_TxAddData(paramCB.runtime.devVersion[0]);
	BLE_PROTOCOL_TxAddData(paramCB.runtime.devVersion[1]);
	BLE_PROTOCOL_TxAddData(paramCB.runtime.devVersion[2]);
	
	// 固件版本
	val1 = PARAM_GetDevVersionRemain();
	val2 = PARAM_Get_DevReleaseVersion();
	val3 = PARAM_Get_FwUpdateVersion();
	val4 = PARAM_Get_FwDebugReleaseVersion();
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);

	// 固件版本编号
	valTmp = PARAM_GetFwBinNum();
	val1 = (uint8)((valTmp >> 8) & 0xFF);
	val2 = (uint8)(valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	
	BLE_PROTOCOL_TxAddFrame();	
}

// 仪表状态信息回复
void BLE_PROTOCOL_SendCmdStatusAck(void)
{
	uint8 val1, val2, val3, val4;
	uint16 valTmp;
	uint32 valTemp;

	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_GETSTATUS_UP); 	
	BLE_PROTOCOL_TxAddData(0x16);	

	// 骑行总里程
	valTemp = paramCB.nvm.param.common.record.total.distance;
	val1 = (uint8)((valTemp >> 24) & 0xFF);
	val2 = (uint8)((valTemp >> 16) & 0xFF);
	val3 = (uint8)((valTemp >> 8) & 0xFF);
	val4 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);

	// 总时间
	valTemp = PARAM_GetTotalRidingTime();
	val1 = (uint8)((valTemp >> 24) & 0xFF);
	val2 = (uint8)((valTemp >> 16) & 0xFF);
	val3 = (uint8)((valTemp >> 8) & 0xFF);
	val4 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);

	// 总卡路里
	valTemp = PARAM_GetTotalCalories();
	val1 = (uint8)((valTemp >> 24) & 0xFF);
	val2 = (uint8)((valTemp >> 16) & 0xFF);
	val3 = (uint8)((valTemp >> 8) & 0xFF);
	val4 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);

	// 最大速度
	valTmp = paramCB.nvm.param.common.record.trip.speedMax;
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 平均速度
	valTmp = paramCB.nvm.param.common.record.trip.speedAverage;
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 电量百分比
	val1 = PARAM_GetBatteryPercent();
	BLE_PROTOCOL_TxAddData(val1);

	// 助力档位
	val1 = (uint8)PARAM_GetAssistLevel();
	BLE_PROTOCOL_TxAddData(val1);

	// 大灯状态
	val1 = (uint8)PARAM_GetLightSwitch();
	BLE_PROTOCOL_TxAddData(val1);

	// 自动关机时间
	val1 = PARAM_GetPowerOffTime();
	BLE_PROTOCOL_TxAddData(val1);

	// 限速值
	val1 = paramCB.nvm.param.common.speed.limitVal/10;
	BLE_PROTOCOL_TxAddData(val1);

	// 显示单位
	val1 = PARAM_GetUnit();
	BLE_PROTOCOL_TxAddData(val1);
	
	// 锁车状态
	BLE_PROTOCOL_TxAddData(0x00);

	// 仪表背光亮度
	val1 = PARAM_GetBrightness();
	BLE_PROTOCOL_TxAddData(val1);

	BLE_PROTOCOL_TxAddFrame();	
}

// 仪表控制命令回复
void BLE_PROTOCOL_SendCmdControlAck(uint8 controlCmd, uint8 paramH, uint8 paramL)
{
	uint8 val1,val2;
	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_CONTROL_UP); 	
	BLE_PROTOCOL_TxAddData(0x03);	

	switch (controlCmd)
	{
		case LIGHT:
		
			if(0x00 == paramL)
			{
				// 关灯
				PARAM_SetLightSwitch(FALSE);	
			}
			else
			{
				// 开灯
				PARAM_SetLightSwitch(TRUE);	
		
			}
			val1 = 0;
			val2 = paramL;
			
			// 大灯开启后，屏幕亮度变为最暗
			if (PARAM_GetLightSwitch())
			{
				BACK_LIGHT_Start(BACK_LIGHT_LEVEL_20_100);
			}
			// 大灯关闭后，恢复之前的背光等级
			else
			{
				BACK_LIGHT_Start((BACK_LIGHT_LEVEL_E)PARAM_GetBrightness());
			}
			break;
		
		case AUTO_SHUTDOWN_TIME:
		
			PARAM_SetPowerOffTime(paramL);

			TIMER_KillTask(TIMER_ID_AUTO_POWER_OFF);
		
			// 重置自动关机任务
			STATE_ResetAutoPowerOffControl();
		
			// NVM更新，立即写入
			NVM_SetDirtyFlag(TRUE);
			val1 = 0;
			val2 = paramL;
			break;
		
		case SPEED_LIMIT:
		
			PARAM_SetSpeedLimit((uint16)paramL * 10);

			// NVM更新，立即写入
			NVM_SetDirtyFlag(TRUE);
			val1 = 0;
			val2 = paramL;
			
			break;
		
		case UNIT:
		
			PARAM_SetUnit((UNIT_E)paramL);

			// NVM更新，立即写入
			NVM_SetDirtyFlag(TRUE);
			val1 = 0;
			val2 = paramL;
			break;
		
		case ASSIST:
			
			// 获取当前助力值
			val1 = (uint8)paramCB.runtime.assist;
		
			if(0 == paramL)
			{
				if(val1 > 0)
				{
					val1--;
					PARAM_SetAssistLevel((ASSIST_ID_E)val1);
				}
			}
			else
			{
				if(val1 < PARAM_GetMaxAssist())
				{
					val1++;
					PARAM_SetAssistLevel((ASSIST_ID_E)val1);
				}
			}
			val1 = 0;
			val2 = (uint8)paramCB.runtime.assist;
			break;
		
		case LOCK:
			val1 = 0;
			val2 = 0;
			break;

		case BACKLIGHT:
		
			// 更改背光亮度
			BACK_LIGHT_Start((BACK_LIGHT_LEVEL_E)paramL);
			PARAM_SetBrightness((BACK_LIGHT_LEVEL_E)paramL);
			
			// NVM更新，立即写入
			NVM_SetDirtyFlag(TRUE);
			val1 = 0;
			val2 = paramL;
			break;
		default:
			break;
	}
	BLE_PROTOCOL_TxAddData(controlCmd);	
	BLE_PROTOCOL_TxAddData(val1);	
	BLE_PROTOCOL_TxAddData(val2);

	BLE_PROTOCOL_TxAddFrame();	
}

// 仪表档位消息上报
void BLE_PROTOCOL_SendAssistInfor(uint8 param)
{
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_CONTROL_UP); 	
	BLE_PROTOCOL_TxAddData(0x03);	
	
	BLE_PROTOCOL_TxAddData(ASSIST);	
	BLE_PROTOCOL_TxAddData(0x00);	
	BLE_PROTOCOL_TxAddData((ASSIST_ID_E)param);

	BLE_PROTOCOL_TxAddFrame();	
}


// 仪表获取实时运动状态回复
void BLE_PROTOCOL_SendCmdGetRuningStateAck(void)
{
	uint8 val1, val2, val3, val4;
	uint16 valTmp;
	uint32 valTemp;

	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_GETDATA_RUNNING_UP); 	
	BLE_PROTOCOL_TxAddData(0x0B);	

	// 实时功率
	valTmp = PARAM_GetPower();
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 额定功率
	valTmp = PARAM_GetPowerMax();
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 助力档位
	val1 = (uint8)PARAM_GetAssistLevel();
	BLE_PROTOCOL_TxAddData(val1);

	// 实时速度
	valTmp = paramCB.runtime.speed;
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 总里程
	valTemp = paramCB.nvm.param.common.record.total.distance;
	val1 = (uint8)((valTemp >> 24) & 0xFF);
	val2 = (uint8)((valTemp >> 16) & 0xFF);
	val3 = (uint8)((valTemp >> 8) & 0xFF);
	val4 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);

	BLE_PROTOCOL_TxAddFrame();	
}


// 仪表上报历史数据
void BLE_PROTOCOL_SendHistoryData(uint32 param)
{
	uint8 val1, val2, val3, val4;
	uint16 valTmp;
	uint32 valTemp;
	TIME_E timeTemp;
	
	RECORD_ITEM_CB record;

	if(!RECORD_ReadFromNVM(&record))
	{
		return;
	}

	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_REPORT_HISTORICAL_DATA_UP); 	
	BLE_PROTOCOL_TxAddData(0x0A);	

	// 运动日期,精确到天即可
	timeTemp.year = (uint16)record.param.dateTime.year + 2000;
	timeTemp.month = record.param.dateTime.month;
	timeTemp.day= record.param.dateTime.day;
	timeTemp.hour= record.param.dateTime.hour;
	timeTemp.minute = record.param.dateTime.min;
	timeTemp.second = record.param.dateTime.sec;
	valTemp = DS1302_GetTimeStampFromDateTime(timeTemp) - 8 * 3600;//Unix时间戳-8H 上报给服务器，转换为北京时间
	val1 = (uint8)((valTemp >> 24) & 0xFF);
	val2 = (uint8)((valTemp >> 16) & 0xFF);
	val3 = (uint8)((valTemp >> 8) & 0xFF);
	val4 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);
	
	// 运动时长
	val1 = (uint8)(record.param.tripTimer >> 8);
	val2 = (uint8)(record.param.tripTimer & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 最高速度
	valTmp = record.param.speedMax;
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 卡路里
	valTmp = record.param.calories;
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 所属时间段
	val1 = record.param.timeSlot;
	BLE_PROTOCOL_TxAddData(val1);

	// 所属时间段里程
	valTemp = record.param.trip;
	val1 = (uint8)((valTemp >> 8) & 0xFF);
	val2 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	BLE_PROTOCOL_TxAddFrame();	
}

// 发送获取MAC地址
void BLE_PROTOCOL_SendGetMacAddr(uint32 param)
{
	uint8 bleMacStr[] = "TTM:MAC-?";
	
	// 使能BLE发送
	BLE_BRTS_TX_REQUEST();

	// 将待发送命令添加到物理层缓冲区
	BLE_UART_AddTxArray(0xFFFF, bleMacStr, sizeof(bleMacStr));
}


// 仪表状态组1信息回复
void BLE_PROTOCOL_SendCmdSelectStatus1Ack(void)
{
	uint8 val1, val2, val3, val4;
	uint16 valTmp;
	uint32 valTemp;

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_SELECT_GETSTATUS_UP); 	
	BLE_PROTOCOL_TxAddData(0x16);

	// 状态组1序号
	BLE_PROTOCOL_TxAddData(0x01);	

	// 骑行总里程
	valTemp = paramCB.nvm.param.common.record.total.distance;
	val1 = (uint8)((valTemp >> 24) & 0xFF);
	val2 = (uint8)((valTemp >> 16) & 0xFF);
	val3 = (uint8)((valTemp >> 8) & 0xFF);
	val4 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);

	// 总时间
	valTemp = PARAM_GetTotalRidingTime();
	val1 = (uint8)((valTemp >> 24) & 0xFF);
	val2 = (uint8)((valTemp >> 16) & 0xFF);
	val3 = (uint8)((valTemp >> 8) & 0xFF);
	val4 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);

	// 总卡路里
	valTemp = PARAM_GetTotalCalories();
	val1 = (uint8)((valTemp >> 24) & 0xFF);
	val2 = (uint8)((valTemp >> 16) & 0xFF);
	val3 = (uint8)((valTemp >> 8) & 0xFF);
	val4 = (uint8)(valTemp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);
	BLE_PROTOCOL_TxAddData(val3);
	BLE_PROTOCOL_TxAddData(val4);

	// 最大速度
	valTmp = paramCB.nvm.param.common.record.trip.speedMax;
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 平均速度
	valTmp = paramCB.nvm.param.common.record.trip.speedAverage;
	val1 = ((valTmp >> 8) & 0xFF);
	val2 = (valTmp & 0xFF);
	BLE_PROTOCOL_TxAddData(val1);
	BLE_PROTOCOL_TxAddData(val2);

	// 电量百分比
	val1 = PARAM_GetBatteryPercent();
	BLE_PROTOCOL_TxAddData(val1);

	// 助力档位
	val1 = (uint8)PARAM_GetAssistLevel();
	BLE_PROTOCOL_TxAddData(val1);

	// 大灯状态
	val1 = (uint8)PARAM_GetLightSwitch();
	BLE_PROTOCOL_TxAddData(val1);

	// 显示单位
	val1 = PARAM_GetUnit();
	BLE_PROTOCOL_TxAddData(val1);
	
	// 锁车状态
	BLE_PROTOCOL_TxAddData(0x00);

	BLE_PROTOCOL_TxAddFrame();	
}

// 仪表状态组2信息回复
void BLE_PROTOCOL_SendCmdSelectStatus2Ack(void)
{
	uint8 val1;

	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_SELECT_GETSTATUS_UP); 	
	BLE_PROTOCOL_TxAddData(0x0E);	
	
	// 状态组2序号
	BLE_PROTOCOL_TxAddData(0x02);	

	// 仪表背光亮度
	val1 = PARAM_GetBrightness();
	BLE_PROTOCOL_TxAddData(val1);
	
	// 自动关机时间
	val1 = PARAM_GetPowerOffTime();
	BLE_PROTOCOL_TxAddData(val1);

	// 限速值
	val1 = paramCB.nvm.param.common.speed.limitVal/10;
	BLE_PROTOCOL_TxAddData(val1);
	
	// 启动方式
	BLE_PROTOCOL_TxAddData(0x01);
	
	// 巡航模式
	BLE_PROTOCOL_TxAddData(0x01);
	
	BLE_PROTOCOL_TxAddFrame();	
}

// 仪表状态组3信息回复
void BLE_PROTOCOL_SendCmdSelectStatus3Ack(void)
{
	int i = 0;
	
	BLE_PROTOCOL_TxAddData(BLE_PROTOCOL_CMD_HEAD); 		
	BLE_PROTOCOL_TxAddData(BLE_CMD_SELECT_GETSTATUS_UP); 	
	BLE_PROTOCOL_TxAddData(0x2A);	
	
	// 状态组3序号
	BLE_PROTOCOL_TxAddData(0x03);	

	// 以下均是参数填充
	// 电池序列号
	for (i = 0;i < 32;i++)
	{
		BLE_PROTOCOL_TxAddData(0);
	}
	
	// 电池充放电次数
	BLE_PROTOCOL_TxAddData(0);
	BLE_PROTOCOL_TxAddData(0);
	
	// 电池损耗率
	BLE_PROTOCOL_TxAddData(0);
	BLE_PROTOCOL_TxAddData(0);

	// 设计容量
	BLE_PROTOCOL_TxAddData(0);
	BLE_PROTOCOL_TxAddData(0);

	// 满充容量
	BLE_PROTOCOL_TxAddData(0);
	BLE_PROTOCOL_TxAddData(0);

	// 剩余容量
	BLE_PROTOCOL_TxAddData(0);
	BLE_PROTOCOL_TxAddData(0);
	
	BLE_PROTOCOL_TxAddFrame();	
}

uint8 MAC_ASCII_To_Byte(uint8 param1, uint8 param2)
{
	uint8 ret1 = 0;
	uint8 ret2 = 0;
	
	if ((param1 >= '0') && (param1 <= '9'))
	{
		ret1 = ((param1 - '0') << 4)&0xF0;
	}
	else if ((param1 >= 'A') && (param1 <= 'Z'))
	{
		ret1 = ((param1 - 'A' + 10) << 4)&0xF0;
	}
	
	if ((param2 >= '0') && (param2 <= '9'))
	{
		ret2 = (param2 - '0')&0x0F;
	}
	else  if ((param2 >= 'A') && (param2 <= 'Z'))
	{
		ret2 = (param2 - 'A' + 10)&0x0F;
	}
	
	return (ret1 + ret2);
}

// crc8
uint8 okay_get_checksum(uint8 * ptr, uint8 len)
{
	uint8 crc; 
	uint8 i; 
	crc = 0; 


	while(len--)
	{
		crc ^= *ptr++; 
		for(i=0; i<8; i++)
		{
			if(crc&0x01)
				crc = (crc>>1) ^ 0x8C;
			else 
				crc >>= 1;
		}
	}
	return crc;
}

// 发送广播数据包
void BLE_PROTOCOL_SendAdv(uint32 param)
{
//	uint8 i;
//	uint8 cc;
//	uint8 bleAdvStr[] = "TTM:ADD-";
	uint8 senBleAdvdBuf[24] = {0};
	
	if (0 == param)
	{
		// 广播连接数据包
		senBleAdvdBuf[0] = 'T';
		senBleAdvdBuf[1] = 'T';
		senBleAdvdBuf[2] = 'M';
		senBleAdvdBuf[3] = ':';
		senBleAdvdBuf[4] = 'A';
		senBleAdvdBuf[5] = 'D';
		senBleAdvdBuf[6] = 'D';
		senBleAdvdBuf[7] = '-';
		
		// MAC 地址
		senBleAdvdBuf[8] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[0], bleUartCB.rx.macBuff[1]);
		senBleAdvdBuf[9] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[3], bleUartCB.rx.macBuff[4]);
		senBleAdvdBuf[10] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[6], bleUartCB.rx.macBuff[7]);
		senBleAdvdBuf[11] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[9], bleUartCB.rx.macBuff[10]);
		senBleAdvdBuf[12] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[12], bleUartCB.rx.macBuff[13]);
		senBleAdvdBuf[13] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[15], bleUartCB.rx.macBuff[16]);
		
		//55 'A' '1' 01 00 00 25 35 07 55
		senBleAdvdBuf[14] = 0x55;
		senBleAdvdBuf[15] =  paramCB.nvm.param.protocol.carModel[0];
		senBleAdvdBuf[16] =  paramCB.nvm.param.protocol.carModel[1];
		senBleAdvdBuf[17] = '0';
		senBleAdvdBuf[18] = 0x00;
		senBleAdvdBuf[19] = 0x00;
		senBleAdvdBuf[20] = 0x25;
		//senBleAdvdBuf[21] = 0x35;
		senBleAdvdBuf[21] = okay_get_checksum(&senBleAdvdBuf[15],6);
		
		senBleAdvdBuf[22] = 0x07;
		senBleAdvdBuf[23] = 0x55;
		// 将待发送命令添加到物理层缓冲区
		BLE_UART_AddTxArray(0xFFFF, senBleAdvdBuf, 24);
	}
	else
	{
		// 复位广播数据包（非连接数据包）
		senBleAdvdBuf[0] = 'T';
		senBleAdvdBuf[1] = 'T';
		senBleAdvdBuf[2] = 'M';
		senBleAdvdBuf[3] = ':';
		senBleAdvdBuf[4] = 'A';
		senBleAdvdBuf[5] = 'D';
		senBleAdvdBuf[6] = 'D';
		senBleAdvdBuf[7] = '-';
		senBleAdvdBuf[8] = 0x18;
		senBleAdvdBuf[9] = 0x03;
		
		// MAC 地址
		senBleAdvdBuf[10] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[0], bleUartCB.rx.macBuff[1]);
		senBleAdvdBuf[11] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[3], bleUartCB.rx.macBuff[4]);
		senBleAdvdBuf[12] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[6], bleUartCB.rx.macBuff[7]);
		senBleAdvdBuf[13] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[9], bleUartCB.rx.macBuff[10]);
		senBleAdvdBuf[14] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[12], bleUartCB.rx.macBuff[13]);
		senBleAdvdBuf[15] = MAC_ASCII_To_Byte(bleUartCB.rx.macBuff[15], bleUartCB.rx.macBuff[16]);
		
		senBleAdvdBuf[16] = 0x01;
		senBleAdvdBuf[17] = 0x05;
		senBleAdvdBuf[18] = 0x00;
		senBleAdvdBuf[19] = 0x01;
		senBleAdvdBuf[20] = 0x00;
		senBleAdvdBuf[21] = 0x00;
		BLE_UART_AddTxArray(0xFFFF, senBleAdvdBuf, 22);
	}
}


#ifndef __IAP_H__
#define __IAP_H__
 
#include "common.h"

//===========================================================================================================================
// 用户务必根据自己所选芯片型号设置FLASH大小
#define IAP_N32_FLASH_SIZE 					(64*2048UL - IAP_PARAM_FLASH_SIZE)			// 所选MCU的FLASH容量大小(单位:字节)
#define IAP_N32_FLASH_BASE					0x08000000UL		// MCU的FLASH的起始地址
#define IAP_FLASH_ADRESS_OFFSET				0x00003000UL		// 预留12K为BOOT

#define IAP_FLASH_APP_ADDR					(IAP_N32_FLASH_BASE+IAP_FLASH_ADRESS_OFFSET)     // 第一个应用程序起始地址(存放在FLASH),保留IAP_FLASH_ADRESS_OFFSET的空间为IAP使用

// 以下宏由开发人员修改，用户不需操作
#define IAP_FLASH_PAGE_SIZE					2048UL				// 2K bytes

// BOOT信息地址
#define IAP_MCU_BOOT_ADDEESS 				(IAP_N32_FLASH_BASE + (64-1)*IAP_FLASH_PAGE_SIZE + 96)
// APP信息地址
#define IAP_MCU_APP_ADDEESS 				(IAP_N32_FLASH_BASE + (64-1)*IAP_FLASH_PAGE_SIZE + 128)

// 测试标识版本地址
#define IAP_MCU_TEST_FLAG_ADDEESS			(IAP_N32_FLASH_BASE + (64-1)*IAP_FLASH_PAGE_SIZE + 320)

// 标志区域
#define IAP_PARAM_FLASH_SIZE				2*2048UL

// IAP控制结构体定义
typedef struct
{
	BOOL appValid;			// APP数据是否可用

	volatile struct{
		uint32 sn0;				// MCU唯一序列号
		uint32 sn1;
		uint32 sn2;
	}mcuUID;
	
}IAP_CB;

extern IAP_CB iapCB;


//【外部调用函数声明】
void IAP_Init(void);

// 写入bin数据
BOOL IAP_WriteAppBin(uint32 appAddr, uint8 *appBuf, uint32 appLen);    

// 擦除APP所有页
BOOL IAP_EraseAPPArea(void);

// 跳转到APP
void IAP_JumpToAppFun(uint32 param);

//======================================================================================================
void IAP_FlashReadWordArray(uint32 readAddr, uint32 *pBuffer, uint16 numToRead);
BOOL IAP_FlashWriteWordArrayWithErase(uint32 writeAddr, uint32 *pBuffer, uint16 NumWordToWrite);
uint8 IAP_CmpareN(const uint8 *str1, const uint8 *str2, uint8 len);
void IAP_FLASH_WriteByteWithErase(uint32 writeAddr, uint8 *pBuffer, uint16 NumByteToWrite);
void IAP_FlashReadByteArray(uint32 readAddr, uint8 *pBuffer, uint16 numToRead);

#endif


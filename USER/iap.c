#include "common.h"
#include "iap.h"
#include "timer.h"
#include "uartdrive.h"
#include "dtconfig.h"

/*********************************************************
* 变量定义区
*********************************************************/
IAP_CB iapCB;

// FLASH读写操作
BOOL IAP_FlashWriteWordArray(uint32 writeAddr, uint32 *pBuffer, uint16 numToWrite);		//从指定地址开始写入指定长度的数据
void IAP_FlashReadWordArray(uint32 readAddr, uint32 *pBuffer, uint16 numToRead);   		//从指定地址开始读出指定长度的数据
BOOL IAP_FlashWriteWordArrayAndCheck(uint32 writeAddr, uint32 *pBuffer, uint16 numToWrite);
uint32 IAP_FlashReadWord(uint32 flashAddr);
// APP处理函数
void IAP_RunAPP(uint32 appAddr);

/*******************************************************************************
  * @函数名称	IAP_RunAPP
  * @函数说明   跳转到应用程序段
  * @输入参数   appAddr:应用程序的起始地址
  * @返回参数   无
*******************************************************************************/
void IAP_RunAPP(uint32 appAddr)
{
	//定义一个函数类型的参数	
	typedef void (*IAP_Func)(void);
	IAP_Func jumpToApp;

	if(((*(vu32*)appAddr) & 0x2FFE0000) == 0x20000000)	// 检查栈顶地址是否合法.
	{ 
		if (((*(vu32*)(appAddr+4)) & 0xFFF80000) != 0x08000000)   // APP无效
		{
			return;
		}

		// 跳转之前关总中断，复位外设为默认值
		DI();
//		RCC_EnableAPB2PeriphReset(0xFFFFFFFF, ENABLE);
//		RCC_EnableAPB2PeriphReset(0xFFFFFFFF, DISABLE);
//		RCC_EnableAPB1PeriphReset(0xFFFFFFFF, ENABLE);
//		RCC_EnableAPB1PeriphReset(0xFFFFFFFF, DISABLE);
		
		jumpToApp = (IAP_Func)*(vu32*)(appAddr+4);		// 用户代码区第二个字为程序开始地址(复位地址)		
	  	__set_MSP(*(vu32*)appAddr);						// 初始化用户程序的堆栈指针
		jumpToApp();									// 跳转到用户程序APP
	}
}

// 窗口时间到跳转到APP代码区运行
void IAP_JumpToAppFun(uint32 param)
{
	// 检查栈顶地址是否合法.
	if(((*(volatile uint32*)IAP_N32_FLASH_BASE) & 0x2FFE0000) != 0x20000000)	
	{
		return;
	}
	
	IAP_RunAPP(IAP_N32_FLASH_BASE);
}

uint8 IAP_CmpareN(const uint8 *str1, const uint8 *str2, uint8 len)
{
	uint16 i = 0;

	while(i < len)
	{
		if(*(str1 +i) != *(str2 +i))
		{
			return 0;
		}
		i++;
	}
	return 1;
}

/*******************************************************************************
  * @函数名称	IAP_Init
  * @函数说明   
  * @输入参数   
  * @返回参数   无
*******************************************************************************/
void IAP_Init(void)
{
	// 注意:擦写flash必须开启HSI时钟
	FLASH_ClockInit();					
}

/*******************************************************************************
  * @函数名称	IAP_WriteAppBin
  * @函数说明   将接收到的bin文件数据写入指定地址的FLASH当中
  * @输入参数   appAddr:应用程序的起始地址 appBuf：应用程序缓冲数据 appSize:应用程序数据大小（单位：字节）
  * @返回参数   无
*******************************************************************************/
BOOL IAP_WriteAppBin(uint32 appAddr, uint8 *appBuf, uint32 appSize)
{
	uint16 i = 0;
	uint16 j = 0;
	uint32 curAppAddr = appAddr; 	// 当前写入的地址
	uint32 *iapBuf = NULL;
	BOOL writeStatus;
	
	// 参数判断
	if (NULL == appBuf)
	{
		return FALSE;
	}

	// 参数检验是否非法，地址不能是IAP本身代码地址，绝对不能自己把自己擦掉
	if ((appAddr < IAP_FLASH_APP_ADDR) || ((appAddr + appSize) > (IAP_N32_FLASH_BASE+IAP_N32_FLASH_SIZE)))
	{
		return FALSE;		// 非法地址
	}

	// 开辟临时内存
	iapBuf = (uint32 *)malloc(1024);
	if (NULL == iapBuf)
	{
		return FALSE;
	}
	
	// 循环写入
	for (i = 0, j = 0; j < appSize; j += 4)
	{
		// 字节整合
		iapBuf[i++] = (uint32)(*appBuf) + ((uint32)(*(appBuf+1)) << 8) + ((uint32)(*(appBuf+2)) << 16) + ((uint32)(*(appBuf+3)) << 24);
		// 偏移4个字节
		appBuf += 4;
	}      
	
	// 直接写入FLASH
	writeStatus = IAP_FlashWriteWordArray(curAppAddr, iapBuf, appSize/4);	
	
	// 释放内存
	free(iapBuf);
	
	return writeStatus;
}

// 擦除APP所有页
BOOL IAP_EraseAPPArea(void)
{
	uint16 i;
	FLASH_STS status = FLASH_COMPL;
	BOOL eraseStatus = TRUE;
	
	//DI();				// 关闭总中断
    FLASH_Unlock();

    // 清除所有挂起标志位
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    // 擦除APP所有页
	for (i = (IAP_FLASH_ADRESS_OFFSET/IAP_FLASH_PAGE_SIZE); i < (IAP_N32_FLASH_SIZE/IAP_FLASH_PAGE_SIZE); i++)
	{
		status = FLASH_EraseOnePage(i*IAP_FLASH_PAGE_SIZE + IAP_N32_FLASH_BASE);

		if (FLASH_COMPL != status)
		{
			eraseStatus = FALSE;

			break;
		}
	}

	FLASH_Lock();		// 上锁
	//EI(); 				// 开启总中断

	return eraseStatus;
}
 
/*******************************************************************************
  * @函数名称	IAP_FlashReadHalfWord
  * @函数说明   读取指定地址的半字(16位数据)
  * @输入参数   faddr:FLASH当前地址
  * @返回参数   从当前FLASH地址开始，向后读取16个字节（半字）的数据：*(vu16*)faddr，注意地址必须为4的倍数
*******************************************************************************/
uint32 IAP_FlashReadWord(uint32 flashAddr)
{
	return *(volatile uint32*)flashAddr; 
}

/*******************************************************************************
  * @函数名称	IAP_FlashWriteHalfWordArrayAndCheck
  * @函数说明   向FLASH中写入指定长度的数据，辅助使用库函数IAP_WordProgram，每次写入字，并且加入了写入数据校验
  * @输入参数   writeAddr:应用程序的起始地址 pBuffer：整个扇区的内容 numToWrite:（单位：字）
  * @返回参数   BOOL,如果为TRUE,则该部分写入和校验成功，否则失败
----注意:此函数不能单独调用使用，因为没有解锁上锁-------------------------------
*******************************************************************************/
BOOL IAP_FlashWriteWordArrayAndCheck(uint32 writeAddr, uint32 *pBuffer, uint16 numToWrite)   
{ 			 		 
	uint16 i;
	uint32 readWord;

	// 参数判断
	if (NULL == pBuffer)
	{
		return FALSE;
	}

	// 参数检验是否非法
	if ((writeAddr < IAP_FLASH_APP_ADDR) || ((writeAddr + numToWrite) > (IAP_N32_FLASH_BASE+IAP_N32_FLASH_SIZE+IAP_PARAM_FLASH_SIZE)))
	{
		return FALSE;	// 非法地址
	}
	
	for (i = 0; i < numToWrite; i++)
	{
		FLASH_ProgramWord(writeAddr, pBuffer[i]);			// 写
		readWord = IAP_FlashReadWord(writeAddr);		// 读
	  	writeAddr += 4;

	  	// 写入FLASH中的缓冲区数据与读取的数据是否匹配
		if (readWord == pBuffer[i])
		{
			continue;
		}
		else
		{
			return FALSE;
		}
	}  
	return TRUE;
} 

/*******************************************************************************
  * @函数名称	IAP_FlashWriteWordArray
  * @函数说明   将指定大小的缓冲区数据按页写入FLASH当中
  * @输入参数   writeAddr:写入地址 pBuffer:缓冲区数据首地址 numToWrite:需要写入的长度（单位：字）
  * @返回参数   BOOL型的变量writeSucceed
*******************************************************************************/
BOOL IAP_FlashWriteWordArray(uint32 writeAddr, uint32 *pBuffer, uint16 numToWrite)	
{
	BOOL writeStatus = TRUE;
	
	// 参数判断
	if (NULL == pBuffer)
	{
		return FALSE;
	}

	// 参数检验是否非法
	if ((writeAddr < IAP_FLASH_APP_ADDR) || ((writeAddr + numToWrite) > (IAP_N32_FLASH_BASE+IAP_N32_FLASH_SIZE)))
	{
		return FALSE;	// 非法地址
	}
	
	//DI();				// 关闭总中断
	FLASH_Unlock();		// 解锁

	// 清除所有挂起标志位
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

	// 写入页
	if (!IAP_FlashWriteWordArrayAndCheck(writeAddr, pBuffer, numToWrite))	
	{
		writeStatus = FALSE;
	}

	FLASH_Lock();		// 上锁
	//EI(); 				// 开启总中断
	
	return writeStatus;
}

/*******************************************************************************
  * @函数名称	IAP_FlashReadHalfWordArray
  * @函数说明   从指定地址开始读出指定长度的数据
  * @输入参数   readAddr:读取地址 pBuffer:缓冲区数据首地址 numToWrite:需要读入的长度（单位：半字）
  * @返回参数   无
*******************************************************************************/
void IAP_FlashReadWordArray(uint32 readAddr, uint32 *pBuffer, uint16 numToRead)   	
{
	uint16 i;
	uint8 tmp;
	uint8 tmpBuf[4];
	uint32 *buf = NULL;

	// 参数判断
	if (NULL == pBuffer)
	{
		return;
	}

	// 参数检验是否非法
	if ((readAddr < IAP_FLASH_APP_ADDR) || ((readAddr + numToRead) > (IAP_N32_FLASH_BASE+IAP_N32_FLASH_SIZE+IAP_PARAM_FLASH_SIZE)))
	{
		return;	// 非法地址
	}
	
	// 判断是否4字节对齐，如果不是4字节对齐，则进行偏移处理
	if (readAddr % 4 != 0)
	{
		tmp = readAddr % 4;
		readAddr -= tmp; 
		
		// 获取原始数据
		for (i = 0; i < numToRead + 1; i++)
		{
			*(buf + i) = IAP_FlashReadWord(readAddr);
			readAddr += 4;
		}
		// 重新组合
		for (i = 0; i < numToRead; i++)
		{
			if (tmp == 1)
			{
				tmpBuf[0] = (uint8)(buf[i] >> 8);
				tmpBuf[1] = (uint8)(buf[i] >> 16);
				tmpBuf[2] = (uint8)(buf[i] >> 24);
				tmpBuf[3] = (uint8)buf[i + 1];
			}
			else if (tmp == 2)
			{
				tmpBuf[0] = (uint8)(buf[i] >> 16);
				tmpBuf[1] = (uint8)(buf[i] >> 24);
				tmpBuf[2] = (uint8)buf[i + 1];
				tmpBuf[3] = (uint8)(buf[i + 1] >> 8);
			}
			else if (tmp == 3)
			{
				tmpBuf[0] = (uint8)(buf[i] >> 24);
				tmpBuf[1] = (uint8)(buf[i + 1] >> 8);
				tmpBuf[2] = (uint8)(buf[i + 1] >> 16);
				tmpBuf[3] = (uint8)(buf[i + 1] >> 24);
			}

			pBuffer[i] = (uint32)tmpBuf[0] + (uint32)(tmpBuf[1] << 8) \
						+ (uint32)(tmpBuf[2] << 16) + (uint32)(tmpBuf[3] << 24);
		}
	}
	else
	{
		for (i = 0; i < numToRead; i++)
		{
			pBuffer[i] = IAP_FlashReadWord(readAddr);	// 读取4个字节.
			readAddr += 4;								// 偏移4个字节.	
		}	
	}
}

/*******************************************************************************
  * @函数名称	IAP_FlashReadHalfByteArray
  * @函数说明   从指定地址开始读出指定长度的数据
  * @输入参数   readAddr:读取地址 pBuffer:缓冲区数据首地址 numToWrite:需要读入的长度（单位：半字）
  * @返回参数   无
*******************************************************************************/
void IAP_FlashReadByteArray(uint32 readAddr, uint8 *pBuffer, uint16 numToRead) 
{
	
	uint16 i;
	uint8 offsetTmp;
	uint32 readBuf = 0;
	uint8 *tmpBuf = NULL;
	
	// 参数判断
	if (NULL == pBuffer)
	{
		return;
	}

	// 参数检验是否非法
	if ((readAddr < IAP_FLASH_APP_ADDR) || ((readAddr + numToRead * 4) > (IAP_N32_FLASH_BASE+IAP_N32_FLASH_SIZE+IAP_PARAM_FLASH_SIZE)))
	{
		return;	// 非法地址
	}

	tmpBuf = (uint8 *)malloc((numToRead/4 + 2)*4);

	// 判断是否4字节对齐，如果不是4字节对齐，则进行偏移处理
	if (readAddr % 4 != 0)
	{
		offsetTmp = readAddr % 4;
		readAddr -= offsetTmp; 

		// 获取原始数据
		for (i = 0; i < (numToRead - 1)/4 + 2; i++)
		{
			readBuf = IAP_FlashReadWord(readAddr);

			tmpBuf[4 * i] = (uint8)readBuf;
			tmpBuf[4 * i + 1] = (uint8)(readBuf >> 8);
			tmpBuf[4 * i + 2] = (uint8)(readBuf >> 16);
			tmpBuf[4 * i + 3] = (uint8)(readBuf >> 24);

			readAddr += 4;
		}

		for (i = 0; i < numToRead ; i++)
		{
			pBuffer[i] = tmpBuf[i + offsetTmp];
		}

	}
	else
	{
		for (i = 0; i < (numToRead - 1)/4 + 1; i++)
		{
			readBuf = IAP_FlashReadWord(readAddr);		// 读取4个字节.
			
			tmpBuf[4 * i] = (uint8)readBuf;
			tmpBuf[4 * i + 1] = (uint8)(readBuf >> 8);
			tmpBuf[4 * i + 2] = (uint8)(readBuf >> 16);
			tmpBuf[4 * i + 3] = (uint8)(readBuf >> 24);
			
			readAddr += 4;								// 偏移4个字节.	
		}	

		for (i = 0; i < numToRead ; i++)
		{
			pBuffer[i] = tmpBuf[i];
		}
	}

	free(tmpBuf);
}

/*******************************************************************************
  * @函数名称	IAP_FlashWriteWordArrayWithErase
  * @函数说明   将指定大小的缓冲区数据按页写入FLASH当中
  * @输入参数   writeAddr:写入地址 pBuffer:缓冲区数据首地址 numToWrite:需要写入的长度（单位：字）
  * @返回参数   BOOL型的变量writeSucceed
*******************************************************************************/
BOOL IAP_FlashWriteWordArrayWithErase(uint32 writeAddr, uint32 *pBuffer, uint16 NumWordToWrite)	
{
	BOOL writeStatus = TRUE;
	uint32 pagePos = 0;						// 扇区地址
	uint16 pageOffsent = 0;					// 在扇区内的偏移
	uint16 pageRemain = 0;  				// 扇区剩余空间大小 
	uint32 offaddr;    						// 去掉0X08000000后的地址
 	uint16 i = 0;
 	uint32 * pFlashBuff = NULL;				// 保存读取扇区的数据
 	uint16 numByteToWrite = NumWordToWrite * 4;
	
	// 参数判断
	if (NULL == pBuffer|| (NULL == numByteToWrite))
	{
		return FALSE;
	}

	// 参数检验是否非法
	if ((writeAddr < IAP_FLASH_APP_ADDR) || ((writeAddr + numByteToWrite) > (IAP_N32_FLASH_BASE+IAP_N32_FLASH_SIZE + IAP_PARAM_FLASH_SIZE)))
	{
		return FALSE;	// 非法地址
	}

//	DI();				// 关闭总中断
	FLASH_Unlock();		// 解锁

	// 申请1024Byte空间保存页数据
	pFlashBuff = (uint32 *)malloc(IAP_FLASH_PAGE_SIZE);
	if (NULL == pFlashBuff)
	{
		return FALSE;
	}

	offaddr = writeAddr - IAP_N32_FLASH_BASE;		// 实际偏移地址
	pagePos = offaddr / IAP_FLASH_PAGE_SIZE;		// 页地址
	pageOffsent = offaddr % IAP_FLASH_PAGE_SIZE;	// 在页内偏移
	pageRemain = IAP_FLASH_PAGE_SIZE - pageOffsent;	// 页剩余空间大小

	if (numByteToWrite <= pageRemain)
	{
		pageRemain = numByteToWrite;				// 不大于该页范围
	}
	
	while(1)
	{
		// 读出整个页的内容
		IAP_FlashReadWordArray(pagePos*IAP_FLASH_PAGE_SIZE+IAP_N32_FLASH_BASE, pFlashBuff, IAP_FLASH_PAGE_SIZE/4);

		for (i = 0; i < pageRemain/4; i++)	// 校验数据
		{
			if (pFlashBuff[pageOffsent/4 + i] != 0xFFFFFFFF)
			{
				// 里面的数据不全是0xFFFFFFFF，说明需要擦除
				break;
			}
		}

		if (i < pageRemain/4)					// 需要擦除
		{
			// 擦除这个页
			FLASH_EraseOnePage(pagePos * IAP_FLASH_PAGE_SIZE + IAP_N32_FLASH_BASE);

			// 复制
			for (i = 0; i < pageRemain/4; i++)
			{
				pFlashBuff[i + pageOffsent/4] = pBuffer[i]; 
			}

			// 写入整页
			if (!IAP_FlashWriteWordArrayAndCheck(pagePos * IAP_FLASH_PAGE_SIZE + IAP_N32_FLASH_BASE, pFlashBuff, IAP_FLASH_PAGE_SIZE/4))	
			{
				// 释放数据缓存区
				free(pFlashBuff);
	
				writeStatus = FALSE;

				break;
				//return writeStatus;
			}
		}
		else 
		{
			// 写已经擦除了的,直接写入扇区剩余区间.
			if (!IAP_FlashWriteWordArrayAndCheck(writeAddr, pBuffer, pageRemain/4))	
			{
				// 释放数据缓存区
				free(pFlashBuff);
	
				writeStatus = FALSE;

				break;
				//return writeStatus;
			}
		}
		
		if (numByteToWrite == pageRemain)
		{
			break;									// 写入结束了
		}
		else										// 写入未结束
		{
			pagePos++;								// 扇区地址增1
			pageOffsent = 0;						// 偏移位置为0 	 

		   	pBuffer += (pageRemain/4);  			// 指针偏移
			writeAddr += pageRemain;				// 写地址偏移	   
		   	numByteToWrite -= pageRemain;			// 字节数递减
		   	
			if (numByteToWrite > IAP_FLASH_PAGE_SIZE)
			{
				pageRemain = IAP_FLASH_PAGE_SIZE;	// 下一个扇区还是写不完
			}
			else
			{
				pageRemain = numByteToWrite;		// 下一个扇区可以写完了	
			}
		}	
		
	}

	// 释放数据缓存区
	free(pFlashBuff);

	FLASH_Lock();		// 上锁
//	EI(); 				// 开启总中断
	
	return writeStatus;
}

/*********************************************************************
* 函   数  名: SPI_FLASH_WriteByteWithErase
* 函数入参: uint8* pBuffer: 数据存储区
			uint32 WriteAddr: 开始写入的地址(24bit)
			uint16 NumByteToWrite: 要写入的字节数(最大65535)	
* 函数出参: 无
* 返   回  值: void 
* 功能描述: 在指定地址开始写入指定长度的数据，该函数带擦除操作!	   
**********************************************************************/
void IAP_FLASH_WriteByteWithErase(uint32 writeAddr, uint8 *pBuffer, uint16 NumByteToWrite)   
{ 
	//BOOL writeStatus = TRUE;
	uint32 pagePos = 0;						// 扇区地址
	uint16 pageOffsent = 0;					// 在扇区内的偏移
	uint16 pageRemain = 0;  				// 扇区剩余空间大小 
	uint32 offaddr;    						// 去掉0X08000000后的地址
 	uint16 i = 0;
 	uint8 *pFlashBuff = NULL;				// 保存读取扇区的数据
 	uint16 numByteToWrite = NumByteToWrite;

	if ((NULL == pBuffer) || (0 == NumByteToWrite))
	{
		return;
	}
	
//	DI();				// 关闭总中断
	FLASH_Unlock();		// 解锁
	
	// 申请1024Byte空间保存扇区数据
	pFlashBuff = (uint8 *)malloc(IAP_FLASH_PAGE_SIZE);
	if (NULL == pFlashBuff)
	{
		return;
	}

	offaddr = writeAddr - IAP_N32_FLASH_BASE;		// 实际偏移地址
	pagePos = offaddr / IAP_FLASH_PAGE_SIZE;		// 页地址
	pageOffsent = offaddr % IAP_FLASH_PAGE_SIZE;	// 在页内偏移
	pageRemain = IAP_FLASH_PAGE_SIZE - pageOffsent;	// 页剩余空间大小

	if (NumByteToWrite <= pageRemain)
	{
		pageRemain = NumByteToWrite;				// 不大于1024个字节
	}
	
	while (1) 
	{	
		// 读出整个扇区的内容
		IAP_FlashReadByteArray(pagePos*IAP_FLASH_PAGE_SIZE+IAP_N32_FLASH_BASE, pFlashBuff, IAP_FLASH_PAGE_SIZE);
		
		// 擦除这个页
		FLASH_EraseOnePage(pagePos * IAP_FLASH_PAGE_SIZE + IAP_N32_FLASH_BASE);


		// 复制
		for (i = 0; i < pageRemain; i++)
		{
			pFlashBuff[i + pageOffsent] = pBuffer[i];	  
		}

			// 写入整页
		if (!IAP_FlashWriteWordArrayAndCheck(pagePos * IAP_FLASH_PAGE_SIZE + IAP_N32_FLASH_BASE, (uint32 *)pFlashBuff, IAP_FLASH_PAGE_SIZE/4))	
		{
			// 释放数据缓存区
			free(pFlashBuff);
	
			//writeStatus = FALSE;

			break;
			//return writeStatus;
		}

		
		if (NumByteToWrite == pageRemain)
		{
			break;									// 写入结束了
		}
		else										// 写入未结束
		{
			pagePos++;								// 扇区地址增1
			pageOffsent = 0;						// 偏移位置为0 	 

		   	pBuffer += pageRemain;  			// 指针偏移
			writeAddr += pageRemain;				// 写地址偏移	   
		   	numByteToWrite -= pageRemain;			// 字节数递减
		   	
			if (numByteToWrite > IAP_FLASH_PAGE_SIZE)
			{
				pageRemain = IAP_FLASH_PAGE_SIZE;	// 下一个扇区还是写不完
			}
			else
			{
				pageRemain = numByteToWrite;		// 下一个扇区可以写完了	
			}
		}	 
	}
	
	// 释放数据缓存区
	free(pFlashBuff);	
	
	FLASH_Lock();		// 上锁
//	EI(); 				// 开启总中断
}



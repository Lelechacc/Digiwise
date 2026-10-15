#ifndef 	__SYSTEM_H__
#define 	__SYSTEM_H__

#include "common.h"

#define	SYS_OSC 		48UL	// 系统时钟，单位:MHz

// 函数声明
void SYSTEM_Init(void);

// 系统总复位
void SYSTEM_Rst(uint32 param);

#endif


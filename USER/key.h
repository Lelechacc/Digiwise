#ifndef     __KEY_H__
#define     __KEY_H__

#include "keyMsgProcess.h"

#define KEY_FILTER_LEVEL_N				3	// 按键滤波等级，取值范围1-7

// 按键缓存
typedef struct
{
	uint8 buff[KEY_NAME_MAX];

	uint8 lastValue[KEY_NAME_MAX];
}KEY_CB;


void KEY_Init(void);

void KEY_CALLBACK_Scan(uint8* p8bitKeyValueArray);

#endif


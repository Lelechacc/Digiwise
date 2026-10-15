/******************************************************************************************************
* 【模 块 名】：keyMsgProcess
* 【包含文件】：keyMsgProcess.c  keyMsgProcess.h
* 【版本记录】：V0.0.0 - 2013.11.6 by Sun Quanjin
*                        支持按下、释放、长按、重复按下等消息的抛出与响应
*               V0.1.0 - 2017.5.2  by Sun Quanjin
*                        增加 CLICK 和 DBCLICK 两个消息
* 【使用方法】：需要特别注意的是，按键按下时对应的bit为0，释放后对应的bit为1
* 
******************************************************************************************************/
#include "common.h"
#include "keyMsgProcess.h"
#include "timer.h"

KEYMSG_CB keyMsgCB;

#if 0==KEY_MSG_USE_SAME_HOLD_THRESHOLD
// 每个按键的长按门限
const uint16 keyHoldThreshHold[KEY_NAME_MAX] = 
{
	// 常规触摸按键长按门限值
	1000,   	// KEY_NAME_ADD
	2000,   	// KEY_NAME_DEC
	2000,   	// KEY_NAME_POWER
	1000,   	// KEY_NAME_ADD_DEC
	// ---------------- 2026-10-15 15:32:37 reverse digit animation ----------------
	3000,		// KEY_NAME_ADD_POWER
	// --------------------------------------------------------------------------
	1000,		// KEY_NAME_ADD_LIGHT
	2000,		// KEY_NAME_DEC_POWER
};
#endif

#if 0==KEY_MSG_USE_SAME_REPEAT_TIME
// 每个按键的重复周期
const uint16 keyRepeatTime[KEY_NAME_MAX] = 
{
	// 常规触摸按键重复周期
	300,		// KEY_NAME_ADD
	300,		// KEY_NAME_DEC
	300,		// KEY_NAME_POWER
	300,		// KEY_NAME_ADD_DEC
	300,		// KEY_NAME_ADD_POWER
	0,			// KEY_NAME_ADD_LIGHT (previous default)
	300,		// KEY_NAME_DEC_POWER
};
#endif
/******************************************************************************
* 【内部接口声明】
******************************************************************************/
// 【接口名称】KEYMSG_ServiceDoNothing
// 【接口功能】空响应服务，凡是不需要响应的按键消息，必须注册为此服务
void KEYMSG_ServiceDoNothing(uint32 param);

// 【接口名称】KEYMSG_ScanProcess
// 【接口功能】按键扫描处理
void KEYMSG_ScanProcess(void);

// 【接口名称】KEYMSG_MsgProcess
// 【接口功能】按键消息处理
void KEYMSG_MsgProcess(void);

// 【接口名称】KEYMSG_ServiceProcess
// 【接口功能】按键服务处理
void KEYMSG_ServiceProcess(void);

// 【接口名称】KEYMSG_ThrowMsg
// 【接口功能】抛出按键消息，保存在消息环形队列中
uint8 KEYMSG_ThrowMsg(KEY_NAME_E keyName, KEY_MSG_E keyMsg);

#if 0==KEY_MSG_THROW_KEY_UP_AFTER_REPEAT	// KEY_UP 屏蔽位置位时，不抛出 KEY_UP 消息
// 使能或禁用指定按键的释放消息
void KEYMSG_MSG_EnableKeyUpMessage(KEY_NAME_E keyName, uint8 bOn);
#endif



// 【接口名称】
// 【函数名称】KEYMSG_ServiceDoNothing
// 【接口功能】按键消息处理模块初始化接口
// 【使用方法】空函数，什么也不做
void KEYMSG_ServiceDoNothing(uint32 param)
{
	return ;
}

// 【接口名称】模块初始化接口
// 【函数名称】KEYMSG_Init
// 【接口功能】按键消息处理模块初始化接口
// 【使用方法】在 main 中大循环之前调用，注意，在调用本接口之前，必须完成定时器模块初始化
void KEYMSG_Init(void)
{
	uint8 i,j;

	// 数据结构初始化
	for(i=0; i<KEY_NAME_MAX; i++)
	{
		keyMsgCB.keyMsgInfo[i].end = 0;
		for(j=0; j<KEY_INFO_SIZE; j++)
		{
			keyMsgCB.keyMsgInfo[i].stateCount[j] = KEY_STATE_MASK;
		}

		// 清除所有消息的屏蔽请求
		for(j=0; j<KEY_MSG_MAX; j++)
		{
			keyMsgCB.msgMaskRequest[i][j] = FALSE;
		}
	}
	for(i=0; i<sizeof(keyMsgCB.keyValue); i++)
	{
		keyMsgCB.keyValue[i] = 0xFF;
		#if 0==KEY_MSG_THROW_KEY_UP_AFTER_REPEAT
		keyMsgCB.keyUpMsgMaskBit[i] = 0x00;
		#endif
	}
	
	// 消息环形队列初始化
	keyMsgCB.msgQueue.head = 0;
	keyMsgCB.msgQueue.end  = 0;
	for(i=0; i<KEY_MSG_QUEUE_SIZE; i++)
	{
		keyMsgCB.msgQueue.node[i].keyName = KEY_NAME_NULL;
		keyMsgCB.msgQueue.node[i].keyMsg = KEY_MSG_NULL;
	}
	
	keyMsgCB.GroupKeyInvalidRequest =  FALSE;
	keyMsgCB.scanRequest = FALSE;
 
	// 按键消息服务初始化
	KEYMSG_StopAllService();
	
	// 按键刷新函数接口初始化
	keyMsgCB.KeyUpdate = NULL;
	
	// 启动按键扫描
	TIMER_AddTask(TIMER_ID_KEY_SCAN,
					KEY_SCAN_TIME,
					KEYMSG_CALLBACK_Scan,
					0,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_DO_AT_ONCE);
}

// 【接口名称】按键扫描回调函数注册接口
// 【函数名称】KEYMSG_RegisterKeyScanCallback
// 【接口功能】注册按键扫描回调函数接口
// 【使用方法】在按键读取模块中，创建一个形如下列形参的函数，将读取到的按键值从低字节开始放入指针所指区域，
//             并通过此接口向本模块注册，需要注意的是：按键按下为0，按键释放为1
void KEYMSG_RegisterKeyScanCallback(void(*fun)(uint8* p8bitKeyValueArray))
{
	keyMsgCB.KeyUpdate = fun;
}
uint8 keyuiui;
// 【接口名称】按键消息响应注册接口
// 【函数名称】KEYMSG_RegisterMsgService
// 【接口功能】按键消息注册接口
// 【使用方法】用于注册按键消息服务，执行之前无须先注销，可直接调用本接口改变消息服务，注册的服务函数的形式必须与本接口中形参一致
void KEYMSG_RegisterMsgService(KEY_NAME_E keyName, KEY_MSG_E keyMsg, void(*service)(uint32 param), uint32 param)
{
	// 参数合法性检验
	if(keyName >= KEY_NAME_MAX)
	{
		return ;
	}
	if(keyMsg >= KEY_MSG_MAX)
	{
		return ;
	}
	if(!service)
	{
		return ;
	}

	// 配置按键服务
	keyMsgCB.keyMsgInfo[keyName].service[keyMsg] = service;
	keyMsgCB.keyMsgInfo[keyName].param[keyMsg] = param;
}

// 【接口名称】停止消息响应接口
// 【函数名称】KEYMSG_StopService
// 【接口功能】注销按键消息服务
// 【使用方法】需要停止按键消息响应时，通过本接口停止对指定按键的指定消息的响应
void KEYMSG_StopService(KEY_NAME_E keyName, KEY_MSG_E keyMsg)
{
	// 参数合法性检验
	if(keyName >= KEY_NAME_MAX)
	{
		return ;
	}
	if(keyMsg >= KEY_MSG_MAX)
	{
		return ;
	}
	
	// 将对应按键的对应服务设置为空
	keyMsgCB.keyMsgInfo[keyName].service[keyMsg] = KEYMSG_ServiceDoNothing;
}

// 【接口名称】停止所有消息响应接口
// 【函数名称】KEYMSG_StopAllService
// 【接口功能】注销所有按键消息服务
// 【使用方法】停止所有按键所有消息的响应
void KEYMSG_StopAllService(void)
{
	uint8 i,j;
	
	// 按键消息服务初始化
	for(i=0; i<KEY_NAME_MAX; i++)
	{
		for(j=0; j<KEY_MSG_MAX; j++)
		{
			keyMsgCB.keyMsgInfo[i].service[j] = KEYMSG_ServiceDoNothing;
		}
	}
}

// 【接口名称】KEYMSG_CALLBACK_Scan
// 【接口功能】按键处理回调函数，在定时器控制下，被周期调用，实现按键扫描
void KEYMSG_CALLBACK_Scan(uint32 param)
{
	keyMsgCB.scanRequest = TRUE;
}

// 【接口名称】KEYMSG_ScanProcess
// 【接口功能】按键扫描处理
void KEYMSG_ScanProcess(void)
{
	uint8 i;
	KEYMSG_INFO_CB* pKeyInfo = NULL;
	uint16 currentKeyBit = 0;	// 当前按键状态，无论哪个按键，都需要把按键值移动到最高位，以便与状态队列中的数值进行比较
	
	uint16 currentCount = 0;	// 当前按键状态计数值
	uint16 currentState = 0;	// 当前按键状态
	uint16* pStateCount = NULL;	// 当前操作中的状态计数值

	if(!keyMsgCB.scanRequest)
	{
		return ;
	}
	
	keyMsgCB.scanRequest = FALSE;
	
	// 调用回调函数读取按键值，未注册回调函数时退出
	if(NULL == keyMsgCB.KeyUpdate)
	{
		return ;
	}
	(*keyMsgCB.KeyUpdate)(keyMsgCB.keyValue);
	
	// 对每个按键当前的状态进行处理，将按键状态放入各自的按键信息节点中
	for(i=0; i<KEY_NAME_MAX; i++)
	{
		// 获取当前按键信息结构体指针
		pKeyInfo = &keyMsgCB.keyMsgInfo[i];
		
		// 获取当前操作中的按键状态计数值指针
		pStateCount = &(pKeyInfo->stateCount[pKeyInfo->end]);
		
		// 获取当前按键状态
		currentState = (*pStateCount) & KEY_STATE_MASK;
		
		// 获取当前按键状态计数值
		currentCount = (*pStateCount) & (~KEY_STATE_MASK);
		
		// 获取当前读取到的按键状态，并移位
		currentKeyBit = keyMsgCB.keyValue[i/8]&(0x01<<(i%8));	// 屏蔽其它按键值，只保留当前按键值
		currentKeyBit <<= 7 + 8 - (i%8);					// 将按键值移位到16bit的最高位

		// 如果当前按键状态与上一次状态相同，则最后一个状态计数值递增
		if(currentKeyBit == currentState)
		{
			// 次数未溢出时，可以递增，否则不变
			if(currentCount < (KEY_STATE_MASK-1))
			{
				// 【计数值处理方法说明】
				// 计数值为0，说明当前计数值为1，并且已抛出边沿消息；
				// 计数值为1，说明当前计数值为1，但未抛出边沿消息；
				// 计数值 >1，代表实际的计数值
				// 因此遇到计数值为0时，说明前面已经抛出了边沿消息，计数值需要增加2，以避免重复抛出边沿消息
				if(0 == currentCount)
				{
					(*pStateCount) += 2;
				}
				else
				{
					(*pStateCount) ++;
				}
			}
		}
		// 当前按键状态与上一次状态不同，则新增状态，同时触发对应消息
		else
		{
			pKeyInfo->end ++;
			pKeyInfo->end %= KEY_INFO_SIZE;
			pKeyInfo->stateCount[pKeyInfo->end] = currentKeyBit + 1;	// 当前状态添加到队列末尾，并且计数值赋1
		}
	}
}

// 【接口名称】KEYMSG_MsgProcess
// 【接口功能】按键消息处理
void KEYMSG_MsgProcess(void)
{
	uint8 i;
	KEYMSG_INFO_CB* pKeyInfo = NULL;
	uint16 currentCount = 0;	// 当前按键状态计数值
	uint16 currentState = 0;	// 当前按键状态
	uint16* pStateCount = NULL;	// 当前操作中的状态计数值
	
	// 遍历当前的状态，根据需要抛出按键消息，添加到按键环形队列中
	// 本段用于DOWN / HOLD / REPEAT / UP四个消息的判断与抛出
	for(i=0; i<KEY_NAME_MAX; i++)
	{
		// 获取当前按键信息结构体指针
		pKeyInfo = &keyMsgCB.keyMsgInfo[i];
		
		// 获取当前操作中的按键状态计数值指针
		pStateCount = &(pKeyInfo->stateCount[pKeyInfo->end]);
		
		// 获取当前按键状态
		currentState = (*pStateCount) & KEY_STATE_MASK;
		
		// 获取当前按键状态计数值
		currentCount = (*pStateCount) & (~KEY_STATE_MASK);
		
		// 最高位为0，说明当前状态为按键按下
		if(0 == currentState)
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 当前状态计数值为1，说明当前状态为第一次出现，需要抛出按下消息
			if(1 == currentCount)
			{
			#if 1==KEY_MSG_SWITCH_DOWN
				// 抛出按键按下消息
				KEYMSG_ThrowMsg((KEY_NAME_E)i, KEY_MSG_DOWN);
			#endif
			#if 0==KEY_MSG_THROW_KEY_UP_AFTER_REPEAT	// KEY_UP 屏蔽位置位时，不抛出 KEY_UP 消息
				keyMsgCB.keyUpMsgMaskBit[i/8] &= (0x01<<(i%8));	// 按键按下时解除 KEY_UP 限制
			#endif
				// 计数值清零，即代表已抛出消息
				(*pStateCount) &= KEY_STATE_MASK;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 当前状态计数值大于1，则判断是否达到长按门限
			else if(currentCount > 1)
			{
				// 达到长按门限，抛出长按消息，注意这里的判断条件，不能用>，否则将不断抛出长按消息
			#if 0==KEY_MSG_USE_SAME_HOLD_THRESHOLD
				if(currentCount*KEY_SCAN_TIME == keyHoldThreshHold[i])	// 每个按键使用单独的长按门限
			#else
				if(currentCount*KEY_SCAN_TIME == KEY_HOLD_TIME) // 所有按键使用统一的长按门限
			#endif
				{
				#if 1==KEY_MSG_SWITCH_HOLD
				
					// 抛出按键长按消息
					KEYMSG_ThrowMsg((KEY_NAME_E)i, KEY_MSG_HOLD);

					// 当 + 键或 - 键长按时，屏蔽 UP 消息
					if((KEY_NAME_ADD == i)||(KEY_NAME_DEC == i)||(KEY_NAME_POWER == i))
					{
						KEYMSG_SetMaskRequest((KEY_NAME_E)i, KEY_MSG_UP);
					}
				#endif
				#if 0==KEY_MSG_THROW_KEY_UP_AFTER_REPEAT	// KEY_UP 屏蔽位置位时，不抛出 KEY_UP 消息
					keyMsgCB.keyUpMsgMaskBit[i/8] |= (0x01<<(i%8));	// 按键长按时设置 KEY_UP 限制
				#endif
					// 计数值增加1，防止下一次重复抛出长按消息
					(*pStateCount)++;
				}
				// 超出长按门限，判断是否达到重复抛出门限
			#if 0==KEY_MSG_USE_SAME_HOLD_THRESHOLD
				else if(currentCount*KEY_SCAN_TIME >= keyHoldThreshHold[i] + keyRepeatTime[i])	// 每个按键使用单独的长按门限
			#else
				else if(currentCount*KEY_SCAN_TIME >= KEY_HOLD_TIME + KEY_RETHROW_TIME) // 所有按键使用统一的长按门限
			#endif
				{
				#if 1==KEY_MSG_SWITCH_REPEAT
					// 抛出按键重复消息
					KEYMSG_ThrowMsg((KEY_NAME_E)i, KEY_MSG_REPEAT);
				#endif
					
					// 将计数值设置为长按门限+1，为下次抛出重复消息做准备
					(*pStateCount) &= KEY_STATE_MASK;
				#if 0==KEY_MSG_USE_SAME_HOLD_THRESHOLD
					(*pStateCount) += (keyHoldThreshHold[i]/KEY_SCAN_TIME) + 1; // 每个按键使用单独的长按门限
				#else
					(*pStateCount) += (KEY_HOLD_TIME/KEY_SCAN_TIME) + 1;	// 所有按键使用统一的长按门限
				#endif
				}
				// 未达到长按门限，不予处理
				else
				{}
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 当前状态计数值为0，说明已抛出边沿消息，不再予以处理
			else
			{}
		}
		// 最高位为1，说明当前状态为按键释放
		else
		{
			// 当前状态计数值为1，说明当前状态为第一次出现，需要抛出释放消息
			if(1 == currentCount)
			{
			#if 1==KEY_MSG_SWITCH_UP
				// 抛出按键释放消息
				KEYMSG_ThrowMsg((KEY_NAME_E)i, KEY_MSG_UP);
			#endif
				
				// 计数值清零，即代表已抛出消息
				(*pStateCount) &= KEY_STATE_MASK;
			}
			// 计数值为0时，说明已抛出边沿消息，不再予以处理
			// 计数值>1时，为按键释放状态下计数，不予处理
			else
			{}
		}
	}
}

// 【接口名称】KEYMSG_ServiceProcess
// 【接口功能】按键服务处理
void KEYMSG_ServiceProcess(void)
{
	KEY_NAME_E keyName;
	KEY_MSG_E keyMsg;
	void (*service)(uint32 param) = 0;
	
	// 消息队列为空，退出
	if(keyMsgCB.msgQueue.head == keyMsgCB.msgQueue.end)
	{
		return ;
	}
	
	// 取出最早的消息执行
	keyName  = keyMsgCB.msgQueue.node[keyMsgCB.msgQueue.head].keyName;
	keyMsg = keyMsgCB.msgQueue.node[keyMsgCB.msgQueue.head].keyMsg;
	
	if(keyName >= KEY_NAME_MAX)
	{
		return ;
	}
	if(keyMsg >= KEY_MSG_MAX)
	{
		return ;
	}
	
	// 执行已注册的按键消息服务
	service = keyMsgCB.keyMsgInfo[keyName].service[keyMsg];
	(*service)(keyMsgCB.keyMsgInfo[keyName].param[keyMsg]);
	
	keyMsgCB.msgQueue.head ++;
	keyMsgCB.msgQueue.head %= KEY_MSG_QUEUE_SIZE;
}

// 【接口名称】按键处理接口
// 【函数名称】KEYMSG_Process
// 【接口功能】按键处理接口
// 【使用方法】在 main 中大循环内调用
void KEYMSG_Process(void)
{
	// 按键扫描处理
	KEYMSG_ScanProcess();
	
	// 按键消息处理
	KEYMSG_MsgProcess();
	
	// 按键服务处理
	KEYMSG_ServiceProcess();
}

// 【接口名称】KEYMSG_ThrowMsg
// 【接口功能】抛出按键消息，保存在消息环形队列中
uint8 KEYMSG_ThrowMsg(KEY_NAME_E keyName, KEY_MSG_E keyMsg)
{
	// 当前按键的当前消息有屏蔽请求，则不添加，并清除屏蔽请求
	if(keyMsgCB.msgMaskRequest[keyName][keyMsg])
	{
		keyMsgCB.msgMaskRequest[keyName][keyMsg] = FALSE;

		return 0;
	}

	// 有全局屏蔽请求，则退出
	if(keyMsgCB.globalMaskRequest)
	{
		return 0;
	}
	
	// 消息队列满，禁止添加
	if(((keyMsgCB.msgQueue.end+1)%KEY_MSG_QUEUE_SIZE) == keyMsgCB.msgQueue.head)
	{
#if 0==KEY_MSG_PRIORITY	// 旧消息优先时，队列满禁止添加
	  
		return 0;
		
#else	// 新消息优先时，队列满，删除最早的消息
		
		keyMsgCB.msgQueue.head ++;
		keyMsgCB.msgQueue.head %= KEY_MSG_QUEUE_SIZE;
#endif
	}
	
#if 0==KEY_MSG_THROW_KEY_UP_AFTER_REPEAT	// KEY_REPEAT 消息后禁止抛出 KEY_UP 消息
	// KEY_UP 屏蔽位置位时，不抛出 KEY_UP 消息，并且清除屏蔽位
	if((KEY_MSG_UP == keyMsg) && (keyMsgCB.keyUpMsgMaskBit[keyName/8] & (0x01<<(keyName%8))))
	{
		keyMsgCB.keyUpMsgMaskBit[keyName/8] &= (0x01<<(keyName%8));
	}
	else
	{
		// 添加到队列中
		keyMsgCB.msgQueue.node[keyMsgCB.msgQueue.end].keyName  = keyName;
		keyMsgCB.msgQueue.node[keyMsgCB.msgQueue.end].keyMsg = keyMsg;
		// 队尾后移
		keyMsgCB.msgQueue.end ++;
		keyMsgCB.msgQueue.end %= KEY_MSG_QUEUE_SIZE;
	}
#else	// 无论何种情况，按键释放时均抛出 KEY_UP 消息
	// 添加到队列中
	keyMsgCB.msgQueue.node[keyMsgCB.msgQueue.end].keyName  = keyName;
	keyMsgCB.msgQueue.node[keyMsgCB.msgQueue.end].keyMsg = keyMsg;
	
	// 队尾后移
	keyMsgCB.msgQueue.end ++;
	keyMsgCB.msgQueue.end %= KEY_MSG_QUEUE_SIZE;
#endif

	return 1;
}

#if 0==KEY_MSG_THROW_KEY_UP_AFTER_REPEAT	// KEY_UP 屏蔽位置位时，不抛出 KEY_UP 消息
// 使能或禁用指定按键的释放消息
void KEYMSG_MSG_EnableKeyUpMessage(KEY_NAME_E keyName, uint8 bOn)
{
	if(0 == bOn)
	{
		keyMsgCB.keyUpMsgMaskBit[keyName/8] |= 0x01<<(keyName%8);
	}
	else
	{
		keyMsgCB.keyUpMsgMaskBit[keyName/8] &= ~(0x01<<(keyName%8));
	}
}
#endif

// 设置指定按键的指定消息的屏蔽请求
void KEYMSG_SetMaskRequest(KEY_NAME_E keyName, KEY_MSG_E keyMsg)
{
	keyMsgCB.msgMaskRequest[keyName][keyMsg] = TRUE;
}

// 清除指定按键的指定消息的屏蔽请求
void KEYMSG_ClearMaskRequest(KEY_NAME_E keyName, KEY_MSG_E keyMsg)
{
	keyMsgCB.msgMaskRequest[keyName][keyMsg] = FALSE;
}

// 设置全局屏蔽请求
void KEYMSG_SetGlobalMaskRequest(void)
{
	keyMsgCB.globalMaskRequest = TRUE;
}

// 清除全局屏蔽请求
void KEYMSG_ClearGlobalMaskRequest(void)
{
	keyMsgCB.globalMaskRequest = FALSE;
}

// 设置+-键弹起恢复请求
void KEYMSG_SetAddDecKeyUpRecoveryRequest(BOOL state)
{
	keyMsgCB.addDecKeyUpRercoveryRequest = state;
}

// 获取+-键弹起恢复请求
BOOL KEYMSG_GetAddDecKeyUpRecoveryRequest(void)
{
	return keyMsgCB.addDecKeyUpRercoveryRequest;
}

// 设置+ i键弹起恢复请求
void KEYMSG_SetAddInfoKeyUpRecoveryRequest(BOOL state)
{
	keyMsgCB.addInfoKeyUpRercoveryRequest = state;
}

// 获取+ i键弹起恢复请求
BOOL KEYMSG_GetAddInfoKeyUpRecoveryRequest(void)
{
	return keyMsgCB.addInfoKeyUpRercoveryRequest;
}

// 设置组合键恢复请求
void KEYMSG_SetGroupKeyRecoveryRequest(BOOL state)
{
	keyMsgCB.GroupKeyInvalidRequest = state;
}

// 获取组合键恢复请求
BOOL KEYMSG_GetGroupKeyUpRecoveryRequest(void)
{
	return keyMsgCB.GroupKeyInvalidRequest;
}


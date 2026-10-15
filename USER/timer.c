/********************************************************************************
  * 文 件 名: timer.C
  * 版 本 号: 初版 V0.0 
  * 负责人 ：欧修宇          
*******************************************************************************/

/********************************* 头文件****************************************/
#include "timer.h"
#include "common.h"
// 软件定时器结构体
TIMER_CB timerCB;


/*********************************************************************
* 函 数 名: TIMER_InitTimerx
* 函数入参: 无
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 初始化定时器 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
void TIMx_INT_Init(uint16 arr, uint32 psc)
{
	TIM_TimeBaseInitType TIM_TimeBaseStructure;
	NVIC_InitType NVIC_InitStructure;


	// TIM6 clock enable   
	RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM6, ENABLE);
	
	// 定时器TIMx初始化
	TIM_DeInit(N32_TIMERx_NUM_DEF);						// 复位定时器x
	TIM_TimeBaseStructure.Period = arr;					// 设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.Prescaler = psc;				// 设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.ClkDiv = 0; 					// 设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.CntMode = TIM_CNT_MODE_UP; 	// TIM向上计数模式
	TIM_TimeBaseStructure.RepetCnt = 0;					// 重复次数为0
	TIM_InitTimeBase(N32_TIMERx_NUM_DEF, &TIM_TimeBaseStructure);		// 根据指定的参数初始化TIMx的时间基数单位	

	TIM_ClearFlag(N32_TIMERx_NUM_DEF, TIM_FLAG_UPDATE);					// 清除溢出中断标志
	// TIM2 enable update irq   
	TIM_ConfigInt(N32_TIMERx_NUM_DEF, TIM_INT_UPDATE, ENABLE);

	// 中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = N32_TIMERx_IRQn_DEF; 		// TIMx更新中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;		// 先占优先级2级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;				// 从优先级2级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 				// IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure); 
}

// 定时器x中断服务程序
void TIM6_IRQHandler(void)
{
	if (TIM_GetIntStatus(N32_TIMERx_NUM_DEF, TIM_INT_UPDATE) != RESET)    
	{
		// 清除TIMx更新中断标志  
		TIM_ClrIntPendingBit(N32_TIMERx_NUM_DEF, TIM_INT_UPDATE);
		
		// 定时器回调函数
		TIMER_CALLBACK_OnTimer();
	}
}

// 初始化定时器
void TIMER_InitTimerx(void)
{
	TIMx_INT_Init(9, 23999);		// (23999+1) / 48MHz * (9+1) =  5ms
}

// 启动硬件定时器
void TIMER_StartTimerx(void)
{
	// 使能TIMx
	TIM_Enable(N32_TIMERx_NUM_DEF, ENABLE);
}

// 停止硬件定时器
void TIMER_StopTimerx(void)
{
	TIM_Enable(N32_TIMERx_NUM_DEF, DISABLE);
}

void TIMER_DoNothing(uint32 param)
{}

void TIMER_Init(void)
{
	TIMER_ID i;

	// 虚拟定时器数据结构初始化
	for(i=(TIMER_ID)0; i<TIMER_ID_MAX; i++)
	{
		timerCB.count[i] = 0;
		timerCB.top[i] = 0;
		timerCB.action[i] = TIMER_DoNothing;
		timerCB.param[i] = 0;
		timerCB.loopCount[i] = 0;
		timerCB.active[i] = TRUE;
	}

	timerCB.queue.head = 0;
	timerCB.queue.end  = 0;
	
	// 硬件定时器x初始化
	TIMER_InitTimerx();
		
	// 启动定时器x
	TIMER_StartTimerx();
}


/*********************************************************************
* 函 数 名: TIMER_AddTask
* 函数入参: TIMER_ID id  定时器任务id, uint32 time 定时时间, void(*action)(uint32 param) 定时任务回调函数, 
			uint32 param 回调函数入参, int32 actionTime 执行次数, ACTION_MODE_E actionMode 执行模式
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 添加定时器任务 
*注意事项
	1.定时最小时间单位为5ms或10ms 设置定时任务时需要为最小定时时间的整数倍 已防止出现异常情况
	2.添加定时任务时 在之前调用删除定时任务删除同ID的任务
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/8/6
*     修改描述: 增加 action 判空 	
**********************************************************************/
BOOL TIMER_AddTask(TIMER_ID id, uint32 time, void(*action)(uint32 param), uint32 param, int32 actionTime, ACTION_MODE_E actionMode)
{
	// 执行次数为0，不添加
	if (0 == actionTime)
	{
		return FALSE;
	}
	
	// 查空
	if (NULL == action)
	{
		return FALSE;
	}

	timerCB.top[id] = time/TIMER_TIME;
	timerCB.action[id] = action;
	timerCB.param[id] = param;
	timerCB.loopCount[id] = actionTime-1;	// 动作次数减1，即为重复次数
	timerCB.count[id] = time/TIMER_TIME; 	// 这条赋值语句放到最后，可以保证在前面参数设置OK以后才启动软件定时器
	timerCB.actionMode[id] = actionMode;
	timerCB.active[id] = TRUE;
	
	return TRUE;
}

/*********************************************************************
* 函 数 名: TIMER_KillTask
* 函数入参: TIMER_ID id 定时器任务id
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 销毁定时器任务 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/8/6
*     修改描述: i 对 TIMER_TASK_QUEUE_SIZE 取余防止溢出	 
**********************************************************************/
void TIMER_KillTask(TIMER_ID id)
{
	uint8 i;
	
	if ((TIMER_ID_NULL == id) || (id >= TIMER_ID_MAX))
	{
		return;
	}
	
	// 计数值清零，即可将当前软件定时器释放
	timerCB.count[id] = 0; // 这条赋值语句放到最前，可以保证在第一时间停止软件定时器

	// 同时要删除已添加入任务队列的任务
	i = timerCB.queue.head;
	i %= TIMER_TASK_QUEUE_SIZE;
	
	while (i != timerCB.queue.end)
	{
		// 队列中有当前定时器对应的任务，就删除
		if((timerCB.queue.task[i].action == timerCB.action[id])&&
			(timerCB.queue.task[i].param == timerCB.param[id])&&
			(timerCB.queue.task[i].id == i))
		{
			timerCB.queue.task[i].action = TIMER_DoNothing;
		}
		
		i ++;
		i %= TIMER_TASK_QUEUE_SIZE;
	}
	
	timerCB.top[id] = 0;
	timerCB.action[id] = TIMER_DoNothing;
	timerCB.param[id] = 0;
	timerCB.loopCount[id] = 0;
	timerCB.active[id] = TRUE;
}


/*********************************************************************
* 函 数 名: TIMER_AddToQueue
* 函数入参:  回调函数，回调函数入参，定时器Id
* 函数出参: 是否添加成功
* 返 回 值: BOOL 
* 功能描述: 向定时器任务队列中添加一个任务 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
BOOL TIMER_AddToQueue(void(*action)(uint32 param), uint32 param, TIMER_ID id)
{
	// 队列满，返回失败信息
	if ((timerCB.queue.end + 1) % TIMER_TASK_QUEUE_SIZE == timerCB.queue.head)
	{
		return FALSE;
	}

	// 队列未满，添加
	timerCB.queue.task[timerCB.queue.end].action = action;
	timerCB.queue.task[timerCB.queue.end].param  = param;
	timerCB.queue.task[timerCB.queue.end].id  = id;
	timerCB.queue.end ++;
	timerCB.queue.end %= TIMER_TASK_QUEUE_SIZE;

	return TRUE;
}


/*********************************************************************
* 函 数 名: TIMER_TaskQueueProcess
* 函数入参: 无
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 定时器任务队列处理，在main中调用 ,取出定时任务执行队列中的任务并执行
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
void TIMER_TaskQueueProcess(void)
{
	void(*action)(uint32 param) = NULL;
	uint32 param;
	TIMER_TASK* pTask = NULL;
	
	// 队列空，退出
	if(timerCB.queue.head == timerCB.queue.end)
	{
		return;
	}

	// 队列非空，取出head处的任务执行之
	pTask = &timerCB.queue.task[timerCB.queue.head];
	action = pTask->action;
	param  = pTask->param;
	(*action)(param);

	// 删除任务
	timerCB.queue.head ++;
	timerCB.queue.head %= TIMER_TASK_QUEUE_SIZE;
}

/*********************************************************************
* 函 数 名: TIMER_CALLBACK_OnTimer
* 函数入参: 无
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 硬件定时器中断触发的回调函数,每五毫秒进入一次 依次扫码注册的定时任务，
            将对应任务的count值进行减少，为0时触发对应的执行模式 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
void TIMER_CALLBACK_OnTimer(void)
{
	uint8 i;
	for(i=0; i<TIMER_ID_MAX; i++)
	{
		// 计数值大于0，说明当前定时器处于活动状态，需要进行处理
		if(timerCB.count[i] > 0)
		{
			// 跳过被暂停的定时器
			if(!timerCB.active[i])
			{
				continue;
			}
			
			// 计数值递减
			timerCB.count[i] --;
			// 计数值为0，说明软件定时器时间到
			if(0 == timerCB.count[i])
			{
				// 如果是无限循环，则无条件重新设置计数初值
				if(timerCB.loopCount[i] < 0)
				{
					timerCB.count[i] = timerCB.top[i];
				}
				// 有限循环
				else if(timerCB.loopCount[i] > 0)
				{
					timerCB.count[i] = timerCB.top[i];
					
					// 剩余循环次数递减
					timerCB.loopCount[i] --;
				}
				// 循环次数为0，不予重复，当前软件定时器资源释放
				else
				{
				}
				
				// 执行模式为立即执行，则在中断中执行动作
				if(ACTION_MODE_DO_AT_ONCE == timerCB.actionMode[i])
				{
					(*timerCB.action[i])(timerCB.param[i]);
				}
				// 其它模式，添加到任务队列中，在中断外执行
				else
				{
					TIMER_AddToQueue(timerCB.action[i], timerCB.param[i] , (TIMER_ID)i);
				}
			}
		}
	}
}


/*********************************************************************
* 函 数 名: TIMER_Pause
* 函数入参: TIMER_ID id 定时任务id
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 暂停指定的定时器 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
void TIMER_Pause(TIMER_ID id)
{
	if((TIMER_ID_NULL == id)||(id >= TIMER_ID_MAX))
	{
		return;
	}

	timerCB.active[id] = FALSE;
}


/*********************************************************************
* 函 数 名: TIMER_Resume
* 函数入参: TIMER_ID id 定时任务id
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 恢复指定的定时器 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
void TIMER_Resume(TIMER_ID id)
{
	if((TIMER_ID_NULL == id)||(id >= TIMER_ID_MAX))
	{
		return;
	}

	timerCB.active[id] = TRUE;
}


/*********************************************************************
* 函 数 名: TIMER_KillAllTimer
* 函数入参: 无
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 注销所有定时器 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
void TIMER_KillAllTimer(void)
{
	TIMER_ID i;
		
	for(i=(TIMER_ID)0; i<TIMER_ID_MAX; i++)
	{
		timerCB.count[i] = 0;
		timerCB.loopCount[i] = 0;
		timerCB.queue.task[i].action = TIMER_DoNothing; 
	}
}


/*********************************************************************
* 函 数 名: TIMER_ResetTimer
* 函数入参: TIMER_ID id 定时任务id
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 重置定时器计数次数 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
void TIMER_ResetTimer(TIMER_ID id)
{
	if((TIMER_ID_NULL == id)||(id >= TIMER_ID_MAX))
	{
		return;
	}

	timerCB.count[id] = timerCB.top[id];
}


/*********************************************************************
* 函 数 名: TIMER_ChangeTime
* 函数入参: TIMER_ID id 定时任务id, uint32 time 修改的定时时间
* 函数出参: 是否成功更改
* 返 回 值: BOOL 
* 功能描述: 改变指定定时器定时时间 
***********
* 修改历史:
*   1.修改作者:欧修宇
*     修改日期: 2024/7/12
*     修改描述: 新函数 	
**********************************************************************/
BOOL TIMER_ChangeTime(TIMER_ID id, uint32 time)
{
	timerCB.top[id] = time/TIMER_TIME;
	timerCB.count[id] = time/TIMER_TIME;	
		
	return TRUE ;
}


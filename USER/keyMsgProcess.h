#ifndef 	__KEY_MSG_PROCESS_MODULE_H__
#define 	__KEY_MSG_PROCESS_MODULE_H__

/******************************************************************************
* 【用户定制参数区】
* 根据系统需要修改，务必严格按照注释内容设置参数
******************************************************************************/
// 包含必要的模块头文件

#define	KEY_SCAN_TIME			20		// 按键扫描周期，单位：ms，必须为虚拟定时器的分辨率的N倍(N>=1)
#define	KEY_HOLD_TIME			1500	// 按键长按门限，单位：ms，必须为扫描周期的N倍(N>=1)
#define	KEY_RETHROW_TIME		500		// 按键重复抛出周期，单位：ms，必须为扫描周期的N倍(N>=2)

// 按键名定义
typedef enum
{
	// 按键名称定义，可根据系统需要进行定制，但一定要保证在 KEY_NAME_MAX 之前添加
	// 按照每个按键所占的位，从低位向高位排列，首个按键枚举值必须为0
	KEY_NAME_ADD,
	KEY_NAME_DEC,
	KEY_NAME_POWER,
	KEY_NAME_ADD_DEC,
	KEY_NAME_ADD_POWER,
	KEY_NAME_ADD_LIGHT,
	KEY_NAME_DEC_POWER,

	/*=============================================================
	** 以下两项禁止修改，并且必须位于上述定义的最后
	==============================================================*/
	KEY_NAME_MAX,
	KEY_NAME_NULL,
}KEY_NAME_E;

// 用于记录按键状态的字节数
#define KEY_STATE_BYTE_COUNT							((KEY_NAME_MAX+7)/8)

// 按键判断周期等价的扫描周期数
#define KEY_JUDGE_COUNT								(KEY_JUDGE_TIME/KEY_SCAN_TIME)
// 按键掩码定义
typedef enum
{
	KEY_MASK_ADD = (1UL<<KEY_NAME_ADD),
	KEY_MASK_DEC = (1UL<<KEY_NAME_DEC),
	KEY_MASK_POWER = (1UL<<KEY_NAME_POWER),
	KEY_MASK_ADD_DEC = (1UL<<KEY_NAME_ADD_DEC),
	KEY_MASK_ADD_POWER = (1UL<<KEY_NAME_ADD_POWER),
	KEY_MASK_ADD_LIGHT = (1UL<<KEY_NAME_ADD_LIGHT),
	KEY_MASK_DEC_POWER = (1UL<<KEY_NAME_DEC_POWER),

	KEY_MASK_ALL = KEY_MASK_ADD | KEY_MASK_DEC | KEY_MASK_POWER |
		KEY_MASK_ADD_DEC | KEY_MASK_ADD_POWER | KEY_MASK_DEC_POWER,
	KEY_MASK_NULL = 0,
}KEY_MASK_E;


// ■■【注意】■■
#define	KEY_MSG_USE_SAME_HOLD_THRESHOLD			0	// 是否使用相同的长按触发门限，此项为0时，需要根据需要修改c文件中的门限常量数值
#define	KEY_MSG_USE_SAME_REPEAT_TIME			0	// 是否使用相同的重复周期，此项为0时，需要根据需要修改c文件中对应的常量数值

/******************************************************************************
* 【系统参数区】
* 注意：请不要修改这里的数值，除非对本模块的实现原理十分了解
******************************************************************************/
#define	KEY_INFO_SIZE					2		// 按键信息节点队列尺寸，即最多可保持几个按键状态
#define	KEY_STATE_MASK					0x8000	// 按键状态掩码
#define	KEY_MSG_QUEUE_SIZE				10		// 按键消息缓冲区尺寸，即最多可保持多少个消息，注意实际上可用元素为该值-1

#define	KEY_MSG_SWITCH_DOWN				1		// 消息响应开关，按键按下消息
#define KEY_MSG_SWITCH_HOLD				1		// 消息响应开关，按键长按消息
#define KEY_MSG_SWITCH_REPEAT			1		// 消息响应开关，按键重复消息
#define KEY_MSG_SWITCH_UP				1		// 消息响应开关，按键释放消息

/******************************************************************************
* 【系统定义区】
* 请不要修改这里的数值
******************************************************************************/
#ifndef	NULL
#define	NULL	0
#endif

#define	KEY_MSG_PRIORITY					1	// 消息队列满时处理：0 - 旧消息优先，自动抛弃最新消息；1 - 新消息优先，自动覆盖旧消息
#define	KEY_MSG_THROW_KEY_UP_AFTER_REPEAT	1	// 重复按键释放后是否抛出按键释放消息【此功能待完善，稳定之前不要把值改为0】

// 按键消息定义
typedef enum
{
	// 按键消息定制，最多支持四种消息
#if 1==KEY_MSG_SWITCH_DOWN
	KEY_MSG_DOWN,			// 按键触发边沿消息
#endif
#if 1==KEY_MSG_SWITCH_UP
	KEY_MSG_UP,				// 按键解除边沿消息
#endif
#if 1==KEY_MSG_SWITCH_HOLD
	KEY_MSG_HOLD,			// 按键长按消息
#endif
#if 1==KEY_MSG_SWITCH_REPEAT
	KEY_MSG_REPEAT,			// 按键重复消息，在按键长按之后周期性抛出
#endif
	
	/*=============================================================
	** 以下两项禁止修改，并且必须位于上述定义的最后
	==============================================================*/
	KEY_MSG_MAX,			// 按键消息个数
	KEY_MSG_NULL,			// 非法的消息
}KEY_MSG_E;


// 按键信息节点，按键按下时最高位0，按键释放时最高位为1，低7位为按键状态计数值，使用时需要注意最大记录次数
typedef struct
{
	uint8	end;
	uint16	stateCount[KEY_INFO_SIZE];				// 每个元素最高位为按键状态，0为按下，1为释放，其余位为记录次数
	void	(*service[KEY_MSG_MAX])(uint32 param);	// 按键响应
	uint32	param[KEY_MSG_MAX];						// 按键响应服务的参数
}KEYMSG_INFO_CB;

// 按键消息队列，用于保存抛出的消息
typedef struct
{
	uint8	head;
	uint8	end;
	struct
	{
		KEY_NAME_E	keyName;	// 按键名
		KEY_MSG_E	keyMsg;		// 消息
	}node[KEY_MSG_QUEUE_SIZE];
}KEY_MSG_QUEUE;

// 按键控制结构体
typedef struct
{
	volatile uint8	scanRequest;					// 按键扫描请求标志
	
	uint8	keyValue[KEY_STATE_BYTE_COUNT];			// 按键值，定时更新，每个bit代表一个按键状态，
													// 每个按键对应的位由 KEY_NAME_E 中的定义决定
													// 1-8个按键时，需要1个字节表达
													// 9-16个按键时，需要2个字节表达
													// 因此，该按键值序列长度计算公式为 (KEY_NAME_MAX + 7)/8
#if 0==KEY_MSG_THROW_KEY_UP_AFTER_REPEAT			// 长按后抛出 KEY_REPEAT 消息后，如果不需要抛出按键释放消息，需要有标志位记录
	uint8	keyUpMsgMaskBit[KEY_STATE_BYTE_COUNT];	// 参考上面的说明
													// 标志位为1，下一次禁止抛出 KEY_UP 消息，
													// 标志位为0，不限制 KEY_UP 消息
#endif
	KEYMSG_INFO_CB	keyMsgInfo[KEY_NAME_MAX];		// 按键信息节点
	KEY_MSG_QUEUE	msgQueue;						// 按键消息缓冲区
	
	void(*KeyUpdate)(uint8* p8bitKeyValueArray);	// 按键刷新函数，需要使用者提供回调函数接口，在回调函数中为按键赋值

	// 消息屏蔽请求
	BOOL msgMaskRequest[KEY_NAME_MAX][KEY_MSG_MAX];
	BOOL globalMaskRequest;							// 全局屏蔽请求，此请求置位后，必须要检测到按键全部释放后才能清除，否则不抛出任何消息

	BOOL addDecKeyUpRercoveryRequest;				// +-键弹起恢复请求
	BOOL addInfoKeyUpRercoveryRequest;				// + i键弹起恢复请求
	BOOL addKeyInvalidRequest;						// +键无效请求
	BOOL GroupKeyInvalidRequest;					// 组合键无效请求
}KEYMSG_CB;

extern KEYMSG_CB keyMsgCB;


/******************************************************************************
* 【外部接口声明】
******************************************************************************/
// 【接口名称】模块初始化接口
// 【函数名称】KEYMSG_Init
// 【接口功能】按键消息处理模块初始化接口
// 【使用方法】在 main 中大循环之前调用，注意，在调用本接口之前，必须完成定时器模块初始化
void KEYMSG_Init(void);

// 【接口名称】按键扫描回调函数注册接口
// 【函数名称】KEYMSG_RegisterKeyScanCallback
// 【接口功能】注册按键扫描回调函数接口
// 【使用方法】在按键读取模块中，创建一个形如下列形参的函数，将读取到的按键值从低字节开始放入指针所指区域，
//			   并通过此接口向本模块注册，需要注意的是：按键按下为0，按键释放为1
void KEYMSG_RegisterKeyScanCallback(void(*fun)(uint8* p8bitKeyValueArray));

// 【接口名称】按键消息响应注册接口
// 【函数名称】KEYMSG_RegisterMsgService
// 【接口功能】按键消息注册接口
// 【使用方法】用于注册按键消息服务，执行之前无须先注销，可直接调用本接口改变消息服务，注册的服务函数的形式必须与本接口中形参一致
void KEYMSG_RegisterMsgService(KEY_NAME_E keyName, KEY_MSG_E keyMsg, void(*service)(uint32 param), uint32 param);

// 按键消息响应注册接口，可以设置长按门限
void KEYMSG_RegisterMsgServiceWithHoldTime(KEY_NAME_E keyName, KEY_MSG_E keyMsg, void(*service)(uint32 param), uint32 param, uint16 holdTime);


// 【接口名称】停止消息响应接口
// 【函数名称】KEYMSG_StopService
// 【接口功能】注销按键消息服务
// 【使用方法】需要停止按键消息响应时，通过本接口停止对指定按键的指定消息的响应
void KEYMSG_StopService(KEY_NAME_E keyName, KEY_MSG_E keyMsg);

// 【接口名称】停止所有消息响应接口
// 【函数名称】KEYMSG_StopAllService
// 【接口功能】注销所有按键消息服务
// 【使用方法】停止所有按键所有消息的响应
void KEYMSG_StopAllService(void);

// 【接口名称】定时刷新回调函数
// 【函数名称】KEYMSG_CALLBACK_Scan
// 【接口功能】按键处理回调函数，在定时器控制下，被周期调用，实现按键扫描
// 【使用方法】将此函数放入定时器中断响应函数中调用，注意定时器周期和按键扫描周期的关系
void KEYMSG_CALLBACK_Scan(uint32 param);

// 【接口名称】按键处理接口
// 【函数名称】KEYMSG_Process
// 【接口功能】按键处理接口
// 【使用方法】在 main 中大循环内调用
void KEYMSG_Process(void);

// 设置指定按键的指定消息的屏蔽请求
void KEYMSG_SetMaskRequest(KEY_NAME_E keyName, KEY_MSG_E keyMsg);

// 清除指定按键的指定消息的屏蔽请求
void KEYMSG_ClearMaskRequest(KEY_NAME_E keyName, KEY_MSG_E keyMsg);

// 设置全局屏蔽请求
void KEYMSG_SetGlobalMaskRequest(void);

// 清除全局屏蔽请求
void KEYMSG_ClearGlobalMaskRequest(void);

// 设置+-键弹起恢复请求
void KEYMSG_SetAddDecKeyUpRecoveryRequest(BOOL state);

// 获取+-键弹起恢复请求
BOOL KEYMSG_GetAddDecKeyUpRecoveryRequest(void);

// 设置+ i键弹起恢复请求
void KEYMSG_SetAddInfoKeyUpRecoveryRequest(BOOL state);

// 获取+ i键弹起恢复请求
BOOL KEYMSG_GetAddInfoKeyUpRecoveryRequest(void);

// 设置组合键恢复请求
void KEYMSG_SetGroupKeyRecoveryRequest(BOOL state);

// 获取组合键恢复请求
BOOL KEYMSG_GetGroupKeyUpRecoveryRequest(void);

#endif


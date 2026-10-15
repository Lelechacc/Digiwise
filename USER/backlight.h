#ifndef __BACK_LIGHT_H__
#define __BACK_LIGHT_H__

#define	BACK_LIGHT_ON()		do{\
							BACK_LIGHT_PWM(100);\
							}while(0)
#define	BACK_LIGHT_OFF()		do{\
							BACK_LIGHT_PWM(0);\
							}while(0)
#define	BACK_LIGHT_PWM(i)	do{\
							TIM_SetCmp1(TIM4, i);\
							TIM_EnableCtrlPwmOutputs(TIM4, ENABLE);\
							}while(0)

// LCD背光灯工作的模式
typedef enum
{
	BACK_LIGHT_LEVEL_OFF = 0,	// 关闭
//	BACK_LIGHT_LEVEL_5_100,
//	BACK_LIGHT_LEVEL_10_100,		// 10%
//	BACK_LIGHT_LEVEL_15_100,
	BACK_LIGHT_LEVEL_20_100,		// 20%
//	BACK_LIGHT_LEVEL_25_100,
//	BACK_LIGHT_LEVEL_30_100,		// 30%
//	BACK_LIGHT_LEVEL_35_100,
	BACK_LIGHT_LEVEL_40_100,		// 40%
//	BACK_LIGHT_LEVEL_45_100,
//	BACK_LIGHT_LEVEL_50_100,		// 50%
//	BACK_LIGHT_LEVEL_55_100,
	BACK_LIGHT_LEVEL_60_100,		// 60%
//	BACK_LIGHT_LEVEL_65_100,
//	BACK_LIGHT_LEVEL_70_100,		// 70%
//	BACK_LIGHT_LEVEL_75_100,
	BACK_LIGHT_LEVEL_80_100,		// 80%
//	BACK_LIGHT_LEVEL_85_100,
//	BACK_LIGHT_LEVEL_90_100,		// 90%
//	BACK_LIGHT_LEVEL_95_100,
	BACK_LIGHT_LEVEL_ON,			// 100%常开

	BACK_LIGHT_LEVEL_MAX
}BACK_LIGHT_LEVEL_E;

// 参数结构体
typedef struct
{
	BACK_LIGHT_LEVEL_E level;

	uint16 step;
			 
}BACK_LIGHT_CB;

extern BACK_LIGHT_CB backlightCB;


/******************************************************************************
* 【外部接口声明】
******************************************************************************/
// 初始化
void BACK_LIGHT_Init(void);

// 设置定时器，并指定工作模式
void BACK_LIGHT_Start(BACK_LIGHT_LEVEL_E level);


#endif


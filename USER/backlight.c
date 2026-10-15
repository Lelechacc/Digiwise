#include "common.h"
#include "backlight.h"
#include "timer.h"
#include "delay.h"

BACK_LIGHT_CB backlightCB;

/*********************************************************
* ■■■■■函数定义区■■■■■■■■■■■■■■■■■■
*********************************************************/
void BACK_LIGHT_Init(void)
{	
	GPIO_InitType GPIO_InitStructure;
	TIM_TimeBaseInitType TIM_TimeBaseStructure;
	OCInitType TIM_OCInitStructure;	
	
	RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM4, ENABLE);
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_AFIO, ENABLE);
	
	// GPIO配置
	GPIO_InitStruct(&GPIO_InitStructure);
  /* GPIOA Configuration:TIM3 Channel1, 2, 3 and 4 as alternate function push-pull */
  GPIO_InitStructure.Pin        = GPIO_PIN_6;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Current = GPIO_DC_4mA;
  GPIO_InitStructure.GPIO_Alternate = GPIO_AF2_TIM4;
  GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);

  /* Time base configuration */
  TIM_TimeBaseStructure.Period    = 99;
  TIM_TimeBaseStructure.Prescaler = 23;
  TIM_TimeBaseStructure.ClkDiv    = 0;
  TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;

  TIM_InitTimeBase(TIM4, &TIM_TimeBaseStructure);
	
	/* PWM1 Mode configuration: Channel2 */
  TIM_OCInitStructure.OcMode       = TIM_OCMODE_PWM1;
    TIM_OCInitStructure.OutputState = TIM_OUTPUT_STATE_ENABLE;
    TIM_OCInitStructure.Pulse       = 0;
    TIM_OCInitStructure.OcPolarity  = TIM_OC_POLARITY_HIGH;
  TIM_InitOc1(TIM4, &TIM_OCInitStructure);

  TIM_ConfigOc1Preload(TIM4, TIM_OC_PRE_LOAD_ENABLE);
	TIM_ConfigArPreload(TIM4, ENABLE);

  /* TIM4 enable counter */
  TIM_Enable(TIM4, ENABLE);
	TIM_EnableCtrlPwmOutputs(TIM4, ENABLE);

	// 初始化关掉背光
	BACK_LIGHT_Start(BACK_LIGHT_LEVEL_OFF);
}

// 启动定时器，并指定工作模式
void BACK_LIGHT_Start(BACK_LIGHT_LEVEL_E level)
{
	if (level >= BACK_LIGHT_LEVEL_MAX)
	{
		return;
	}

	// 新模式与当前工作中的模式相同，不予执行
	if (backlightCB.level == level)
	{
		return;
	}
	backlightCB.level = level;

	switch (level)
	{
		case BACK_LIGHT_LEVEL_OFF:
			BACK_LIGHT_OFF();
			break;

		case BACK_LIGHT_LEVEL_ON:
			BACK_LIGHT_ON();
			break;

	/*	case BACK_LIGHT_LEVEL_5_100:
			BACK_LIGHT_PWM(5);
			break;

		case BACK_LIGHT_LEVEL_10_100:
			BACK_LIGHT_PWM(10);
			break;

		case BACK_LIGHT_LEVEL_15_100:
			BACK_LIGHT_PWM(15);
			break;
*/
		case BACK_LIGHT_LEVEL_20_100:
			BACK_LIGHT_PWM(20);
			break;
/*
		case BACK_LIGHT_LEVEL_25_100:
			BACK_LIGHT_PWM(25);
			break;

		case BACK_LIGHT_LEVEL_30_100:
			BACK_LIGHT_PWM(30);
			break;

		case BACK_LIGHT_LEVEL_35_100:
			BACK_LIGHT_PWM(35);
			break;
*/
		case BACK_LIGHT_LEVEL_40_100:
			BACK_LIGHT_PWM(40);
			break;
/*
		case BACK_LIGHT_LEVEL_45_100:
			BACK_LIGHT_PWM(45);
			break;

		case BACK_LIGHT_LEVEL_50_100:
			BACK_LIGHT_PWM(50);
			break;

		case BACK_LIGHT_LEVEL_55_100:
			BACK_LIGHT_PWM(55);
			break;
*/
		case BACK_LIGHT_LEVEL_60_100:
			BACK_LIGHT_PWM(60);
			break;
/*
		case BACK_LIGHT_LEVEL_65_100:
			BACK_LIGHT_PWM(65);
			break;

		case BACK_LIGHT_LEVEL_70_100:
			BACK_LIGHT_PWM(70);
			break;

		case BACK_LIGHT_LEVEL_75_100:
			BACK_LIGHT_PWM(75);
			break;
*/
		case BACK_LIGHT_LEVEL_80_100:
			BACK_LIGHT_PWM(80);
			break;
/*
		case BACK_LIGHT_LEVEL_85_100:
			BACK_LIGHT_PWM(85);
			break;

		case BACK_LIGHT_LEVEL_90_100:
			BACK_LIGHT_PWM(90);
			break;

		case BACK_LIGHT_LEVEL_95_100:
			BACK_LIGHT_PWM(95);
			break;
*/			
		default:
			BACK_LIGHT_OFF();
			break;
	}
}



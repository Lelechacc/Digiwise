#include "common.h"
#include "System.h"
#include "delay.h"

// 内部函数声明
static void CLOCK_Configuration(void);
static void NVIC_Configuration(void);

// 系统硬件底层配置
void SYSTEM_Init(void)
{
	CLOCK_Configuration();

	// 更新内核运行时钟变量
	SystemCoreClockUpdate();

	// 配置中断分组情况
	NVIC_Configuration();

	// 低电压检测配置
	//LVD_Configuration();

	// 中断向量表偏移
#if __SYSTEM_DEBUG__
#else
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8000);
#endif
}

// 系统总复位
void SYSTEM_Rst(uint32 param)
{
	__disable_fault_irq();
	NVIC_SystemReset();
}

// 运行时钟配置
void CLOCK_Configuration(void)
{
	RCC_DeInit();
	
	// Enable HSE        
	RCC_ConfigHse(RCC_HSE_ENABLE);

	// Wait till HSE is ready        
	if (RCC_WaitHseStable() == SUCCESS)// 等待HSE 起振，SUCCESS;HSE 晶振稳定且就绪
	{
		RCC_ConfigHclk(RCC_SYSCLK_DIV1);

		RCC_ConfigPclk1(RCC_HCLK_DIV2);
		RCC_ConfigPclk2(RCC_HCLK_DIV1);
		// 当 HCLK <= 32MHz 时，等待周期数最小为 0；当 32MHz < HCLK <= 64MHz时，等待周期数最小为 1
		FLASH_SetLatency(FLASH_LATENCY_1);
		FLASH_PrefetchBufSet(FLASH_PrefetchBuf_EN);
		RCC_ConfigPll(RCC_PLL_SRC_HSE_DIV1, RCC_PLL_MUL_6, RCC_PLLDIVCLK_DISABLE);
		RCC_EnablePll(ENABLE);
		while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) == RESET); 	// 检查指定的RCC 标志位(PLL 准备好标志)设置与否
		
		RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);
		while (RCC_GetSysclkSrc() != (uint32)0x0C);
	}
	else
	{
		RCC_EnableHsi(ENABLE);
		RCC_WaitHsiStable();
		
		RCC_ConfigHclk(RCC_SYSCLK_DIV1);

		RCC_ConfigPclk1(RCC_HCLK_DIV2);
		RCC_ConfigPclk2(RCC_HCLK_DIV1);
		// 当 HCLK <= 32MHz 时，等待周期数最小为 0；当 32MHz < HCLK <= 64MHz时，等待周期数最小为 1
		FLASH_SetLatency(FLASH_LATENCY_1);
		FLASH_PrefetchBufSet(FLASH_PrefetchBuf_EN);
		RCC_ConfigPll(RCC_PLL_HSI_PRE_DIV1, RCC_PLL_MUL_3, RCC_PLLDIVCLK_DISABLE);
		RCC_EnablePll(ENABLE);
		while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) == RESET); 	// 检查指定的RCC 标志位(PLL 准备好标志)设置与否
		
		RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);
		while (RCC_GetSysclkSrc() != (uint32)0x0C);
	}
}

// NVIC中断分组配置
void NVIC_Configuration(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
}


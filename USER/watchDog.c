#include "watchDog.h"

//*********************************************************** 
//函数名称： WDT_Init
//看门狗初始化
//***********************************************************/
void WatchDog_Init(void)
{
	IWDG_WriteConfig(IWDG_WRITE_ENABLE);
	
	IWDG_SetPrescalerDiv(IWDG_PRESCALER_DIV32);
	
	IWDG_CntReload(0xFFF);
	
	IWDG_ReloadKey();
	
	IWDG_Enable();
}
	

#include "common.h"
#include "powerCtl.h" 

// ◊¥Ã¨≥ı ºªØ
void POWER_Init(void)
{
	GPIO_InitType GPIO_InitStructure;
	
	// Enable GPIO clock  
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOD, ENABLE); 
	
	// GPIO≈‰÷√
	// Initialize GPIO_InitStructure
	GPIO_InitStruct(&GPIO_InitStructure);

	GPIO_InitStructure.Pin            = GPIO_PIN_2;
	GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
	GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
	GPIO_InitStructure.GPIO_Pull      = GPIO_No_Pull;
	GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Out_PP;
	GPIO_InitPeripheral(GPIOD, &GPIO_InitStructure);

	POWER_OFF();
}



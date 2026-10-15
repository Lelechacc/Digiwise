#include "lcdTest.h"
#include "lcd.h"
#include "delay.h"

/******************************************************************************
* 【内部函数】
******************************************************************************/

LCD_GPIO_PIN LcdPinList[] =
{
	{GPIO_PIN_1, GPIOA},
	{GPIO_PIN_2, GPIOA},
	{GPIO_PIN_3, GPIOA},
	{GPIO_PIN_6, GPIOA},
	{GPIO_PIN_7, GPIOA},
	{GPIO_PIN_8, GPIOA},
	{GPIO_PIN_9, GPIOA},
	{GPIO_PIN_10, GPIOA},
	{GPIO_PIN_15, GPIOA},

	{GPIO_PIN_0, GPIOB},
	{GPIO_PIN_1, GPIOB},
	{GPIO_PIN_3, GPIOB},
	{GPIO_PIN_4, GPIOB},
	{GPIO_PIN_5, GPIOB},
	{GPIO_PIN_8, GPIOB},
	{GPIO_PIN_9, GPIOB},
	{GPIO_PIN_10, GPIOB},
	{GPIO_PIN_11, GPIOB},
	{GPIO_PIN_12, GPIOB},
	{GPIO_PIN_13, GPIOB},
	{GPIO_PIN_14, GPIOB},
	{GPIO_PIN_15, GPIOB},

	{GPIO_PIN_0, GPIOC},
	{GPIO_PIN_1, GPIOC},
	{GPIO_PIN_2, GPIOC},
	{GPIO_PIN_3, GPIOC},
	{GPIO_PIN_4, GPIOC},
	{GPIO_PIN_5, GPIOC},
	{GPIO_PIN_6, GPIOC},
	{GPIO_PIN_7, GPIOC},
	{GPIO_PIN_8, GPIOC},
};


void LcdIoDirectionSet(LCD_GPIO_PIN *lcd, uint8 inout);
void LcdIoWriteBit(LCD_GPIO_PIN *lcd, uint8 bitVal);

// 出厂测试IO定义
void LcdIoInit(void)
{
	uint8 i;

	// GPIO配置全部上拉输入
    for (i = 0; i < sizeof(LcdPinList)/sizeof(LcdPinList[0])  ; i++)
    {
        LcdIoDirectionSet(&LcdPinList[i], 0);
    }
}

// IO 口切换设置
void LcdIoDirectionSet(LCD_GPIO_PIN *lcd, uint8 inout)
{
    GPIO_InitType GPIO_InitStructure;
	
	// Enable LCD GPIO Clocks
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA | RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOC | RCC_APB2_PERIPH_GPIOD, ENABLE);
	
    if (inout)
    {
		// GPIO配置
		// Initialize GPIO_InitStructure
		GPIO_InitStruct(&GPIO_InitStructure);

		GPIO_InitStructure.Pin            = lcd->pin;
		GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
		GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
		GPIO_InitStructure.GPIO_Pull      = GPIO_No_Pull;
		GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Out_PP;
    }
    else
    {
   		GPIO_InitStructure.Pin            = lcd->pin;
		GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;
		GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
		GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
		GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Input;
    }
    
	GPIO_InitPeripheral(lcd->gpio, &GPIO_InitStructure);
}

// IO 口切换设置
void LcdIoWriteBit(LCD_GPIO_PIN *lcd, uint8 bitVal)
{
    if (bitVal)
	{
		GPIO_SetBits(lcd->gpio, lcd->pin);
	}
	else
	{
		GPIO_ResetBits(lcd->gpio, lcd->pin);
	}
}

// LCD IO短路检测，true表示
RET_RESULT LcdIoTestResult(void)
{
    // 循环测试IO口是否短路
    int i = 0, j = 0;
    uint8 io_result = 0;
    RET_RESULT result = RET_SUCESS;
    
    // 全部上拉输入
    for (i = 0; i < sizeof(LcdPinList)/sizeof(LcdPinList[0]) ; i++)
    {
        LcdIoDirectionSet(&LcdPinList[i], 0);
    }
			
    // 逐个置低判断是否短路
    for (i = 0; i < sizeof(LcdPinList)/sizeof(LcdPinList[0]); i++)
    {
		// 将第	i 个设置为推挽输出模式
        LcdIoDirectionSet(&LcdPinList[i], 1);
        
		// 将第	i 个推挽输出0
        LcdIoWriteBit(&LcdPinList[i], 0);
				
        for (j = i+1; j < sizeof(LcdPinList)/sizeof(LcdPinList[0]); j++)
        {
            io_result = (uint8)GPIO_ReadOutputDataBit(LcdPinList[j].gpio, LcdPinList[j].pin);
            
			// AT芯片不加延时会报错
			Delayus(100);

			// 其它引脚读取到了低电平说明短路了
            if (0 == io_result)
            {
                result = RET_FAIL;
                
                return result;
            }
        }
    }
    
    return result;
}


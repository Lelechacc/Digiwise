#include "common.h"
#include "delay.h"

// 简单延时
void Delayus(uint8 i)
{	
	// 内核48MHz，1us
	while (i--) 
	{
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();  		
	}
}

// 简单延时
void Delayms(uint8 i) 
{
	uint16 x;
	uint8 y;

	// 内核48MHz，1ms
	while (i--)
	{
		x = 47; 
		while (x--)
		{
			y = 54;
			while (y--)
			{
				NOP(); 
			} 
		}
	}		
}

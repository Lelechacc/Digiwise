#ifndef __POWER_CTL_H__
#define __POWER_CTL_H__	 

#include "n32l40x_gpio.h"

#define POWER_ON()						GPIO_SetBits(GPIOD, GPIO_PIN_2)
#define POWER_OFF()						GPIO_ResetBits(GPIOD, GPIO_PIN_2)


void POWER_Init(void);

		 				    
#endif


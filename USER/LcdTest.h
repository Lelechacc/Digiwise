#ifndef 	__LCD_TEST_H__
#define 	__LCD_TEST_H__

#include "common.h"

typedef enum{
	RET_FAIL = 1,
	RET_SUCESS  = 0,
}RET_RESULT;

typedef struct
{
	uint16 pin;
	GPIO_Module *gpio;

}LCD_GPIO_PIN;

void LcdIoInit(void);
RET_RESULT LcdIoTestResult(void);

#endif


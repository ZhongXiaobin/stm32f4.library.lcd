#ifndef __DRIVER_LED_H_
#define __DRIVER_LED_H_

#include "explore_system.h"
#include "stm32f4xx.h"

/*LED亮灭对应的宏定义*/
#define LED_ON 0
#define LED_OFF 1

/*直接操作LED的位带操作宏*/
#define DS0 PFout(9)
#define DS1 PFout(10)
#define LED0 PFout(9)
#define LED1 PFout(10)

/*LED初始化函数*/
void LED_Init(void);

#endif /*__DRIVER_LED_H_*/

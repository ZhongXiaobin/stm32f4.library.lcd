#ifndef __EXPLORE_SYSTICK_H_
#define __EXPLORE_SYSTICK_H_

#include "stm32f4xx.h"

/*初始化延时函数*/
void Systick_Init(u8 SYSCLK);

/*延时nus微秒，最大延时798915us*/
void delay_us(u32 nus);

/*延时nms毫秒，最大延时65536ms*/
void delay_ms(u16 nms);

/*延时nms毫秒，最大延时798ms*/
void delay_xms(u16 nms);

#endif

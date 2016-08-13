#ifndef __EXPLORE_USART_H_
#define __EXPLORE_USART_H_

#include "stdio.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

/*定义最大接收字节数*/
#define USART_REC_LEN 200

/*接收缓冲，最大 USART_REC_LEN 个字节，末字节为换行符*/
extern u8 USART_RX_BUF[USART_REC_LEN];

/*接收状态标记(自定义的标志)*/
extern u16 USART_RX_STA;

/*串口1初始化函数*/
void Usart_Init(u32 bound);

#endif /*__EXPLORE_USART_H_*/

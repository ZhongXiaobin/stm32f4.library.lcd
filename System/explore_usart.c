#include "explore_usart.h"

/**
 * USART_RX_STA: 软件虚拟的寄存器，用于控制字节流的接收
 * USART_RX_STA[15]: 接收完成标志
 * USART_RX_STA[14]: 接收到0x0d
 * USART_RX_STA[13:0]: 接收到的有效字节数目
 */
u16 USART_RX_STA = 0;

/*接收缓冲数组,最大接收USART_REC_LEN个字节*/
u8 USART_RX_BUF[USART_REC_LEN];

/**
 * @Description 初始化I/O串口2
 * @param bound 波特率
 */
void Usart_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/*第一步：使能外设时钟*/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/*第二步:配置串口2对应的GPIO，设置为复用推挽输出*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*第三步：设置串口2对应的GPIO复用映射*/
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	/*第四步：USART2初始化设置*/
	/*根据参数设置波特率*/
	USART_InitStructure.USART_BaudRate = bound;
	/*设置串口字长为8位数据格式*/
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	/*设置串口停止位为1位*/
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	/*设置串口无奇偶校验*/
	USART_InitStructure.USART_Parity = USART_Parity_No;
	/*设置串口无硬件数据流控制*/
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	/*设置串口工作模式为收发模式*/
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	/*第五步：配置串口2中断*/
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	/*第六步：使能串口2*/
	USART_Cmd(USART2, ENABLE);

	/*第七步：开启串口2中断*/
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

/**
 * @Description 串口2中断服务函数，每有接收一个字节，申请一次中断，中断函数里面接收，直到接收到换行停止接收
 */
void USART2_IRQHandler(void)
{
	/*用于保存本次串口接收到的字节*/
	u8 Res;

	/*判断是不是接收中断，如果是接收中断，执行if里面的代码*/
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		/*读取串口1接收到的数据*/
		Res = USART_ReceiveData(USART2);

		/*接收未完成，这里接收的是一大串字节*/
		if((USART_RX_STA & 0x8000) == 0)
		{
			/*判断前一个接收的是不是0x0d*/
			if(USART_RX_STA & 0x4000)
			{
				/*如果前一个字节收到0x0d的话，再接收一个字节*/
				if(Res != 0x0a)
				{
					/*如果在接收的不是到0x0a，接收错误，重新开始*/
					USART_RX_STA = 0;
				}
				else
				{
					/*如果再接收到0x0a，就把USART_RX_STA中接收完成标志置位，表示接收完成了*/
					USART_RX_STA |= 0x8000;
				}
			}
			else
			{
				/*如果前一个字节还没收到0x0d的话，就接着接收下一个字节*/
				if(Res == 0x0d)
				{
					/*如果当前接收到的是0x0d的话*/
					/*则置位接收到0x0d的标志位 USART_RX_STA[14]*/
					USART_RX_STA |= 0x4000;
				}
				else
				{
					/*将当前接收到的值存入缓存数组中*/
					USART_RX_BUF[USART_RX_STA & 0X3FFF] = Res;

					/*接收到的字节个数加一*/
					USART_RX_STA++;

					/*判断接收到的数据有没有超过最开始设置的缓存区长度*/
					if(USART_RX_STA > (USART_REC_LEN - 1))
					{
						/*如果超出范围，表明接收数据错误，重新开始接收*/
						USART_RX_STA = 0;
					}
				}
			}
		}
	}
}

/*加入以下代码,支持printf函数，而不需要选择use MicroLIB*/
#pragma import(__use_no_semihosting)

/*标准库需要的支持函数*/
struct __FILE
{
	int handle;
};

FILE __stdout;

/**
 * @Description 定义_sys_exit()以避免使用半主机模式
 */
void _sys_exit(int x)
{
	x = x;
}

/**
 * @Description 重定义fputc函数
 */
int fputc(int ch, FILE *f)
{
	/*循环发送,直到发送完毕*/
	while((USART2->SR & 0X40) == 0)
		;

	/*装载要发送的数据*/
	USART2->DR = (u8) ch;

	return ch;
}

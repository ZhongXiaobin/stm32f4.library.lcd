#include "explore_systick.h"

/*保存延时1us/1ms滴答定时器的计数个数*/
static u8 fac_us = 0;
static u16 fac_ms = 0;

/**
 * @Description 初始化延时函数
 * @param system_clock 系统时钟(单位为Mhz)
 */
void Systick_Init(u8 system_clock)
{
	/*设置滴答定时器的时钟为系统时钟的8分频*/
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

	/*系统时钟为system_clock，滴答定时器的时钟为((system_clock * 10^6) / 8) */
	/*可以算出，一个计数值的时间是 (8 / (system_clock * 10^6)) */
	/*那么定时1us的计数值就是 ((10^-6 * (system_clock * 10^6))/ 8) = (system_clock / 8) */
	fac_us = system_clock / 8;

	/*定时1ms的计数值肯定就是1us的100倍了*/
	fac_ms = (u16) fac_us * 1000;
}

/**
 * @Description 延时nus微秒
 * @param nus 延时时间 nus <= 798915us(最大值即2^24 / fac_us@fac_us = 21)
 */
void delay_us(u32 nus)
{
	/*记录读取的滴答定时器控制寄存器CTRL数据的临时变量*/
	u32 temp;

	/*设置定时器的重装载值为 nus * fac_us*/
	/*根据定时的时间给滴答定时器加载重装载值*/
	SysTick->LOAD = nus * fac_us;

	/*设置滴答定时器的计数值为0，清空滴答定时器*/
	SysTick->VAL = 0x00;

	/*开启滴答定时器，此刻滴答定时器已经默默的在后台开始计时了*/
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

	/*持续的读取滴答定时器COUNTFLAG(CTRL的位16)，判断定时是否到达*/
	/*定时器没有到达之前程序一直停在这个死循环里面*/
	do
	{
		temp = SysTick->CTRL;
	} while ((temp & 0x01) && !(temp & (1 << 16)));

	/*关闭滴答定时器*/
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

	/*设置滴答定时器的计数值为0，清空滴答定时器*/
	SysTick->VAL = 0x00;
}

/**
 * @Description 延时nms毫秒
 * @param nus 延时时间 nms <= 798ms 算法同上一个函数
 */
void delay_xms(u16 nms)
{
	u32 temp;
	SysTick->LOAD = (u32) nms * fac_ms;
	SysTick->VAL = 0x00;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	do
	{
		temp = SysTick->CTRL;
	} while ((temp & 0x01) && !(temp & (1 << 16)));
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	SysTick->VAL = 0X00;
}

/**
 * @Description 延时nms毫秒
 * @param nus 延时时间 nms<=65535
 */
void delay_ms(u16 nms)
{
	/*这个函数其实就是将nms分解开来，分解成为多个 540ms * repeat + remain */
	/*然后分别延时，因为上面的延时函数组最多只可以延时798ms*/
	u8 repeat = nms / 540;
	u16 remain = nms % 540;

	while (repeat)
	{
		delay_xms(540);
		repeat--;
	}

	if (remain)
	{
		delay_xms(remain);
	}
}

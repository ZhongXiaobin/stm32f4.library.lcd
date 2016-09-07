#include "stm32f4xx.h"
#include "explore_system.h"
#include "explore_systick.h"
#include "explore_usart.h"
#include "driver_led.h"
#include "driver_lcd.h"

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	Systick_Init(168);
	Usart_Init(115200);
	LED_Init();
	LCD_Init();

	POINT_COLOR = RED;
	BACK_COLOR = WHITE;
	LCD_CenterShowString(50, "LCD Display Experiment", 24);

	while(1)
	{
		
	}
}

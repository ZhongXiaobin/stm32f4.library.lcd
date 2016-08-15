#include "driver_lcd.h"
#include "font.h"

/*@notice 本程序只适用4.3寸LCD，主控IC为NT35510*/
/*@notice LCD驱动版本 Lcd v1.1.3*/

/*LCD的画笔颜色*/
u16 POINT_COLOR = 0x0000;

/*LCD的背景颜色*/
u16 BACK_COLOR = 0xFFFF;

/*管理LCD重要参数*/
_lcd_dev lcddev;

/**
 * @Description 向LCD写命令
 * @param cmd 命令值
 */
void LCD_WriteCmd(vu16 cmd)
{
	cmd = cmd;
	LCD->LCD_REG = cmd;
}

/**
 * @Description 向LCD写数据
 * @param data 数据值
 */
void LCD_WriteData(vu16 data)
{
	data = data;
	LCD->LCD_RAM = data;
}

/**
 * @Description 读取LCD返回数据
 * @return data 读取到的数值
 */
u16 LCD_ReadData(void)
{
	vu16 data;
	data = LCD->LCD_RAM;
	return data;
}

/**
 * @Description 先写命令再写数据
 * @param LCD_Reg 命令值
 * @param LCD_RegValue 数据值
 */
void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
{
	LCD->LCD_REG = LCD_Reg;
	LCD->LCD_RAM = LCD_RegValue;
}

/**
 * @Description 读寄存器
 * @param LCD_Reg 寄存器地址
 * @return LCD_ReadData 读到的数据
 */
u16 LCD_ReadReg(u16 LCD_Reg)
{
	LCD_WriteCmd(LCD_Reg);
	delay_us(5);
	return LCD_ReadData();
}

/**
 * @Description 开始写GRAM，将保存在lcddev.wramcmd里面的命令写进LCD
 */
void LCD_WriteRAMPrepare(void)
{
	LCD->LCD_REG = lcddev.wramcmd;
}

/**
 * @Description LCD写GRAM,写入颜色值
 * @param RGB_Code 颜色值
 */
void LCD_WriteRAM(u16 RGB_Code)
{
	LCD->LCD_RAM = RGB_Code;
}

/**
 * @Description LCD开启显示
 */
void LCD_DisplayOn(void)
{
	LCD_WriteCmd(0X2900);
}

/**
 * @Description LCD关闭显示
 */
void LCD_DisplayOff(void)
{
	LCD_WriteCmd(0X2800);
}

/**
 * @Description 简单的延时函数,延时i
 * @notice 当mdk -O1时间优化时需要设置
 */
void LCD_Delay(u8 i)
{
	while (i--);
}

/**
 * @Description 将24位色转化为16位色
 * @param color24 24位色
 */
u16 LCD_ColorChange(u32 color24)
{
	u8 r = (color24 >> 16) & 0xff;
	u8 g = (color24 >> 8) & 0xff;
	u8 b = color24 & 0xff;

	r = (r / 8) & 0x1f;
	g = (g / 4) & 0x3f;
	b = (b / 8) & 0x1f;

	return (r << 11) | (g << 5) | b;
}

/**
 * @Description 读取某个点(一个点)的颜色值
 * @param x,y 坐标值
 * @return 对应点的颜色值，格式为 RRRRRGGG GGGBBBBB
 */
u16 LCD_ReadPoint(u16 x, u16 y)
{
	u16 r = 0;
	u16 g = 0;
	u16 b = 0;

	/*判断有没有超出屏幕范围，如果超出屏幕范围，则函数直接返回*/
	if (x >= lcddev.width || y >= lcddev.height)
	{
		return 0;
	}

	/*设置要读取点的坐标*/
	LCD_SetCursor(x, y);

	/*这里可以参考《模块使用手册》的第10页 0x2E00指令的讲解*/

	/*NT35510 发送读GRAM指令*/
	LCD_WriteCmd(0x2E00);

	/*假读，本次读取值无效《详情参看5510数据手册》*/
	r = LCD_ReadData();

	/*简单延时一下，准备下次读取*/
	LCD_Delay(2);

	/*读取颜色值，读取到的格式：RRRRRxxx GGGGGGxx*/
	r = LCD_ReadData();

	/*简单延时一下，准备下次读取*/
	LCD_Delay(2);

	/*继续读取颜色值，读取到的格式：BBBBBxxx xxxxxxxx*/
	b = LCD_ReadData();

	/*NT35510需要公式转换一下*/
	g = (r & 0xff) << 8;

	/*格式为 RRRRRGGG GGGBBBBB，因此需要移位操作*/
	return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));
}

/**
 * @Description 设置光标起始位置
 * @param Xpos 横坐标
 * @param Ypos 纵坐标
 */
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	/*设置光标x坐标*/
	LCD_WriteCmd(lcddev.setxcmd);
	LCD_WriteData(Xpos >> 8);
	LCD_WriteCmd(lcddev.setxcmd + 1);
	LCD_WriteData(Xpos & 0XFF);

	/*设置光标y坐标*/
	LCD_WriteCmd(lcddev.setycmd);
	LCD_WriteData(Ypos >> 8);
	LCD_WriteCmd(lcddev.setycmd + 1);
	LCD_WriteData(Ypos & 0XFF);
}

/**
 * @Description 设置LCD的自动扫描方向
 * @param dir 0~7,代表8个方向(具体定义见driver_lcd.h)
 * @notice 一般设置为L2R_U2D即可，如果设置为其他扫描方式，会导致部分函数显示不正常
 */
void LCD_ScanDir(u8 dir)
{
	u16 regval = 0;
	u16 dirreg = 0;
	u16 temp;
	if (lcddev.dir == 1)
	{
		switch (dir)
		{
		case 0:
			dir = 6;
			break;
		case 1:
			dir = 7;
			break;
		case 2:
			dir = 4;
			break;
		case 3:
			dir = 5;
			break;
		case 4:
			dir = 1;
			break;
		case 5:
			dir = 0;
			break;
		case 6:
			dir = 3;
			break;
		case 7:
			dir = 2;
			break;
		}
	}
	switch (dir)
	{
	case L2R_U2D:
		regval |= (0 << 7) | (0 << 6) | (0 << 5);
		break;
	case L2R_D2U:
		regval |= (1 << 7) | (0 << 6) | (0 << 5);
		break;
	case R2L_U2D:
		regval |= (0 << 7) | (1 << 6) | (0 << 5);
		break;
	case R2L_D2U:
		regval |= (1 << 7) | (1 << 6) | (0 << 5);
		break;
	case U2D_L2R:
		regval |= (0 << 7) | (0 << 6) | (1 << 5);
		break;
	case U2D_R2L:
		regval |= (0 << 7) | (1 << 6) | (1 << 5);
		break;
	case D2U_L2R:
		regval |= (1 << 7) | (0 << 6) | (1 << 5);
		break;
	case D2U_R2L:
		regval |= (1 << 7) | (1 << 6) | (1 << 5);
		break;
	}
	dirreg = 0X3600;

	LCD_WriteReg(dirreg, regval);

	if (regval & 0x20)
	{
		if (lcddev.width < lcddev.height)
		{
			temp = lcddev.width;
			lcddev.width = lcddev.height;
			lcddev.height = temp;
		}
	}
	else
	{
		if (lcddev.width > lcddev.height)
		{
			temp = lcddev.width;
			lcddev.width = lcddev.height;
			lcddev.height = temp;
		}
	}
}

/**
 * @Description 画点,点的颜色就是画笔的颜色 POINT_COLOR
 * @param x,y 点的坐标
 */
void LCD_DrawPoint(u16 x, u16 y)
{
	LCD_SetCursor(x, y);
	LCD_WriteRAMPrepare();
	LCD->LCD_RAM = POINT_COLOR;
}

/**
 * @Description 快速画点，通过直接操作，而不是函数调用(以空间换时间)
 * @param x,y 点的坐标
 * @param color 点的颜色
 */
void LCD_FastDrawPoint(u16 x, u16 y, u16 color)
{
	/*设置光标x坐标*/
	LCD_WriteCmd(lcddev.setxcmd);
	LCD_WriteData(x >> 8);
	LCD_WriteCmd(lcddev.setxcmd + 1);
	LCD_WriteData(x & 0xFF);

	/*设置光标y坐标*/
	LCD_WriteCmd(lcddev.setycmd);
	LCD_WriteData(y >> 8);
	LCD_WriteCmd(lcddev.setycmd + 1);
	LCD_WriteData(y & 0xFF);

	LCD->LCD_REG = lcddev.wramcmd;
	LCD->LCD_RAM = color;
}

/**
 * @Description 背光设置
 * @param pwm 背光等级，0~100，越大越亮
 */
void LCD_SSDBackLightSet(u8 pwm)
{
	/*配置PWM输出*/
	LCD_WriteCmd(0xBE);

	/*设置PWM频率*/
	LCD_WriteData(0x05);

	/*设置PWM占空比*/
	LCD_WriteData(pwm * 2.55);

	LCD_WriteData(0x01);
	LCD_WriteData(0xFF);
	LCD_WriteData(0x00);
	LCD_WriteData(0x00);
}

/**
 * @Description 设置LCD显示方向
 * @param dir 0,竖屏;1,横屏
 */
void LCD_DisplayDir(u8 dir)
{
	if (dir == SCREEN_VERTICAL)
	{
		/*如果是竖屏，设置分辨率，并标好方向*/
		lcddev.dir = SCREEN_VERTICAL;
		lcddev.width = 480;
		lcddev.height = 800;
	}
	else
	{
		/*如果是横屏，设置分辨率，并标好方向*/
		lcddev.dir = SCREEN_HORIZONTAL;
		lcddev.width = 800;
		lcddev.height = 480;
	}

	/*设置NT35510写坐标指令和写GRAM指令*/
	lcddev.wramcmd = 0X2C00;
	lcddev.setxcmd = 0X2A00;
	lcddev.setycmd = 0X2B00;

	/*设置扫描方向为 L2R_U2D*/
	LCD_ScanDir(DFT_SCAN_DIR);
}

/**
 * @Description 设置窗口,并自动设置画点坐标到窗口左上角(sx, sy)
 * @param sx, sy 窗口起始坐标(左上角)
 * @param width, height 窗口宽度和高度,必须大于0! 窗体大小:width*height.
 */
void LCD_SetWindow(u16 sx, u16 sy, u16 width, u16 height)
{
	u16 ex, ey;
	ex = sx + width - 1;
	ey = sy + height - 1;

	LCD_WriteCmd(lcddev.setxcmd);
	LCD_WriteData(sx >> 8);
	LCD_WriteCmd(lcddev.setxcmd + 1);
	LCD_WriteData(sx & 0xFF);
	LCD_WriteCmd(lcddev.setxcmd + 2);
	LCD_WriteData(ex >> 8);
	LCD_WriteCmd(lcddev.setxcmd + 3);
	LCD_WriteData(ex & 0xFF);

	LCD_WriteCmd(lcddev.setycmd);
	LCD_WriteData(sy >> 8);
	LCD_WriteCmd(lcddev.setycmd + 1);
	LCD_WriteData(sy & 0xFF);
	LCD_WriteCmd(lcddev.setycmd + 2);
	LCD_WriteData(ey >> 8);
	LCD_WriteCmd(lcddev.setycmd + 3);
	LCD_WriteData(ey & 0xFF);
}

/**
 * @Description 清屏函数
 * @param color 清屏的填充色
 */
void LCD_ClearScreen(u16 color)
{
	/*for循环要使用的变量*/
	u32 index = 0;

	/*得到总点数*/
	u32 total = lcddev.width * lcddev.height;

	/*设置光标位置从 坐标(0,0) 处开始绘点*/
	LCD_SetCursor(0, 0);

	/*开始写入GRAM*/
	LCD_WriteRAMPrepare();

	/*开始清屏，绘制所有的点的颜色*/
	for (index = 0; index < total; index++)
	{
		LCD->LCD_RAM = color;
	}
}

/**
 * @Description 在指定区域内填充单个颜色，区域大小为:(ex-sx+1)*(ey-sy+1)
 * @param sx,sy 将要被填充的矩形左上角坐标
 * @param ex,ey 将要被填充的矩形右下角坐标
 * @param color 要填充的颜色
 */
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{
	/*循环变量i, j*/
	u16 i, j;

	/*计算一行点的个数*/
	u16 xlen = ex - sx + 1;

	/*因为默认的扫描方向 L2R_U2D，因此可以设置一次绘制一行*/
	for (i = sy; i <= ey; i++)
	{
		/*设置光标位置(一行的起始位置，指定哪一列)*/
		LCD_SetCursor(sx, i);
		LCD_WriteRAMPrepare();

		/*绘制一行的点*/
		for (j = 0; j < xlen; j++)
			LCD->LCD_RAM = color;
	}
}

/**
 * @Description 在指定区域内填充指定颜色块，区域大小为:(ex-sx+1)*(ey-sy+1)
 * @param sx,sy 将要被填充的矩形左上角坐标
 * @param ex,ey 将要被填充的矩形右下角坐标
 * @param color 要填充的颜色
 */
void LCD_ColorFill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color)
{
	u16 i, j;
	u16 width = ex - sx + 1;
	u16 height = ey - sy + 1;
	for (i = 0; i < height; i++)
	{
		LCD_SetCursor(sx, sy + i);
		LCD_WriteRAMPrepare();
		for (j = 0; j < width; j++)
			LCD->LCD_RAM = color[i * width + j];
	}
}

/**
 * @Description 根据起始坐标绘制一条线段
 * @param x1,y1 起点坐标
 * @param x2,y2 终点坐标
 */
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t;
	int xerr = 0;
	int yerr = 0;
	int delta_x;
	int delta_y;
	int distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1;
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0)
		incx = 1;
	else if (delta_x == 0)
		incx = 0;
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0;
	else
	{
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y)
		distance = delta_x;
	else
		distance = delta_y;
	for (t = 0; t <= distance + 1; t++)
	{
		LCD_DrawPoint(uRow, uCol);
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance)
		{
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}
}

/**
 * @Description 画矩形,根据划线函数画一个矩形
 * @param (x1,y1),(x2,y2) 矩形的对角坐标
 */
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}

/**
 * @Description 根据圆心位置和半径画一个圆
 * @param x0,y0 圆心坐标
 * @param r 半径值
 */
void LCD_DrawCircle(u16 x0, u16 y0, u8 r)
{
	int a, b;
	int di;
	a = 0;
	b = r;
	di = 3 - (r << 1);
	while (a <= b)
	{
		LCD_DrawPoint(x0 + a, y0 - b);
		LCD_DrawPoint(x0 + b, y0 - a);
		LCD_DrawPoint(x0 + b, y0 + a);
		LCD_DrawPoint(x0 + a, y0 + b);
		LCD_DrawPoint(x0 - a, y0 + b);
		LCD_DrawPoint(x0 - b, y0 + a);
		LCD_DrawPoint(x0 - a, y0 - b);
		LCD_DrawPoint(x0 - b, y0 - a);
		a++;

		/*使用Bresenham算法画圆*/
		if (di < 0)
		{
			di += 4 * a + 6;
		}
		else
		{
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}

/**
 * @Description 在指定位置显示一个字符
 * @param x,y 起始坐标
 * @param num 要显示的字符:" " ---> "~"
 * @param size 字体大小 12/16/24
 * @param mode 叠加方式(DRAW_DIRECT),非叠加方式(DRAW_REDRAW)
 */
void LCD_ShowChar(u16 x, u16 y, u8 num, u8 size, u8 mode)
{
	u8 temp, t1, t;

	u16 y0 = y;

	/*得到当前设置的字体一个字符对应点阵集所占的字节数*/
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);

	/*得到偏移后的值(ASCII字库是从空格开始取模，所以-' '就是对应字符的字库)*/
	num = num - ' ';

	for (t = 0; t < csize; t++)
	{
		switch (size)
		{
		case 12:/*调用1206字体*/
			temp = asc2_1206[num][t];
			break;
		case 16:/*调用1608字体*/
			temp = asc2_1608[num][t];
			break;
		case 24:/*调用2412字体*/
			temp = asc2_2412[num][t];
			break;
		default:/*没有字库，则直接返回*/
			return;
		}

		for (t1 = 0; t1 < 8; t1++)
		{
			if (temp & 0x80)
			{
				/*判断最高位是是否为1，如果为1，则画点*/
				LCD_FastDrawPoint(x, y, POINT_COLOR);
			}
			else if (mode == DRAW_REDRAW)
			{
				/*最高位为0，且为非叠加方式的情况下，重新绘制空的点*/
				LCD_FastDrawPoint(x, y, BACK_COLOR);
			}

			/*将下一位移至最高位，循环操作*/
			temp <<= 1;

			/**/
			y++;

			/*如果超出了屏幕范围，则直接返回*/
			if (y >= lcddev.height)
			{
				return;
			}

			/*如果绘完了当前这一列*/
			if ((y - y0) == size)
			{
				/*y坐标回归初始值，开始绘制下一列*/
				y = y0;

				/*横坐标加1，即下一列*/
				x++;

				/*如果超出了屏幕范围，则直接返回*/
				if (x >= lcddev.width)
				{
					return;
				}
				break;
			}
		}
	}
}

/**
 * @Description m^n函数
 * @param m 底数
 * @param n 指数
 */
u32 LCD_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--)
	{
		result *= m;
	}
	return result;
}

/**
 * @Description 显示数字
 * @param x,y 起始坐标
 * @param num 数值(0~4294967295)
 * @param size 字体大小 12/16/24
 * @return 占用屏幕的长度
 * @notice 高位为0表示8进制数
 */
u8 LCD_ShowInt(u16 x, u16 y, u32 num, u8 size)
{
	/*for循环使用到的变量*/
	int i = 0;

	/*记录整数位数的变量，位数 = t+1*/
	u8 t = 0;

	/*记录整数每一位数值的数组*/
	u8 temp[10];

	/*从高位向低位依次取出每一位的数值，存入temp数组中*/
	for (i = 9; i >= 0; i--)
	{
		temp[i] = num / LCD_Pow(10, i);
		num = num % LCD_Pow(10, i);
	}

	/*从最高位开始，找出第一个不为0的数值*/
	for (i = 9; i >= 0; i--)
	{
		if (temp[i] != 0 || i == 0)
			break;
	}

	/*记录位数*/
	t = i;

	/*在屏幕上绘制*/
	for (i = 0; i <= t; i++)
	{
		LCD_ShowChar(x + size / 2 * i, y, temp[t - i] + 48, size, DRAW_DIRECT);
	}

	/*返回位数*/
	return (t + 1);
}

/**
 * @Description 显示小数
 * @param x,y 起始坐标
 * @param num 数值
 * @return 占用屏幕的长度
 * @param size 字体大小 12/16/24
 */
u8 LCD_ShowFloat(u16 x, u16 y, float num, u8 size)
{
	/*取出整数部分*/
	u32 ZhengShu = num / 1;

	/*取出小数部分*/
	float XiaoShu = num - ZhengShu;

	/*取出小数部分前3位*/
	XiaoShu = XiaoShu * 1000;

	/*摈弃小数点前三位之后的数据*/
	u16 Conversion = XiaoShu / 1;

	/*绘制整数部分，并得到整数部分长度*/
	u8 length = LCD_ShowInt(x, y, ZhengShu, size);

	/*绘制小数点*/
	LCD_ShowChar(x + length * size / 2, y, '.', size, DRAW_DIRECT);

	/*绘制小数部分*/
	if (XiaoShu < 10)
	{
		LCD_ShowChar(x + (length + 1) * size / 2, y, '0', size, DRAW_DIRECT);
		LCD_ShowChar(x + (length + 2) * size / 2, y, '0', size, DRAW_DIRECT);
		LCD_ShowInt(x + (length + 3) * size / 2, y, Conversion, size);
	}
	else if (XiaoShu < 100)
	{
		LCD_ShowChar(x + (length + 1) * size / 2, y, '0', size, DRAW_DIRECT);
		LCD_ShowInt(x + (length + 2) * size / 2, y, Conversion, size);
	}
	else
	{
		LCD_ShowInt(x + (length + 1) * size / 2, y, Conversion, size);
	}

	/*返回长度值*/
	return length + 4;
}

/**
 * @Description 显示字符串
 * @param x,y 起点坐标
 * @param width,height 区域大小
 * @param size 字体大小
 * @param *p 字符串起始地址
 */
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, char *p)
{
	u8 x0 = x;
	width += x;
	height += y;

	/*判断是不是非法字符!*/
	while ((*p <= '~') && (*p >= ' '))
	{
		if (x >= width)
		{
			x = x0;
			y += size;
		}
		if (y >= height)
			break;
		LCD_ShowChar(x, y, *p, size, 0);
		x += size / 2;
		p++;
	}
}

/**
 * @Description 初始化LCD，仅仅支持4.3寸屏，驱动IC为NT55330
 */
void LCD_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	FSMC_NORSRAMInitTypeDef FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef ReadTiming;
	FSMC_NORSRAMTimingInitTypeDef WriteTiming;

	/*第一步：使能FSMC时钟和GPIO时钟*/
	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

	/*第二步：配置GPIO模式*/
	/*PB15 推挽输出，用于控制背光*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*下面配置的GPIO全部是FSMC复用的I/O*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

	/*FSMC_A6 - PF12*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	/*FSMC_D0 - PD14*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	/*FSMC_D1 - PD15*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	/*FSMC_D2 - PD0*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	/*FSMC_D3 - PD1*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	/*FSMC_D4 - PE7*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D5 - PE8*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D6 - PE9*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D7 - PE10*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D8 - PE11*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D9 - PE12*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D10 - PE13*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D11 - PE14*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D12 - PE15*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	/*FSMC_D13 - PD8*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	/*FSMC_D14 - PD9*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	/*FSMC_D15 - PD10*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/*FSMC_NE4 - PG12*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	/*FSMC_NOE - PD4*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	/*FSMC_NWE - PD5*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/*第三步：设置GPIO复用模式*/
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource4, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource10, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource7, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource8, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource10, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource12, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource15, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource12, GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource12, GPIO_AF_FSMC);

	/*第四步：设置FSMC读写时序*/
	/*地址建立时间（ADDSET）为16个HCLK 1/168M=6ns*16=96ns*/
	ReadTiming.FSMC_AddressSetupTime = 16;
	/*地址保持时间（ADDHLD）模式A未用到*/
	ReadTiming.FSMC_AddressHoldTime = 0x00;
	/*数据保存时间为60个HCLK = 6*60=360ns*/
	ReadTiming.FSMC_DataSetupTime = 60;
	ReadTiming.FSMC_BusTurnAroundDuration = 0x00;
	ReadTiming.FSMC_CLKDivision = 0x00;
	ReadTiming.FSMC_DataLatency = 0x00;
	/*模式A*/
	ReadTiming.FSMC_AccessMode = FSMC_AccessMode_A;

	/*地址建立时间*/
	WriteTiming.FSMC_AddressSetupTime = 3;
	/*地址保持时间*/
	WriteTiming.FSMC_AddressHoldTime = 0;
	/*数据保存时间*/
	WriteTiming.FSMC_DataSetupTime = 3;
	WriteTiming.FSMC_BusTurnAroundDuration = 0x00;
	WriteTiming.FSMC_CLKDivision = 0x00;
	WriteTiming.FSMC_DataLatency = 0x00;
	/*模式A*/
	WriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;

	/*第五步：设置FSMC读写时序*/
	/*这里我们使用NE4,也就对应BTCR[6],[7]*/
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
	/*设置不复用数据地址*/
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	/*设置存储器类型为SRAM*/
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	/*存储器数据宽度为16bit*/
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	/*存储器写使能*/
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	/*读写使用不同的时序*/
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	/*读时序*/
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &ReadTiming;
	/*写时序*/
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &WriteTiming;

	/*初始化FSMC配置*/
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);

	/*使能BANK1*/
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);

	delay_ms(100);

	lcddev.id = 0x5510;
	printf("LCD ID:%x\r\n", lcddev.id);

	delay_ms(100);

	/*一系列官方提供的初始化指令*/
	LCD_WriteReg(0xF000, 0x55);
	LCD_WriteReg(0xF001, 0xAA);
	LCD_WriteReg(0xF002, 0x52);
	LCD_WriteReg(0xF003, 0x08);
	LCD_WriteReg(0xF004, 0x01);
	LCD_WriteReg(0xB000, 0x0D);
	LCD_WriteReg(0xB001, 0x0D);
	LCD_WriteReg(0xB002, 0x0D);
	LCD_WriteReg(0xB600, 0x34);
	LCD_WriteReg(0xB601, 0x34);
	LCD_WriteReg(0xB602, 0x34);
	LCD_WriteReg(0xB100, 0x0D);
	LCD_WriteReg(0xB101, 0x0D);
	LCD_WriteReg(0xB102, 0x0D);
	LCD_WriteReg(0xB700, 0x34);
	LCD_WriteReg(0xB701, 0x34);
	LCD_WriteReg(0xB702, 0x34);
	LCD_WriteReg(0xB200, 0x00);
	LCD_WriteReg(0xB201, 0x00);
	LCD_WriteReg(0xB202, 0x00);
	LCD_WriteReg(0xB800, 0x24);
	LCD_WriteReg(0xB801, 0x24);
	LCD_WriteReg(0xB802, 0x24);
	LCD_WriteReg(0xBF00, 0x01);
	LCD_WriteReg(0xB300, 0x0F);
	LCD_WriteReg(0xB301, 0x0F);
	LCD_WriteReg(0xB302, 0x0F);
	LCD_WriteReg(0xB900, 0x34);
	LCD_WriteReg(0xB901, 0x34);
	LCD_WriteReg(0xB902, 0x34);
	LCD_WriteReg(0xB500, 0x08);
	LCD_WriteReg(0xB501, 0x08);
	LCD_WriteReg(0xB502, 0x08);
	LCD_WriteReg(0xC200, 0x03);
	LCD_WriteReg(0xBA00, 0x24);
	LCD_WriteReg(0xBA01, 0x24);
	LCD_WriteReg(0xBA02, 0x24);
	LCD_WriteReg(0xBC00, 0x00);
	LCD_WriteReg(0xBC01, 0x78);
	LCD_WriteReg(0xBC02, 0x00);
	LCD_WriteReg(0xBD00, 0x00);
	LCD_WriteReg(0xBD01, 0x78);
	LCD_WriteReg(0xBD02, 0x00);
	LCD_WriteReg(0xBE00, 0x00);
	LCD_WriteReg(0xBE01, 0x64);
	LCD_WriteReg(0xD100, 0x00);
	LCD_WriteReg(0xD101, 0x33);
	LCD_WriteReg(0xD102, 0x00);
	LCD_WriteReg(0xD103, 0x34);
	LCD_WriteReg(0xD104, 0x00);
	LCD_WriteReg(0xD105, 0x3A);
	LCD_WriteReg(0xD106, 0x00);
	LCD_WriteReg(0xD107, 0x4A);
	LCD_WriteReg(0xD108, 0x00);
	LCD_WriteReg(0xD109, 0x5C);
	LCD_WriteReg(0xD10A, 0x00);
	LCD_WriteReg(0xD10B, 0x81);
	LCD_WriteReg(0xD10C, 0x00);
	LCD_WriteReg(0xD10D, 0xA6);
	LCD_WriteReg(0xD10E, 0x00);
	LCD_WriteReg(0xD10F, 0xE5);
	LCD_WriteReg(0xD110, 0x01);
	LCD_WriteReg(0xD111, 0x13);
	LCD_WriteReg(0xD112, 0x01);
	LCD_WriteReg(0xD113, 0x54);
	LCD_WriteReg(0xD114, 0x01);
	LCD_WriteReg(0xD115, 0x82);
	LCD_WriteReg(0xD116, 0x01);
	LCD_WriteReg(0xD117, 0xCA);
	LCD_WriteReg(0xD118, 0x02);
	LCD_WriteReg(0xD119, 0x00);
	LCD_WriteReg(0xD11A, 0x02);
	LCD_WriteReg(0xD11B, 0x01);
	LCD_WriteReg(0xD11C, 0x02);
	LCD_WriteReg(0xD11D, 0x34);
	LCD_WriteReg(0xD11E, 0x02);
	LCD_WriteReg(0xD11F, 0x67);
	LCD_WriteReg(0xD120, 0x02);
	LCD_WriteReg(0xD121, 0x84);
	LCD_WriteReg(0xD122, 0x02);
	LCD_WriteReg(0xD123, 0xA4);
	LCD_WriteReg(0xD124, 0x02);
	LCD_WriteReg(0xD125, 0xB7);
	LCD_WriteReg(0xD126, 0x02);
	LCD_WriteReg(0xD127, 0xCF);
	LCD_WriteReg(0xD128, 0x02);
	LCD_WriteReg(0xD129, 0xDE);
	LCD_WriteReg(0xD12A, 0x02);
	LCD_WriteReg(0xD12B, 0xF2);
	LCD_WriteReg(0xD12C, 0x02);
	LCD_WriteReg(0xD12D, 0xFE);
	LCD_WriteReg(0xD12E, 0x03);
	LCD_WriteReg(0xD12F, 0x10);
	LCD_WriteReg(0xD130, 0x03);
	LCD_WriteReg(0xD131, 0x33);
	LCD_WriteReg(0xD132, 0x03);
	LCD_WriteReg(0xD133, 0x6D);
	LCD_WriteReg(0xD200, 0x00);
	LCD_WriteReg(0xD201, 0x33);
	LCD_WriteReg(0xD202, 0x00);
	LCD_WriteReg(0xD203, 0x34);
	LCD_WriteReg(0xD204, 0x00);
	LCD_WriteReg(0xD205, 0x3A);
	LCD_WriteReg(0xD206, 0x00);
	LCD_WriteReg(0xD207, 0x4A);
	LCD_WriteReg(0xD208, 0x00);
	LCD_WriteReg(0xD209, 0x5C);
	LCD_WriteReg(0xD20A, 0x00);
	LCD_WriteReg(0xD20B, 0x81);
	LCD_WriteReg(0xD20C, 0x00);
	LCD_WriteReg(0xD20D, 0xA6);
	LCD_WriteReg(0xD20E, 0x00);
	LCD_WriteReg(0xD20F, 0xE5);
	LCD_WriteReg(0xD210, 0x01);
	LCD_WriteReg(0xD211, 0x13);
	LCD_WriteReg(0xD212, 0x01);
	LCD_WriteReg(0xD213, 0x54);
	LCD_WriteReg(0xD214, 0x01);
	LCD_WriteReg(0xD215, 0x82);
	LCD_WriteReg(0xD216, 0x01);
	LCD_WriteReg(0xD217, 0xCA);
	LCD_WriteReg(0xD218, 0x02);
	LCD_WriteReg(0xD219, 0x00);
	LCD_WriteReg(0xD21A, 0x02);
	LCD_WriteReg(0xD21B, 0x01);
	LCD_WriteReg(0xD21C, 0x02);
	LCD_WriteReg(0xD21D, 0x34);
	LCD_WriteReg(0xD21E, 0x02);
	LCD_WriteReg(0xD21F, 0x67);
	LCD_WriteReg(0xD220, 0x02);
	LCD_WriteReg(0xD221, 0x84);
	LCD_WriteReg(0xD222, 0x02);
	LCD_WriteReg(0xD223, 0xA4);
	LCD_WriteReg(0xD224, 0x02);
	LCD_WriteReg(0xD225, 0xB7);
	LCD_WriteReg(0xD226, 0x02);
	LCD_WriteReg(0xD227, 0xCF);
	LCD_WriteReg(0xD228, 0x02);
	LCD_WriteReg(0xD229, 0xDE);
	LCD_WriteReg(0xD22A, 0x02);
	LCD_WriteReg(0xD22B, 0xF2);
	LCD_WriteReg(0xD22C, 0x02);
	LCD_WriteReg(0xD22D, 0xFE);
	LCD_WriteReg(0xD22E, 0x03);
	LCD_WriteReg(0xD22F, 0x10);
	LCD_WriteReg(0xD230, 0x03);
	LCD_WriteReg(0xD231, 0x33);
	LCD_WriteReg(0xD232, 0x03);
	LCD_WriteReg(0xD233, 0x6D);
	LCD_WriteReg(0xD300, 0x00);
	LCD_WriteReg(0xD301, 0x33);
	LCD_WriteReg(0xD302, 0x00);
	LCD_WriteReg(0xD303, 0x34);
	LCD_WriteReg(0xD304, 0x00);
	LCD_WriteReg(0xD305, 0x3A);
	LCD_WriteReg(0xD306, 0x00);
	LCD_WriteReg(0xD307, 0x4A);
	LCD_WriteReg(0xD308, 0x00);
	LCD_WriteReg(0xD309, 0x5C);
	LCD_WriteReg(0xD30A, 0x00);
	LCD_WriteReg(0xD30B, 0x81);
	LCD_WriteReg(0xD30C, 0x00);
	LCD_WriteReg(0xD30D, 0xA6);
	LCD_WriteReg(0xD30E, 0x00);
	LCD_WriteReg(0xD30F, 0xE5);
	LCD_WriteReg(0xD310, 0x01);
	LCD_WriteReg(0xD311, 0x13);
	LCD_WriteReg(0xD312, 0x01);
	LCD_WriteReg(0xD313, 0x54);
	LCD_WriteReg(0xD314, 0x01);
	LCD_WriteReg(0xD315, 0x82);
	LCD_WriteReg(0xD316, 0x01);
	LCD_WriteReg(0xD317, 0xCA);
	LCD_WriteReg(0xD318, 0x02);
	LCD_WriteReg(0xD319, 0x00);
	LCD_WriteReg(0xD31A, 0x02);
	LCD_WriteReg(0xD31B, 0x01);
	LCD_WriteReg(0xD31C, 0x02);
	LCD_WriteReg(0xD31D, 0x34);
	LCD_WriteReg(0xD31E, 0x02);
	LCD_WriteReg(0xD31F, 0x67);
	LCD_WriteReg(0xD320, 0x02);
	LCD_WriteReg(0xD321, 0x84);
	LCD_WriteReg(0xD322, 0x02);
	LCD_WriteReg(0xD323, 0xA4);
	LCD_WriteReg(0xD324, 0x02);
	LCD_WriteReg(0xD325, 0xB7);
	LCD_WriteReg(0xD326, 0x02);
	LCD_WriteReg(0xD327, 0xCF);
	LCD_WriteReg(0xD328, 0x02);
	LCD_WriteReg(0xD329, 0xDE);
	LCD_WriteReg(0xD32A, 0x02);
	LCD_WriteReg(0xD32B, 0xF2);
	LCD_WriteReg(0xD32C, 0x02);
	LCD_WriteReg(0xD32D, 0xFE);
	LCD_WriteReg(0xD32E, 0x03);
	LCD_WriteReg(0xD32F, 0x10);
	LCD_WriteReg(0xD330, 0x03);
	LCD_WriteReg(0xD331, 0x33);
	LCD_WriteReg(0xD332, 0x03);
	LCD_WriteReg(0xD333, 0x6D);
	LCD_WriteReg(0xD400, 0x00);
	LCD_WriteReg(0xD401, 0x33);
	LCD_WriteReg(0xD402, 0x00);
	LCD_WriteReg(0xD403, 0x34);
	LCD_WriteReg(0xD404, 0x00);
	LCD_WriteReg(0xD405, 0x3A);
	LCD_WriteReg(0xD406, 0x00);
	LCD_WriteReg(0xD407, 0x4A);
	LCD_WriteReg(0xD408, 0x00);
	LCD_WriteReg(0xD409, 0x5C);
	LCD_WriteReg(0xD40A, 0x00);
	LCD_WriteReg(0xD40B, 0x81);
	LCD_WriteReg(0xD40C, 0x00);
	LCD_WriteReg(0xD40D, 0xA6);
	LCD_WriteReg(0xD40E, 0x00);
	LCD_WriteReg(0xD40F, 0xE5);
	LCD_WriteReg(0xD410, 0x01);
	LCD_WriteReg(0xD411, 0x13);
	LCD_WriteReg(0xD412, 0x01);
	LCD_WriteReg(0xD413, 0x54);
	LCD_WriteReg(0xD414, 0x01);
	LCD_WriteReg(0xD415, 0x82);
	LCD_WriteReg(0xD416, 0x01);
	LCD_WriteReg(0xD417, 0xCA);
	LCD_WriteReg(0xD418, 0x02);
	LCD_WriteReg(0xD419, 0x00);
	LCD_WriteReg(0xD41A, 0x02);
	LCD_WriteReg(0xD41B, 0x01);
	LCD_WriteReg(0xD41C, 0x02);
	LCD_WriteReg(0xD41D, 0x34);
	LCD_WriteReg(0xD41E, 0x02);
	LCD_WriteReg(0xD41F, 0x67);
	LCD_WriteReg(0xD420, 0x02);
	LCD_WriteReg(0xD421, 0x84);
	LCD_WriteReg(0xD422, 0x02);
	LCD_WriteReg(0xD423, 0xA4);
	LCD_WriteReg(0xD424, 0x02);
	LCD_WriteReg(0xD425, 0xB7);
	LCD_WriteReg(0xD426, 0x02);
	LCD_WriteReg(0xD427, 0xCF);
	LCD_WriteReg(0xD428, 0x02);
	LCD_WriteReg(0xD429, 0xDE);
	LCD_WriteReg(0xD42A, 0x02);
	LCD_WriteReg(0xD42B, 0xF2);
	LCD_WriteReg(0xD42C, 0x02);
	LCD_WriteReg(0xD42D, 0xFE);
	LCD_WriteReg(0xD42E, 0x03);
	LCD_WriteReg(0xD42F, 0x10);
	LCD_WriteReg(0xD430, 0x03);
	LCD_WriteReg(0xD431, 0x33);
	LCD_WriteReg(0xD432, 0x03);
	LCD_WriteReg(0xD433, 0x6D);
	LCD_WriteReg(0xD500, 0x00);
	LCD_WriteReg(0xD501, 0x33);
	LCD_WriteReg(0xD502, 0x00);
	LCD_WriteReg(0xD503, 0x34);
	LCD_WriteReg(0xD504, 0x00);
	LCD_WriteReg(0xD505, 0x3A);
	LCD_WriteReg(0xD506, 0x00);
	LCD_WriteReg(0xD507, 0x4A);
	LCD_WriteReg(0xD508, 0x00);
	LCD_WriteReg(0xD509, 0x5C);
	LCD_WriteReg(0xD50A, 0x00);
	LCD_WriteReg(0xD50B, 0x81);
	LCD_WriteReg(0xD50C, 0x00);
	LCD_WriteReg(0xD50D, 0xA6);
	LCD_WriteReg(0xD50E, 0x00);
	LCD_WriteReg(0xD50F, 0xE5);
	LCD_WriteReg(0xD510, 0x01);
	LCD_WriteReg(0xD511, 0x13);
	LCD_WriteReg(0xD512, 0x01);
	LCD_WriteReg(0xD513, 0x54);
	LCD_WriteReg(0xD514, 0x01);
	LCD_WriteReg(0xD515, 0x82);
	LCD_WriteReg(0xD516, 0x01);
	LCD_WriteReg(0xD517, 0xCA);
	LCD_WriteReg(0xD518, 0x02);
	LCD_WriteReg(0xD519, 0x00);
	LCD_WriteReg(0xD51A, 0x02);
	LCD_WriteReg(0xD51B, 0x01);
	LCD_WriteReg(0xD51C, 0x02);
	LCD_WriteReg(0xD51D, 0x34);
	LCD_WriteReg(0xD51E, 0x02);
	LCD_WriteReg(0xD51F, 0x67);
	LCD_WriteReg(0xD520, 0x02);
	LCD_WriteReg(0xD521, 0x84);
	LCD_WriteReg(0xD522, 0x02);
	LCD_WriteReg(0xD523, 0xA4);
	LCD_WriteReg(0xD524, 0x02);
	LCD_WriteReg(0xD525, 0xB7);
	LCD_WriteReg(0xD526, 0x02);
	LCD_WriteReg(0xD527, 0xCF);
	LCD_WriteReg(0xD528, 0x02);
	LCD_WriteReg(0xD529, 0xDE);
	LCD_WriteReg(0xD52A, 0x02);
	LCD_WriteReg(0xD52B, 0xF2);
	LCD_WriteReg(0xD52C, 0x02);
	LCD_WriteReg(0xD52D, 0xFE);
	LCD_WriteReg(0xD52E, 0x03);
	LCD_WriteReg(0xD52F, 0x10);
	LCD_WriteReg(0xD530, 0x03);
	LCD_WriteReg(0xD531, 0x33);
	LCD_WriteReg(0xD532, 0x03);
	LCD_WriteReg(0xD533, 0x6D);
	LCD_WriteReg(0xD600, 0x00);
	LCD_WriteReg(0xD601, 0x33);
	LCD_WriteReg(0xD602, 0x00);
	LCD_WriteReg(0xD603, 0x34);
	LCD_WriteReg(0xD604, 0x00);
	LCD_WriteReg(0xD605, 0x3A);
	LCD_WriteReg(0xD606, 0x00);
	LCD_WriteReg(0xD607, 0x4A);
	LCD_WriteReg(0xD608, 0x00);
	LCD_WriteReg(0xD609, 0x5C);
	LCD_WriteReg(0xD60A, 0x00);
	LCD_WriteReg(0xD60B, 0x81);
	LCD_WriteReg(0xD60C, 0x00);
	LCD_WriteReg(0xD60D, 0xA6);
	LCD_WriteReg(0xD60E, 0x00);
	LCD_WriteReg(0xD60F, 0xE5);
	LCD_WriteReg(0xD610, 0x01);
	LCD_WriteReg(0xD611, 0x13);
	LCD_WriteReg(0xD612, 0x01);
	LCD_WriteReg(0xD613, 0x54);
	LCD_WriteReg(0xD614, 0x01);
	LCD_WriteReg(0xD615, 0x82);
	LCD_WriteReg(0xD616, 0x01);
	LCD_WriteReg(0xD617, 0xCA);
	LCD_WriteReg(0xD618, 0x02);
	LCD_WriteReg(0xD619, 0x00);
	LCD_WriteReg(0xD61A, 0x02);
	LCD_WriteReg(0xD61B, 0x01);
	LCD_WriteReg(0xD61C, 0x02);
	LCD_WriteReg(0xD61D, 0x34);
	LCD_WriteReg(0xD61E, 0x02);
	LCD_WriteReg(0xD61F, 0x67);
	LCD_WriteReg(0xD620, 0x02);
	LCD_WriteReg(0xD621, 0x84);
	LCD_WriteReg(0xD622, 0x02);
	LCD_WriteReg(0xD623, 0xA4);
	LCD_WriteReg(0xD624, 0x02);
	LCD_WriteReg(0xD625, 0xB7);
	LCD_WriteReg(0xD626, 0x02);
	LCD_WriteReg(0xD627, 0xCF);
	LCD_WriteReg(0xD628, 0x02);
	LCD_WriteReg(0xD629, 0xDE);
	LCD_WriteReg(0xD62A, 0x02);
	LCD_WriteReg(0xD62B, 0xF2);
	LCD_WriteReg(0xD62C, 0x02);
	LCD_WriteReg(0xD62D, 0xFE);
	LCD_WriteReg(0xD62E, 0x03);
	LCD_WriteReg(0xD62F, 0x10);
	LCD_WriteReg(0xD630, 0x03);
	LCD_WriteReg(0xD631, 0x33);
	LCD_WriteReg(0xD632, 0x03);
	LCD_WriteReg(0xD633, 0x6D);
	LCD_WriteReg(0xF000, 0x55);
	LCD_WriteReg(0xF001, 0xAA);
	LCD_WriteReg(0xF002, 0x52);
	LCD_WriteReg(0xF003, 0x08);
	LCD_WriteReg(0xF004, 0x00);
	LCD_WriteReg(0xB100, 0xCC);
	LCD_WriteReg(0xB101, 0x00);
	LCD_WriteReg(0xB600, 0x05);
	LCD_WriteReg(0xB700, 0x70);
	LCD_WriteReg(0xB701, 0x70);
	LCD_WriteReg(0xB800, 0x01);
	LCD_WriteReg(0xB801, 0x03);
	LCD_WriteReg(0xB802, 0x03);
	LCD_WriteReg(0xB803, 0x03);
	LCD_WriteReg(0xBC00, 0x02);
	LCD_WriteReg(0xBC01, 0x00);
	LCD_WriteReg(0xBC02, 0x00);
	LCD_WriteReg(0xC900, 0xD0);
	LCD_WriteReg(0xC901, 0x02);
	LCD_WriteReg(0xC902, 0x50);
	LCD_WriteReg(0xC903, 0x50);
	LCD_WriteReg(0xC904, 0x50);
	LCD_WriteReg(0x3500, 0x00);
	LCD_WriteReg(0x3A00, 0x55);
	LCD_WriteCmd(0x1100);
	delay_us(120);
	LCD_WriteCmd(0x2900);

	/*默认开机为竖屏*/
	LCD_DisplayDir(0);

	/*点亮背光*/
	LCD_LED = 1;

	/*默认以白色为背景刷屏*/
	LCD_ClearScreen(WHITE);
}

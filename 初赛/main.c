#include <STC15F2K60S2.H>
#include "onewire.h"

typedef unsigned char uchar;
typedef unsigned int uint;

sbit level = P3^4;
sbit S7 = P3^0;
sbit S6 = P3^1;
sbit S5 = P3^2;
sbit S4 = P3^3;
sbit L1 = P0^0;
sbit L2 = P0^1;
sbit L3 = P0^2;

uchar code dpnum[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xbf,0xff,0xc6};

uchar Fan_working_mode=1;
uchar Fan_pwm;
uchar count_t1=0;
uchar count_t2=0;
uchar Interface=0;
uchar Fan_time_set=0;
uchar Runtime=60;
bit S7_state=0;
uint temperature;

void System_Init();
void Fan_mode();
void Timer0_Init();
void delayms(uchar ms);
void key();
void display_bit(uchar pos,uchar dat);
void Face();
void Timer1_Init();
uint DS18B20_Read();
void LED_working();

void main()
{
	System_Init();
	Timer0_Init();
	Timer1_Init();
	while(1)
	{
		key();
		Fan_mode();
		Face();
		LED_working();
	}
}

void System_Init()
{
	P2 = (P2 & 0x1f) | 0x80;
	P0 = 0xff;
	P2 = (P2 & 0x1f) | 0xa0;
	P0 = 0x00;
	P2 = (P2 & 0x1f) | 0xc0;
	P0 = 0xff;
	P2 = (P2 & 0x1f) | 0xe0;
	P0 = 0xff;
}
void Fan_mode()
{
	switch(Fan_working_mode)
	{
		case 1:
			Fan_pwm=20;
			break;
		case 2:
			Fan_pwm=30;
			break;
		case 3:
			Fan_pwm=70;
			break;
	}
}
void Timer0_Init()
{
	TMOD |= 0X00;  //16位自动重装模式
	TH0 = (65536-10)/256;
	TL0 = (65536-10)%256;
	TR0 = 1;
	ET0 = 1;
	EA = 1;
}
void Timer0() interrupt 1
{
	count_t1++;
	if(count_t1 < Fan_pwm)
		level = 1;
	else if(count_t1 < 100)
		level = 0;
	else if(count_t1 == 100)
	{
		level = 1;
		count_t1=0;
	}
}
void delayms(uchar ms)
{
	uint i,j;
	for(i=ms;i>0;i--)
		for(j=810;j>0;j--);
}
void key()
{
	if(S4 == 0)
	{
		delayms(5);
		if(S4 == 0)
		{
			Fan_working_mode++;
			if(Fan_working_mode>3)
				Fan_working_mode=1;
			while(S4==0);
		}
	}
	else if(S5==0)
	{
		delayms(5);
		if(S5 == 0)
		{
			TR0 = 1;
			TR1 = 1;
			Fan_time_set+=60;
			if(Fan_time_set>120)
				Fan_time_set=0;
			Runtime = Fan_time_set;
			while(S5==0);
		}
	}
	else if(S6==0)
	{
		delayms(5);
		if(S6 == 0)
		{
			Runtime = 0;
		}
	}
	else if(S7==0)
	{
		delayms(5);
		if(S7 == 0)
		{
			if(S7_state==0)
			{
				Interface=1;
				S7_state=1;
			}
			else
			{
				Interface=0;
				S7_state=0;
			}
		}
		while(S7==0);
	}
}
void display_bit(uchar pos,uchar dat)
{
	P2 = (P2 & 0x1f) | 0xc0;
	P0 = 0x01 << pos;
	P2 = (P2 & 0x1f) | 0xe0;
	P0 = dat;
}
void Face()
{
	switch(Interface)
	{
		case 0:	//工作模式和运行时间显示界面
			if(Runtime==0)
			{
				TR0 = 0;
				TR1 = 0;
				level=0;//当计时结束时，停止pwm信号输出
			}
			display_bit(0,dpnum[10]);
			delayms(1);
			display_bit(1,dpnum[Fan_working_mode]);
			delayms(1);
			display_bit(2,dpnum[10]);
			delayms(1);
			display_bit(3,dpnum[11]);
			delayms(1);
			display_bit(4,dpnum[0]);
			delayms(1);
			display_bit(5,dpnum[Runtime/100]);
			delayms(1);
			display_bit(6,dpnum[Runtime/10%10]);
			delayms(1);
			display_bit(7,dpnum[Runtime%10]);
			delayms(1);
			P2 = (P2 & 0x1f) | 0xc0;
			P0 = 0xff;
			P2 = (P2 & 0x1f) | 0xe0;
			P0 = 0xff;
			break;
		case 1: //温度显示界面
			temperature=DS18B20_Read(); //如果倒计时没有结束，温度显示界面会一直闪烁，倒计时结束停止闪烁，把这句话放到main函数里面在界面1,2都会闪烁，猜测为定时器打开和温度读取同时进行的问题
			if(Fan_time_set>120)
				Fan_time_set=0;
			//Runtime = Fan_time_set; //如果加上此句在温度显示界面倒计时一直重新赋值
			if(Runtime==0)
			{
				TR0 = 0;
				TR1 = 0;
				level=0;//当计时结束时，停止pwm信号输出
			}
			display_bit(0,dpnum[10]);
			delayms(1);
			display_bit(1,dpnum[4]);
			delayms(1);
			display_bit(2,dpnum[10]);
			delayms(1);
			display_bit(3,dpnum[11]);
			delayms(1);
			display_bit(4,dpnum[11]);
			delayms(1);
			display_bit(5,dpnum[temperature/10]);
			delayms(1);
			display_bit(6,dpnum[temperature%10]);//温度检测的时候除数为10
			delayms(1);
			display_bit(7,dpnum[12]);
			delayms(1);
			P2 = (P2 & 0x1f) | 0xc0;
			P0 = 0xff;
			P2 = (P2 & 0x1f) | 0xe0;
			P0 = 0xff;
			break;
	}
}
void Timer1_Init()
{
	TMOD |= 0X00;
	TH1 = (65536-10000)/256;
	TL1 = (65536-10000)%256;
	TR1 = 1;
	ET1 = 1;
	EA = 1;
}
void Timer1() interrupt 3
{
	count_t2++;
	if(count_t2 == 100)
	{
		Runtime--;
		count_t2=0;
	}
}
uint DS18B20_Read()
{
	uchar LSB,MSB;
	uint temp;
	
	init_ds18b20();
	Write_DS18B20(0xcc);//跳过ROM指令
	Write_DS18B20(0x44);//开始温度转换
	
	delayms(1);
	
	init_ds18b20();
	Write_DS18B20(0xcc);//跳过ROM指令
	Write_DS18B20(0xbe);//读取高速暂存器
	
	LSB = Read_DS18B20();
	MSB = Read_DS18B20();
	init_ds18b20();
	
	temp = (MSB << 8) | LSB;
	temp >>= 4;
	
	return temp;
}
void LED_working()
{
	switch(Fan_working_mode)
	{
		case 1:
			P2 = (P2 & 0x1f) | 0x80;
			P0 = 0xff;
			L1 = 0;
			break;
		case 2:
			P2 = (P2 & 0x1f) | 0x80;
			P0 = 0xff;
			L2 = 0;
			break;
		case 3:
			P2 = (P2 & 0x1f) | 0x80;
			P0 = 0xff;
			L3 = 0;
			break;
	}
	if(Runtime==0)
	{
		P2 = (P2 & 0x1f) | 0x80;
		P0 = 0xff;
		delayms(1);
	}
}
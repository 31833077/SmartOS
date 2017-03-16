﻿#include "Kernel\Sys.h"
#include "Kernel\TTime.h"

#include "Device\Port.h"

#include "ACZero.h"

ACZero::ACZero()
{
	Period = 10000;
	Width = 0;
	Count = 0;
	Last = 0;
}

ACZero::~ACZero()
{
	if (Port.Opened) Close();
}

void ACZero::Set(Pin pin)
{
	if (pin == P0) return;

	Port.HardEvent = true;
	Port.Set(pin);
	Port.Press.Bind(&ACZero::OnHandler, this);
	Port.UsePress();
}

bool ACZero::Open()
{
	if (!Port.Open()) return false;

	debug_printf("过零检测引脚探测: ");

	// 等一次零点
	Sys.Sleep(20);
	if (Count > 0)
	{
		debug_printf("已连接交流电！\r\n");
	}
	else
	{
		debug_printf("未连接交流电！\r\n");
		Port.Close();
		return false;
	}

	return true;
}

void ACZero::Close()
{
	Port.Close();
}

void ACZero::OnHandler(InputPort& port, bool down)
{
	if (!down)
	{
		Width = port.PressTime;
		return;
	}

	auto now = Sys.Ms();
	Count++;

	// 每256*10ms修正一次周期
	if (Last > 0 && (Count & 0xFF) == 0)
		//if (Last > 0 && (Count & 0xFF) == 0)
	{
		// 两次零点
		int ms = now - Last;
		int us = ms * 1000;

		// 零点信号可能有毛刺或者干扰，需要避开
		//if (us <= Period / 2 || us >= Period * 2) return;
		if (us <= (Period >> 1) || us >= (Period << 1)) return;

		// 通过加权平均算法纠正数据
		Period = (Period * 7 + us) / 8;

		debug_printf("OnHandler us=%d Period=%d Width=%d \r\n", us, Period, Width);
	}

	Last = now;
}

// 等待下一次零点，需要考虑继电器动作延迟
bool ACZero::Wait(int usDelay) const
{
	if (Count == 0 || Last == 0) return false;

	// 计算上一次零点后过去的时间
	int ms = Sys.Ms() - Last;
	if (ms < 0 && ms > 40) return false;

	// 计算下一次零点什么时候到来
	int us = Period - ms * 1000;
	// 继电器动作延迟
	us -= usDelay;

	int d = Period;
	while (us < 0) us += d;
	while (us > d) us -= d;

	debug_printf("ACZero::Wait 周期=%dus 等待=%dus Width=%dms Count=%d Last=%d \r\n", Period, us, Width, Count, (int)Last);

	//Sys.Delay(us);
	Time.Delay(us);

	return true;
}
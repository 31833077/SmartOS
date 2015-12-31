﻿#include "TinyConfig.h"
#include "Config.h"

TinyConfig* TinyConfig::Current	= NULL;

TinyConfig::TinyConfig()
{
	Cfg	= Config::Current;
}

void TinyConfig::LoadDefault()
{
	// 实际内存大小，减去头部大小
	uint len = sizeof(this[0]) - ((int)&Length - (int)this);
	memset(&Length, 0, len);
	Length	= len;

	Kind	= Sys.Code;
	//Server	= 0x01;

	Channel	= 120;
	Speed	= 250;

	PingTime	= 20;
	OfflineTime	= 60;
}

void TinyConfig::Load()
{
	if(!Cfg) return;

	// 尝试加载配置区设置
	uint len = Length;
	if(!len) len = sizeof(this[0]);
	Array bs(&Length, len);
	if(!Cfg->GetOrSet("TCFG", bs))
		debug_printf("TinyConfig::Load 首次运行，创建配置区！\r\n");
	else
		debug_printf("TinyConfig::Load 从配置区加载配置\r\n");

	if(Kind != Sys.Code)
	{
		debug_printf("TinyConfig::Load 设备类型变更\r\n");

		Kind	= Sys.Code;
		Cfg->Set("TCFG", bs);
	}
}

void TinyConfig::Save()
{
	if(!Cfg) return;

	uint len = Length;
	if(!len) len = sizeof(this[0]);

	debug_printf("TinyConfig::Save \r\n");

	Array bs(&Length, len);
	Cfg->Set("TCFG", bs);
}

void TinyConfig::Clear()
{
	if(!Cfg) return;

	LoadDefault();

	debug_printf("TinyConfig::Clear \r\n");

	Array bs(&Length, Length);
	Cfg->Set("TCFG", bs);
}

void TinyConfig::Write(Stream& ms) const
{
	ms.Write(&Length, 0, Length);
}

void TinyConfig::Read(Stream& ms)
{
	ms.Read(&Length, 0, Length);
}

TinyConfig* TinyConfig::Init()
{
	// 默认出厂设置
	static TinyConfig tc;
	TinyConfig::Current = &tc;
	tc.LoadDefault();

	return &tc;
}

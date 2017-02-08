﻿#include "IOK026X.h"

#include "Kernel\Task.h"

#include "Device\Power.h"
#include "Device\WatchDog.h"
#include "Config.h"

#include "Drivers\Esp8266\Esp8266.h"
#include "TokenNet\TokenController.h"
#include "Kernel\Task.h"

IOK026X* IOK026X::Current = nullptr;

IOK026X::IOK026X()
{
	LedPins.Add(PA0);
	LedPins.Add(PA4);

	LedsShow = 2;
	LedsTaskId = 0;

	Host = nullptr;	// 网络主机
	Client = nullptr;

	Data = nullptr;
	Size = 0;
	Current = this;
}

void IOK026X::Init(ushort code, cstring name, COM message)
{
	auto& sys = (TSys&)Sys;
	sys.Code = code;
	sys.Name = (char*)name;

	// RTC 提取时间
	auto Rtc = HardRTC::Instance();
	Rtc->LowPower = false;
	Rtc->External = false;
	Rtc->Init();
	Rtc->Start(false, false);

	// 初始化系统
	sys.Init();
#if DEBUG
	sys.MessagePort = message; // 指定printf输出的串口
	Sys.ShowInfo();

	WatchDog::Start(20000, 10000);
#else
	WatchDog::Start();

	// 系统休眠时自动进入低功耗
	Power::AttachTimeSleep();
#endif

	// Flash最后一块作为配置区
	Config::Current = &Config::CreateFlash();
}

void* IOK026X::InitData(void* data, int size)
{
	// 启动信息
	auto hot = &HotConfig::Current();
	hot->Times++;

	data = hot->Next();
	if (hot->Times == 1)
	{
		Buffer ds(data, size);
		ds.Clear();
		ds[0] = size;
	}
	// Buffer bs(data, size);
	// debug_printf("HotConfig Times %d Data: ",hot->Times);
	// bs.Show(true);

	Data = data;
	Size = size;

	return data;
}

void IOK026X::InitLeds()
{
	for (int i = 0; i < LedPins.Count(); i++)
	{
		auto port = new OutputPort(LedPins[i]);
		port->Invert = true;
		port->Open();
		port->Write(false);
		Leds.Add(port);
	}
}

NetworkInterface* IOK026X::CreateNet()
{
	auto host = new Esp8266(COM2, PB2, PA1);

	// 初次需要指定模式 否则为 Wire
	bool join = host->SSID && *host->SSID;
	//if (!join) host->Mode = NetworkType::AP;

	if (!join)
	{
		*host->SSID = "WSWL";
		*host->Pass = "12345678";

		host->Mode = NetworkType::STA_AP;
	}

	Client->Register("SetWiFi", &Esp8266::SetWiFi, host);
	Client->Register("GetWiFi", &Esp8266::GetWiFi, host);
	Client->Register("GetAPs", &Esp8266::GetAPs, host);

	host->Open();

	return host;
}

/******************************** Token ********************************/

void IOK026X::InitClient()
{
	if (Client) return;

	auto tk = TokenConfig::Current;

	// 创建客户端
	auto client = new TokenClient();
	client->Cfg = tk;

	Client = client;
	Client->MaxNotActive = 480000;
	// 重启
	Client->Register("Gateway/Restart", &TokenClient::InvokeRestart, Client);
	// 重置
	Client->Register("Gateway/Reset", &TokenClient::InvokeReset, Client);
	// 获取所有Ivoke命令
	Client->Register("Api/All", &TokenClient::InvokeGetAllApi, Client);

	if (Data && Size > 0)
	{
		auto& ds = Client->Store;
		ds.Data.Set(Data, Size);
	}

	// 如果若干分钟后仍然没有打开令牌客户端，则重启系统
	Sys.AddTask(
		[](void* p) {
		auto & bsp = *(IOK026X*)p;
		auto & client = *bsp.Client;
		if (!client.Opened)
		{
			debug_printf("联网超时，准备重启Esp！\r\n\r\n");
			// Sys.Reboot();
			auto port = dynamic_cast<Esp8266*>(bsp.Host);
			port->Close();
			Sys.Sleep(1000);
			port->Open();
		}
	},
		this, 8 * 60 * 1000, -1, "联网检查");
}

void IOK026X::Register(int index, IDataPort& dp)
{
	if (!Client) return;

	auto& ds = Client->Store;
	ds.Register(index, dp);
}

void IOK026X::InitNet()
{
	Host = CreateNet();
}

static void OnAlarm(AlarmItem& item)
{
	// 1长度n + 1类型 + 1偏移 + (n-2)数据
	auto bs = item.GetData();
	debug_printf("OnAlarm ");
	bs.Show(true);

	if (bs[1] == 0x06)
	{
		auto client = IOK026X::Current->Client;
		client->Store.Write(bs[2], bs.Sub(3, bs[0] - 2));

		// 主动上报状态
		client->ReportAsync(bs[2], bs[0] - 2);
	}
}

void IOK026X::InitAlarm()
{
	if (!Client)return;

	if (!AlarmObj) AlarmObj = new Alarm();
	Client->Register("Policy/AlarmSet", &Alarm::Set, AlarmObj);
	Client->Register("Policy/AlarmGet", &Alarm::Get, AlarmObj);

	AlarmObj->OnAlarm = OnAlarm;
	AlarmObj->Start();
}

//双联开关被触发 
static void UnionPress(InputPort& port, bool down)
{
	if (IOK026X::Current == nullptr) return;
	auto client = IOK026X::Current->Client;

	byte data[1];
	data[0] = down ? 1 : 0;

	client->Store.Write(port.Index + 1, Buffer(data, 1));
	// 主动上报状态
	client->ReportAsync(port.Index + 1, 1);

}
void IOK026X::Union(Pin pin1, Pin pin2)
{
	Pin p[] = { pin1,pin2 };
	for (size_t i = 0; i < 2; i++)
	{
		auto port = new InputPort(p[i]);
		port->Invert = true;
		port->ShakeTime = 40;
		port->Index = i;
		port->Press.Bind(UnionPress);
		port->UsePress();
		port->Open();
	}
}

static bool ledstat2 = false;
void IOK026X::Restore()
{
	if (Client) Client->Reset("按键重置");

	for (int i = 0; i < 10; i++)
	{
		Leds[1]->Write(ledstat2);
		ledstat2 = !ledstat2;
		Sys.Sleep(300);
	}
}

void IOK026X::FlushLed()
{
	if (LedsShow == 0)			// 启动时候20秒
	{
		auto esp = dynamic_cast<Esp8266*>(Host);
		if (esp && esp->Linked)					// 8266 初始化完成  且  连接完成
		{
			Sys.SetTaskPeriod(LedsTaskId, 500);	// 慢闪
		}

		Leds[0]->Write(ledstat2);
		ledstat2 = !ledstat2;

		if (Sys.Ms() > 20000)
		{
			Leds[0]->Write(false);
			LedsShow = 2;	// 置为无效状态
		}
	}

	bool stat = false;
	// 3分钟内 Client还活着则表示  联网OK
	if (Client && Client->LastActive + 180000 > Sys.Ms() && LedsShow == 1)stat = true;
	Leds[1]->Write(stat);
	if (LedsShow == 2)Sys.SetTask(LedsTaskId, false);
}

byte IOK026X::LedStat(byte showmode)
{
	auto esp = dynamic_cast<Esp8266*>(Host);
	if (esp)
	{
		if (showmode == 1)
		{
			esp->RemoveLed();
			esp->SetLed(*Leds[0]);
		}
		else
		{
			esp->RemoveLed();
			// 维护状态为false
			Leds[0]->Write(false);
		}
	}

	if (showmode != 2)
	{
		if (!LedsTaskId)
		{
			LedsTaskId = Sys.AddTask(&IOK026X::FlushLed, this, 500, 100, "CltLedStat");
			debug_printf("AddTask(IOK026X:FlushLed)\r\n");
		}
		else
		{
			Sys.SetTask(LedsTaskId, true);
			if (showmode == 1)Sys.SetTaskPeriod(LedsTaskId, 500);
		}
		LedsShow = showmode;
	}
	if (showmode == 2)
	{
		// 由任务自己结束，顺带维护输出状态为false
		// if (LedsTaskId)Sys.SetTask(LedsTaskId, false);
		LedsShow = 2;
	}
	return LedsShow;
}

void IOK026X::OnLongPress(InputPort* port, bool down)
{
	if (down) return;

	debug_printf("Press P%c%d Time=%d ms\r\n", _PIN_NAME(port->_Pin), port->PressTime);

	ushort time = port->PressTime;
	auto client = IOK026X::Current->Client;
	//if (time >= 10000 && time < 15000)
	//	Current->Restore();
	//else
	if (time >= 7000)
	{
		if (client) client->Reboot("按键重启");
		Sys.Reboot(1000);
	}
}

/*
NRF24L01+ 	(SPI3)		|	W5500		(SPI2)		|	TOUCH		(SPI3)
NSS			|				NSS			|	PD6			NSS
CLK			|				SCK			|				SCK
MISO		|				MISO		|				MISO
MOSI		|				MOSI		|				MOSI
PE3			IRQ			|	PE1			INT(IRQ)	|	PD11		INT(IRQ)
PD12		CE			|	PD13		NET_NRST	|				NET_NRST
PE6			POWER		|				POWER		|				POWER

ESP8266		(COM4)
TX
RX
PD3			RST
PE2			POWER

TFT
FSMC_D 0..15		TFT_D 0..15
NOE					RD
NWE					RW
NE1					RS
PE4					CE
PC7					LIGHT
PE5					RST

PE13				KEY1
PE14				KEY2

PE15				LED2
PD8					LED1


USB
PA11 PA12
*/
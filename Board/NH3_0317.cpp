﻿#include "NH3_0317.h"

#include "Kernel\Task.h"

#include "Device\Power.h"
#include "Device\WatchDog.h"
#include "Config.h"

#include "Drivers\Esp8266\Esp8266.h"
#include "TokenNet\TokenController.h"
#include "Kernel\Task.h"

NH3_0317* NH3_0317::Current = nullptr;

NH3_0317::NH3_0317()
{
	//LedPins.Add(PA4);
	ButtonPins.Add(PB0);
	LedPins.Add(PA0);

	LedsShow = false;
	LedsTaskId = 0;

	Host	= nullptr;	// 网络主机
	Client	= nullptr;

	Data	= nullptr;
	Size	= 0;
	Current = this;
}

void NH3_0317::Init(ushort code, cstring name, COM message)
{
	auto& sys	= (TSys&)Sys;
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
	//Power::AttachTimeSleep();
#endif

	// Flash最后一块作为配置区
	Config::Current	= &Config::CreateFlash();
}

void* NH3_0317::InitData(void* data, int size)
{
	// 启动信息
	auto hot	= &HotConfig::Current();
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

	Data	= data;
	Size	= size;

	return data;
}

void NH3_0317::InitLeds()
{
	for(int i=0; i<LedPins.Count(); i++)
	{
		auto port	= new OutputPort(LedPins[i]);
		port->Invert = true;
		port->Open();
		port->Write(false);
		Leds.Add(port);
	}
}

void ButtonOnpress(InputPort* port, bool down, void* param)
{
	if (port->PressTime > 1000)
		NH3_0317::OnLongPress(port, down);
}

void NH3_0317::InitButtons(const Delegate2<InputPort&, bool>& press)
{
	for (int i = 0; i < ButtonPins.Count(); i++)
	{
		auto port = new InputPort(ButtonPins[i]);
		port->Index	= i;
		port->Press	= press;
		port->UsePress();
		port->Open();
		Buttons.Add(port);
	}
}

ISocketHost* NH3_0317::Create8266()
{
	// auto host	= new Esp8266(COM2, PB2, PA1);	// 触摸开关的
	auto host	= new Esp8266(COM3, P0, PA5);

	// 初次需要指定模式 否则为 Wire
	bool join	= host->SSID && *host->SSID;
	//if (!join) host->Mode = SocketMode::AP;

	if (!join)
	{
		*host->SSID	= "WSWL";
		*host->Pass = "12345678";

		host->Mode	= SocketMode::STA_AP;
	}
	// 绑定委托，避免5500没有连接时导致没有启动客户端
	host->NetReady.Bind(&NH3_0317::OpenClient, this);

	Client->Register("SetWiFi", &Esp8266::SetWiFi, host);
	Client->Register("GetWiFi", &Esp8266::GetWiFi, host);

	host->OpenAsync();

	return host;
}

/******************************** Token ********************************/

void NH3_0317::InitClient()
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
	// 设置远程地址
	Client->Register("Gateway/SetRemote", &TokenClient::InvokeSetRemote, Client);
	// 获取远程配置信息
	Client->Register("Gateway/GetRemote", &TokenClient::InvokeGetRemote, Client);
	// 获取所有Ivoke命令
	Client->Register("Api/All", &TokenClient::InvokeGetAllApi, Client);

	if (Data && Size > 0)
	{
		auto& ds = Client->Store;
		ds.Data.Set(Data, Size);
	}

	// 如果若干分钟后仍然没有打开令牌客户端，则重启系统
	Sys.AddTask(
		[](void* p){
			auto & bsp = *(NH3_0317*)p;
			auto & client = *bsp.Client;
			if(!client.Opened)
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

void NH3_0317::Register(int index, IDataPort& dp)
{
	if (!Client) return;

	auto& ds = Client->Store;
	ds.Register(index, dp);
}

void NH3_0317::OpenClient(ISocketHost& host)
{
	assert(Client, "Client");

	debug_printf("\r\n OpenClient \r\n");

	auto esp	= dynamic_cast<Esp8266*>(&host);
	if(esp && !esp->Led) esp->SetLed(*Leds[0]);

	auto tk = TokenConfig::Current;

	// STA模式下，主连接服务器
	if (host.IsStation() && esp->Joined && !Client->Master) AddControl(host, tk->Uri(), 0);

	// STA或AP模式下，建立本地监听
	if(Client->Controls.Count() == 0)
	{
		NetUri uri(NetType::Udp, IPAddress::Broadcast(), 3355);
		AddControl(host, uri, tk->Port);
	}                                                                          

	if (!Client->Opened)
		Client->Open();
	else
		Client->AttachControls();
}

TokenController* NH3_0317::AddControl(ISocketHost& host, const NetUri& uri, ushort localPort)
{
	// 创建连接服务器的Socket
	auto socket	= host.CreateRemote(uri);

	// 创建连接服务器的控制器
	auto ctrl	= new TokenController();
	//ctrl->Port = dynamic_cast<ITransport*>(socket);
	ctrl->Socket	= socket;

	// 创建客户端
	auto client	= Client;
	if(localPort == 0)
		client->Master	= ctrl;
	else
	{
		socket->Local.Port	= localPort;
		ctrl->ShowRemote	= true;
		client->Controls.Add(ctrl);
	}

	return ctrl;
}

void NH3_0317::InitNet()
{
	Host	= Create8266();
}

static void OnAlarm(AlarmItem& item)
{
	// 1长度n + 1类型 + 1偏移 + (n-2)数据
	auto bs	= item.GetData();
	debug_printf("OnAlarm ");
	bs.Show(true);

	if(bs[1] == 0x06)
	{
		auto client = NH3_0317::Current->Client;
		client->Store.Write(bs[2], bs.Sub(3, bs[0] - 2));

		// 主动上报状态
		client->ReportAsync(bs[2], bs[0] - 2);
	}
}

void NH3_0317::InitAlarm()
{
	if (!Client) return;

	if (!AlarmObj) AlarmObj = new Alarm();
	Client->Register("Policy/AlarmSet", &Alarm::Set, AlarmObj);
	Client->Register("Policy/AlarmGet", &Alarm::Get, AlarmObj);

	AlarmObj->OnAlarm	= OnAlarm;
	AlarmObj->Start();
}

void NH3_0317::Invoke(const String& ation, const Buffer& bs)
{
	if (!Client) return;

	Client->Invoke(ation, bs);

}

void NH3_0317::Restore()
{
	if(Client) Client->Reset("按键重置");
}

void NH3_0317::FlushLed()
{
	bool stat = false;
	// 3分钟内 Client还活着则表示  联网OK
	if (Client && Client->LastActive + 180000 > Sys.Ms()&& LedsShow)stat = true;
	Leds[1]->Write(stat);
	if (!LedsShow)Sys.SetTask(LedsTaskId, false);
	// if (!LedsShow)Sys.SetTask(Task::Scheduler()->Current->ID, false);
}

bool NH3_0317::LedStat(bool enable)
{
	auto esp = dynamic_cast<Esp8266*>(Host);
	if (esp)
	{
		if (enable)
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
	if (enable)
	{
		if (!LedsTaskId)
			LedsTaskId = Sys.AddTask(&NH3_0317::FlushLed, this, 500, 500, "CltLedStat");
		else
			Sys.SetTask(LedsTaskId, true);
		LedsShow = true;
	}
	else
	{
		// 由任务自己结束，顺带维护输出状态为false
		// if (LedsTaskId)Sys.SetTask(LedsTaskId, false);
		LedsShow = false;
	}
	return LedsShow;
}

void NH3_0317::OnLongPress(InputPort* port, bool down)
{
	if (down) return;

	debug_printf("Press P%c%d Time=%d ms\r\n", _PIN_NAME(port->_Pin), port->PressTime);

	ushort time = port->PressTime;
	auto client	= NH3_0317::Current->Client;
	if (time >= 5000 && time < 10000)
	{
		if(client) client->Reset("按键重置");
	}
	else if (time >= 3000)
	{
		if(client) client->Reboot("按键重启");
		Sys.Reboot(1000);
	}
}

/*
NRF24L01+ 	(SPI3)
NSS			|
CLK			|
MISO		|
MOSI		|
PE3			IRQ
PD12		CE
PE6			POWER

ESP8266		(COM4)
TX
RX
PD3			RST
PE2			POWER

LED1
LED2

KEY1
KEY2

*/

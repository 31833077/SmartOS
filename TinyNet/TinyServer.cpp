﻿#include "Time.h"
#include "TinyServer.h"

#include "JoinMessage.h"

#include "Config.h"

#include "Security\MD5.h"

/******************************** TinyServer ********************************/

static bool OnServerReceived(Message& msg, void* param);

TinyServer::TinyServer(TinyController* control)
{
	Control 	= control;
	Config		= NULL;
	DeviceType	= Sys.Code;

	Control->Received	= OnServerReceived;
	Control->Param		= this;

	Received	= NULL;
	Param		= NULL;

	Current		= NULL;
}

bool TinyServer::Send(Message& msg)
{
	return Control->Send(msg);
}

bool TinyServer::Reply(Message& msg)
{
	return Control->Reply(msg);
}

bool OnServerReceived(Message& msg, void* param)
{
	TinyServer* server = (TinyServer*)param;
	assert_ptr(server);

	// 消息转发
	return server->OnReceive((TinyMessage&)msg);
}

// 常用系统级消息

void TinyServer::Start()
{
	assert_param2(Config, "未指定微网服务器的配置");

	Control->Open();
}

// 收到本地无线网消息
bool TinyServer::OnReceive(TinyMessage& msg)
{
	// 如果设备列表没有这个设备，那么加进去
	byte id = msg.Src;
	Device* dv = Current;
	if(!dv) dv = FindDevice(id);

	switch(msg.Code)
	{
		case 1:
			OnJoin(msg);
			dv = Current;
			break;
		case 2:
			OnDisjoin(msg);
			break;
		case 3:
			// 设置当前设备
			Current = dv;
			OnPing(msg);
			break;
		case 5:
			// 系统指令不会被转发，这里修改为用户指令
			msg.Code = 0x15;
		case 0x15:
			OnReadReply(msg, *dv);
			break;
		case 6:
		case 0x16:
			//OnWriteReply(msg, *dv);
			break;
	}

	// 更新设备信息
	if(dv) dv->LastTime = Time.Current();

	// 设置当前设备
	Current = dv;

	// 消息转发
	if(Received) return Received(msg, Param);

	Current = NULL;

	return true;
}

// 分发外网过来的消息。返回值表示是否有响应
bool TinyServer::Dispatch(TinyMessage& msg)
{
	// 非休眠设备直接发送
	//if(!dv->CanSleep())
	//{
		Send(msg);
	//}
	// 休眠设备进入发送队列
	//else
	//{

	//}

	// 先找到设备
	Device* dv = FindDevice(msg.Dest);
	if(!dv) return false;

	bool rs = false;

	// 缓存内存操作指令
	switch(msg.Code)
	{
		case 5:
		case 0x15:
			rs = OnRead(msg, *dv);
			break;
		case 6:
		case 0x16:
			rs = OnWrite(msg, *dv);
			break;
	}

	// 如果有返回，需要设置目标地址，让网关以为该信息来自设备
	if(rs)
	{
		msg.Dest	= msg.Src;
		msg.Src		= dv->Address;
	}

	return rs;
}

// 组网
bool TinyServer::OnJoin(const TinyMessage& msg)
{
	if(msg.Reply) return false;

	// 如果设备列表没有这个设备，那么加进去
	byte id = msg.Src;
	if(!id) return false;

	ulong now = Time.Current();

	JoinMessage dm;
	dm.ReadMessage(msg);

	// 根据硬件编码找设备
	Device* dv = FindDevice(dm.HardID);
	if(!dv)
	{
		// 以网关地址为基准，进行递增分配
		byte addr = Config->Address;
		{
			id = addr;
			while(FindDevice(++id) != NULL && id < 0xFF);

			debug_printf("发现节点设备 0x%04X ，为其分配 0x%02X\r\n", dm.Kind, id);
			if(id == 0xFF) return false;
		}

		dv = new Device();
		dv->Address	= id;

		Devices.Add(dv);

		// 节点注册
		dv->RegTime	= now;
	}

	// 更新设备信息
	Current		= dv;

	dv->Kind	= dm.Kind;
	dv->HardID	= dm.HardID;
	dv->Version	= dm.Version;

	if(dv->Logins++ == 0) dv->LoginTime = now;
	dv->LastTime = now;

	debug_printf("\r\nTinyServer::新设备第 %d 次组网 TranID=0x%08X \r\n", dv->Logins, dm.TranID);
	dv->Show(true);

	// 生成随机密码。当前时间的MD5
	ByteArray bs((byte*)&now, 8);
	dv->Pass = MD5::Hash(bs);
	dv->Pass.SetLength(8);	// 小心不要超长

	// 响应
	TinyMessage rs;
	rs.Code = msg.Code;
	rs.Dest = msg.Src;
	rs.Sequence	= msg.Sequence;

	// 发现响应
	dm.Reply	= true;

	dm.Server	= Config->Address;
	dm.Channel	= Config->Channel;
	dm.Speed	= Config->Speed / 10;

	dm.Address	= dv->Address;
	dm.Password	= dv->Pass;

	dm.HardID.SetLength(6);	// 小心不要超长
	dm.HardID	= Sys.ID;

	dm.WriteMessage(rs);

	Reply(rs);

	return true;
}

// 读取
bool TinyServer::OnDisjoin(const TinyMessage& msg)
{
	return true;
}

// 心跳保持与对方的活动状态
bool TinyServer::OnPing(const TinyMessage& msg)
{
	// 网关内没有相关节点信息时不鸟他
	if(FindDevice(msg.Src) == NULL)return false;

	// 子操作码
	switch(msg.Data[0])
	{
		// 同步数据
		case 0x01:
		{
			Device* dv = Current;
			if(dv && msg.Length >= 4)
			{
				byte offset	= msg.Data[1];
				byte len	= msg.Data[2];
				debug_printf("设备 0x%02X 同步数据（%d, %d）到网关缓存 \r\n", dv->Address, offset, len);

				int remain = dv->Store.Capacity() - offset;
				if(len > remain) len = remain;
				// 保存一份到缓冲区
				if(len > 0)
				{
					dv->Store.Copy(&msg.Data[3], len, offset);
				}
			}
		}
	}

	// 响应 Ping 指令，告诉客户端有多少指令需要等待执行
	TinyMessage rs;
	rs.Code = msg.Code;
	rs.Dest = msg.Src;
	rs.Sequence	= msg.Sequence;

	//todo。告诉客户端有多少待处理指令

	Reply(rs);

	return true;
}

/*
请求：1起始 + 1大小
响应：1起始 + N数据
错误：错误码2 + 1起始 + 1大小
*/
bool TinyServer::OnRead(TinyMessage& msg, Device& dv)
{
	if(msg.Reply) return false;
	if(msg.Length < 2) return false;

	// 起始地址为7位压缩编码整数
	Stream ms	= msg.ToStream();
	uint offset = ms.ReadEncodeInt();
	uint len	= ms.ReadEncodeInt();

	// 指针归零，准备写入响应数据
	ms.SetPosition(0);

	// 计算还有多少数据可读
	int remain = dv.Store.Length() - offset;
	if(remain < 0)
	{
		// 可读数据不够时出错
		msg.Error = true;
		ms.Write((byte)2);
		ms.WriteEncodeInt(offset);
		ms.WriteEncodeInt(len);
	}
	else
	{
		ms.WriteEncodeInt(offset);
		// 限制可以读取的大小，不允许越界
		if(len > remain) len = remain;
		if(len > 0) ms.Write(dv.Store.GetBuffer(), offset, len);
	}
	msg.Length	= ms.Position();
	msg.Reply	= true;

	return true;
}

// 读取响应，服务端趁机缓存一份。定时上报也是采用该指令。
bool TinyServer::OnReadReply(const TinyMessage& msg, Device& dv)
{
	debug_printf("备份数据到网关") ;
	if(!msg.Reply || msg.Error) return false;
	if(msg.Length < 2) return false;

	// 起始地址为7位压缩编码整数
	Stream ms	= msg.ToStream();
	uint offset = ms.ReadEncodeInt();

	int remain = dv.Store.Capacity() - offset;

	if(remain < 0) return false;

	uint len = ms.Remain();
	if(len > remain) len = remain;
	// 保存一份到缓冲区
	if(len > 0)
	{
		dv.Store.Copy(ms.Current(), len, offset);
	}

	return true;
}

/*
请求：1起始 + N数据
响应：1起始 + 1大小
错误：错误码2 + 1起始 + 1大小
*/
bool TinyServer::OnWrite(TinyMessage& msg, Device& dv)
{
	if(msg.Reply) return false;
	if(msg.Length < 2) return false;

	// 起始地址为7位压缩编码整数
	Stream ms	= msg.ToStream();
	uint offset = ms.ReadEncodeInt();

	// 计算还有多少数据可写
	uint len = ms.Remain();
	int remain = dv.Store.Capacity() - offset;
	if(remain < 0)
	{
		msg.Error = true;

		// 指针归零，准备写入响应数据
		ms.SetPosition(0);

		ms.Write((byte)2);
		ms.WriteEncodeInt(offset);
		ms.WriteEncodeInt(len);

		debug_printf("读写指令错误");
	}
	else
	{
		if(len > remain) len = remain;
		// 保存一份到缓冲区
		if(len > 0)
		{
			dv.Store.Copy(ms.Current(), len, offset);

			// 指针归零，准备写入响应数据
			ms.SetPosition(0);
			ms.WriteEncodeInt(offset);
			// 实际写入的长度
			ms.WriteEncodeInt(len);

			debug_printf("读写指令转换");
		}
	}
	msg.Length	= ms.Position();
	msg.Reply	= true;
	msg.Show();

	return true;
}

Device* TinyServer::FindDevice(byte id)
{
	if(id == 0) return NULL;

	for(int i=0; i<Devices.Count(); i++)
	{
		if(id == Devices[i]->Address) return Devices[i];
	}

	return NULL;
}

Device* TinyServer::FindDevice(const ByteArray& hardid)
{
	if(hardid.Length() == 0) return NULL;

	for(int i=0; i<Devices.Count(); i++)
	{
		if(hardid == Devices[i]->HardID) return Devices[i];
	}

	return NULL;
}

bool TinyServer::DeleteDevice(byte id)
{
	Device* dv = FindDevice(id);
	if(dv && dv->Address == id)
	{
		Devices.Remove(dv);
		delete dv;
		return true;
	}

	return false;
}

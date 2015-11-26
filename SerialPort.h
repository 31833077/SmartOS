﻿#ifndef __SerialPort_H__
#define __SerialPort_H__

#include "Sys.h"
#include "Port.h"
#include "Task.h"
#include "Queue.h"
#include "Power.h"
#include "Net\ITransport.h"

#define SERIAL_BAUDRATE 1024000

// 串口类
class SerialPort : public ITransport, public Power
{
private:
	byte _index;
	byte _parity;
	byte _dataBits;
	byte _stopBits;
	int _baudRate;
	int _byteTime;	// 传送一字节耗时 （略微加大一些）

    USART_TypeDef* _port;
	AlternatePort _tx;
#if defined(STM32F0) || defined(STM32F4)
	AlternatePort _rx;
#else
	InputPort _rx;
#endif

	void Init();

public:
	char 		Name[5];// 名称。COMx，后面1字节\0表示结束
#ifdef STM32F1XX
    bool		IsRemap;// 是否重映射
#endif
	OutputPort* RS485;	// RS485使能引脚
	int 		Error;	// 错误计数

	// 收发缓冲区
#ifndef STM32F0
	Queue Tx;
#endif
	Queue Rx;

	SerialPort();
    SerialPort(COM_Def index,
        int baudRate = SERIAL_BAUDRATE,
        byte parity = USART_Parity_No,       //无奇偶校验
        byte dataBits = USART_WordLength_8b, //8位数据长度
        byte stopBits = USART_StopBits_1)    //1位停止位
	{
		Init();
		Init(index, baudRate, parity, dataBits, stopBits);
	}

    SerialPort(USART_TypeDef* com,
        int baudRate = SERIAL_BAUDRATE,
        byte parity = USART_Parity_No,       //无奇偶校验
        byte dataBits = USART_WordLength_8b, //8位数据长度
        byte stopBits = USART_StopBits_1);    //1位停止位
	// 析构时自动关闭
    virtual ~SerialPort();

    void Init(byte index,
        int baudRate = SERIAL_BAUDRATE,
        byte parity = USART_Parity_No,       //无奇偶校验
        byte dataBits = USART_WordLength_8b, //8位数据长度
        byte stopBits = USART_StopBits_1);    //1位停止位

	uint SendData(byte data, uint times = 3000);

    bool Flush(uint times = 3000);

	void SetBaudRate(int baudRate = SERIAL_BAUDRATE);

    void GetPins(Pin* txPin, Pin* rxPin);

    virtual void Register(TransportHandler handler, void* param = NULL);

	// 电源等级变更（如进入低功耗模式）时调用
	virtual void ChangePower(int level);

	virtual string ToString() { return Name; }

	static SerialPort* GetMessagePort();
protected:
	virtual bool OnOpen();
    virtual void OnClose();

    virtual bool OnWrite(const Array& bs);
	virtual uint OnRead(Array& bs);

private:
	static void OnHandler(ushort num, void* param);
	void OnTxHandler();
	void OnRxHandler();

	Task*	_task;
	uint	_taskidRx;
	static void ReceiveTask(void* param);
};

#endif

#ifndef _W5500_H_
#define _W5500_H_

#include "Sys.h"
#include "Type.h"
#include "List.h"
#include "Stream.h"
#include "Spi.h"
#include "Net\ITransport.h"
//#include "Thread.h"


// Ӳ��Socket����
class HardwareSocket;

// ����֡��ʽ
// 2byte Address + 1byte CONFIG_Phase + nbyte Data Phase
typedef struct
{
	ushort	Address;
	byte	Config;
	Stream	Data;
	void Clear(){ ArrayZero2(this,3);};
}Frame;


class W5500 //: public ITransport // ֻ�߱�IP �Լ������������  ���߱�Socket�������� ���Բ���ITransport
{	
	// ͨ�üĴ����ṹ
	struct{
		byte MR;			// ģʽ			0x0000
		byte GAR[4];		// ���ص�ַ		0x0001
		byte SUBR[4];		// ��������		0x0005
		byte SHAR[6];		// ԴMAC��ַ	0x0009
		byte SIPR[4];		// ԴIP��ַ		0x000f
		byte INTLEVEL[2];	// �͵�ƽ�ж϶�ʱ���Ĵ���	0x0013
		byte IR;			// �жϼĴ���				0x0015
		byte IMR;			// �ж����μĴ���			0x0016
		byte SIR;			// Socket�жϼĴ���			0x0017
		byte SIMR;			// Socket�ж����μĴ���		0x0018
		byte RTR[2];		// ����ʱ��					0x0019
		byte RCR;			// ���Լ���					0x001b
		byte PTIMER;		// PPP ���ӿ���Э������ʱ�Ĵ���	0x001c
		byte PMAGIC;		// PPP ���ӿ���Э������Ĵ���		0x001d
		byte PHAR[6];		// PPPoE ģʽ��Ŀ�� MAC �Ĵ���		0x001e
		byte PSID[2];		// PPPoE ģʽ�»Ự ID �Ĵ���		0x0024
		byte PMRU[2];		// PPPoE ģʽ�������յ�Ԫ			0x0026
		byte UIPR[4];		// �޷��ִ� IP ��ַ�Ĵ�����ֻ����	0x0028
		byte UPORTR[2];		// �޷��ִ�˿ڼĴ�����ֻ����		0x002c
		byte PHYCFGR;		// PHY ���üĴ���					0x002e
		//byte VERSIONR		// оƬ�汾�Ĵ�����ֻ����			0x0039	// ��ַ������
	}General_reg;			// ֻ��һ�� ����ֱ�Ӷ���ͺ�
	
private:
	Spi* _spi;
    InputPort	_IRQ;
	
	HardwareSocket* _socket[8];	// 8��Ӳ��socket
	
	int _Lock;			// �շ���������ȷ��ͬʱֻ��һ������ʹ��
	// ��д֡��֡�������ⲿ����   ������֡�����ڲ��Ķ�д��־��
	bool WriteFrame(Frame& fra);
	bool ReadFrame(Frame& fra,uint length);
	
	byte PhaseOM;		// spi ģʽ��Ĭ�ϱ䳤��
public:
	// �Ǳ�Ҫ�����ⲿ����  ����ֻ��һ��ָ��
	OutputPort* nRest;
	void Reset();
	
	// ����
	W5500();
    W5500(Spi* spi, Pin irq = P0);
    ~W5500();
	
	// ��ʼ��
	void Init();
    void Init(Spi* spi, Pin irq = P0);
	
	// �ص�
	//virtual void Register(TransportHandler handler, void* param = NULL);
private:
	static void OnIRQ(Pin pin, bool down, void* param);		// �жϽŻص�
	void OnIRQ();

public:
	string ToString() { return "W5500"; }
	
	byte GetSocket();
	void Register(byte Index,HardwareSocket* handler);
//protected:
//	virtual bool OnOpen();
//    virtual void OnClose();
//
//    virtual bool OnWrite(const byte* buf, uint len);
//	virtual uint OnRead(byte* buf, uint len);
	
};


// Ӳ��Socket����s
class HardwareSocket
{
private:
	W5500*	_THard;	// Ӳ��������
public:
	//IP_TYPE	Type;	// ����
	bool Enable;	// ����
	byte Index;		// ʹ�õ�ӲSocket���

	HardwareSocket(W5500* thard);
	virtual ~HardwareSocket();

	// �������ݰ�
	virtual bool Process(Stream& ms) = 0;
};

#endif

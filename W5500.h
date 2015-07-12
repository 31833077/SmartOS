#ifndef _W5500_H_
#define _W5500_H_

#include "Sys.h"
#include "Type.h"
#include "List.h"
#include "Stream.h"
#include "Spi.h"
#include "Net\ITransport.h"
//#include "Thread.h"

#if defined(W500DEBUG)

#define w5500_printf printf

#else

__inline void w5500_printf( const char *format, ... ) {}

#endif

// Ӳ��Socket����
class HardwareSocket;

// ����֡��ʽ
// 2byte Address + 1byte CONFIG_Phase + nbyte Data Phase
typedef struct
{
	ushort	Address;
	byte	BSB;		// 5λ    CONFIG_Phase �ɵ��·�װ  ����ֻ��Ҫ֪��BSB�ͺ�
	Stream	Data;
	void Clear(){ ArrayZero2(this,3);};
}Frame;

class W5500 //: public ITransport // ֻ�߱�IP �Լ������������  ���߱�Socket�������� ���Բ���ITransport
{	
	// ͨ�üĴ����ṹ
	struct T_GenReg{
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
	// �շ���������ȷ��ͬʱֻ��һ������ʹ��
	volatile byte _Lock;
	// ���� ip �Ƿ���Dhcp�õ��� 1 ��  0 ����
	//byte IsDhcpIp;
	
	Spi* _spi;
    InputPort	_IRQ;
	// 8��Ӳ��socket
	HardwareSocket* _socket[8];	
	// mac����
	MacAddress _mac;
	IPAddress _ip;
	// ��д֡��֡�������ⲿ����   ������֡�����ڲ��Ķ�д��־��
	bool WriteFrame(Frame& fra);
	bool ReadFrame(Frame& fra,uint length);
	// spi ģʽ��Ĭ�ϱ䳤��
	byte PhaseOM;
	byte RX_FREE_SIZE;	// ʣ����ջ��� kbyte
	byte TX_FREE_SIZE;	// ʣ�෢�ͻ��� kbyte
public:
	// rst���ſ��ܲ��Ƕ����  ����ֻ��һ��ָ��
	OutputPort* nRest;
	// DHCP������IP
	//IPAddress DHCPServer;
	//IPAddress DNSServer;
	
	// �����λ
	void SoftwareReset();
	// ��λ ����Ӳ����λ�������λ
	void Reset();
	
	// ����
	W5500();
    W5500(Spi* spi, Pin irq = P0 ,OutputPort* rst = NULL);	// ����߱���λ���� ����Ĵ������ܶ�
    ~W5500();
	
	// ��ʼ��
	void Init();
    void Init(Spi* spi, Pin irq = P0, OutputPort* rst = NULL);	// ������� rst ��������
	// ����״̬���
	void StateShow();
	
	// ����PHY״̬  �����Ƿ���������
	bool CheckLnk();
	// ���������·��״̬
	void PhyStateShow();
	
	// ���ñ���MAC
	bool SetMac(MacAddress& mac);
	// �������һ��MAC  ������
	void AutoMac();
	// ���� MacAddress
	MacAddress Mac();
	
	// ��������IP
	void SetGateway(IPAddress& ip);
	// ����Ĭ������IP
	void DefGateway();
	// ��ȡ����IP
	IPAddress GetGateway(){ _ip.Value =  *(uint*)General_reg.GAR; return _ip; };
	
	// ��������
	void SetIpMask(IPAddress& mask);
	// ����Ĭ����������
	void DefIpMask();
	// ��ȡ��������
	IPAddress GetIpMask(){ _ip.Value =  *(uint*)General_reg.SUBR; return _ip; };
	
	// �����Լ���IP
	void SetMyIp(IPAddress& ip);
	// ��ȡ�Լ���IP
	IPAddress GetMyIp(){ _ip.Value =  *(uint*)General_reg.SIPR; return _ip; };
	
	/* ��ʱʱ�� = ����ʱ��*���Դ���  */
	// ��������ʱ��		��ʱ�ش�/������ʱ�ж�	��� 6553ms		��Ĭ��200ms��
	void SetRetryTime(ushort ms);
	// �������Դ���		��ʱ�ش��Ĵ���			���256			��Ĭ��8�Σ�
	void SetRetryCount(byte count);
	
	// �ж�ʱ�͵�ƽ����ʱ��
	void SetIrqLowLevelTime(int us);
	
	// ����PINGӦ��
	void OpenPingACK();
	void ClosePingACK();
	
	//void OpenWol();		// ���绽��
	void Recovery();
private:
	// �жϽŻص�
	static void OnIRQ(Pin pin, bool down, void* param);		
	void OnIRQ();

public:
	string ToString() { return "W5500"; }
	
	byte GetSocket();
	void Register(byte Index,HardwareSocket* handler);
};

// Ӳ��Socket������
class HardwareSocket
{
public:
	enum Protocol
	{
		CLOSE	= 0x00,
		TCP		= 0x01,
		UDP		= 0x02,
		MACRAW	= 0x04,
	};
private:	
	struct T_HSocketReg{
		byte Sn_MR ;		//0x0000  	// Socket ģʽ�Ĵ���
		byte Sn_CR ;		//0x0001  	// ���üĴ��� 	����Ϊ���⡿��ֻд����Ϊ0x00��
		byte Sn_IR ;		//0x0002  	// �жϼĴ���	 д1��0����
		byte Sn_SR ;		//0x0003  	// ״̬�Ĵ���	��ֻ����
		byte Sn_PORT[2] ;	//0x0004  	// TCP UDP ģʽ�¶˿ں�  OPEN֮ǰ���ú�
		byte Sn_DHAR[6] ;	//0x0006  	// Ŀ��MAC,SEND_MACʹ��;CONNECT/SEND ����ʱARP��ȡ����MAC
		byte Sn_DIPR[4] ;	//0x000c  	// Ŀ��IP��ַ
		byte Sn_DPORT[2] ;	//0x0010  	// Ŀ��˿�
		byte Sn_MSSR[2] ;	//0x0012  	// TCP UDP ģʽ�� MTU ����䵥Ԫ��С  Ĭ�����ֵ
										// TCP:1460; UDP:1472; MACRAW:1514;
										// MACRAW ģʽʱ ����MTU �����ڲ�����Ĭ��MTU������Ч
										// PPPoE ģʽ�� ��
										// TCP UDP ģʽ�£��������ݱ� MTU��ʱ�����ݽ����Զ����ֳ�Ĭ��MTU ��Ԫ��С
		byte Reserved ;		//0x0014  	
		byte Sn_TOS ;		//0x0015  	// IP��ͷ �������� 	OPEN֮ǰ����
		byte Sn_TTL ;		//0x0016  	// ����ʱ�� TTL 	OPEN֮ǰ����
		byte Reserved2[7] ;	//0x0017  	-  0x001d
		byte Sn_RXBUF_SIZE ;//0x001e  	// ���ջ����С   1 2 4 8 16  ��λKByte
		byte Sn_TXBUF_SIZE ;//0x001f  	// ���ͻ����С   1 2 4 8 16  ��λKByte
		byte Sn_TX_FSR[2] ;	//0x0020  	// ���з��ͼĴ�����С
		byte Sn_TX_RD[2] ;	//0x0022  	// ���Ͷ�����ָ��
		byte Sn_TX_WR[2] ;	//0x0024  	// ����д����ָ��
		byte Sn_RX_RSR[2] ;	//0x0026  	// ���н��ռĴ�����С
		byte Sn_RX_RD[2] ;	//0x0028  	// ���Ͷ�����ָ��
		byte Sn_RX_WR[2] ;	//0x002a  	// ����д����ָ��
		byte Sn_IMR ;		//0x002c  	// �ж����μĴ���  �ṹ��Sn_IRһ�� 0����  1������
		byte Sn_FRAG[2] ;	//0x002d  	// IP��ͷ �ֶβ���  �ֶμĴ���
		
		byte Sn_KPALVTR ;	//0x002f  	// ֻ��TCPģʽ��ʹ��  ����ʱ��Ĵ���  ��λ��5s
										// Ϊ0 ʱ  �ֶ�SEND_KEEP
										// > 0 ʱ  ����SEND_KEEP����
	}HSocketReg;
private:
	W5500*	_THard;	// W5500�������ֿ�����
public:
	bool Enable;	// ����
	byte Index;		// ʹ�õ�ӲSocket���   Ҳ��BSBѡ���һ����
	
	HardwareSocket(W5500* thard);
	virtual ~HardwareSocket();
	// ��Socket
	virtual bool OpenSocket() = 0;
	// �ָ�����
	virtual void Recovery() = 0;
	// �������ݰ�
	virtual bool Process() = 0;
};

#endif

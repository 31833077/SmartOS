
#include "W5500.h"

/*
Ӳ�����ò���
���� PMODE[2:0]
	000		10BI��˫�����ر��Զ�Э��
	001		10BIȫ˫�����ر��Զ�Э��
	010		100BI��˫�����ر��Զ�Э��
	011		100BIȫ˫�����ر��Զ�Э��
	100		100BI��˫���������Զ�Э��
	101		δ����
	110		δ����
	111		���й��ܣ������Զ�Э��
RSTn	����	���͵�ƽ��Ч���͵�ƽʱ������500us������Ч��
InTn	�ж����	���͵�ƽ��Ч��--- ������ IRQ ����
֧��SPI ģʽ0��ģʽ3
*/
/*
�ڲ��ṹ
һ��ͨ�üĴ�����
	0x0000-0x0039	����Ѱַ��BSBѡ��
	IP MAC ��������Ϣ
�˸�Socket�Ĵ�����
	0x0000-0x0030	����Ѱַ��BSBѡ��
8��Socket�շ�����	�ܻ���32K
	������һ��16K(0x0000-0x3fff)  	��ʼ������Ϊ ÿ��Socket 2K
	�ջ���һ��16K(0x0000-0x3fff)  	��ʼ������Ϊ ÿ��Socket 2K
	�ٷ���ʱ  �շ�������Խ16K ��߽�


*/


// ����֡��ʽ
// 2byte Address + 1byte CONFIG_Phase + nbyte Data Phase

// �ɱ����ݳ���ģʽ�£�SCSn ���ʹ���W5500��SPI ֡��ʼ����ַ�Σ������߱�ʾ֡����   �����ܸ�SPI NSS ��һ����
	class ByteStruct
	{
	public:
		void Init(byte data = 0) { *(byte*)this = data; }
		byte ToByte() { return *(byte*)this; }
	};
	// λ�� ��λ��ǰ
	// ���üĴ���0x00
	typedef struct : ByteStruct
	{
		byte OM:2;		// SPI ����ģʽ
						// 00	�ɱ����ݳ���ģʽ��N�ֽ����ݶΣ�1<=N��
						// 01	�̶����ݳ���ģʽ��1�ֽ����ݳ��ȣ�N=1��
						// 10	�̶����ݳ���ģʽ��1�ֽ����ݳ��ȣ�N=2��
						// 11	�̶����ݳ���ģʽ��1�ֽ����ݳ��ȣ�N=4��
						// �̶�ģʽ����ҪNSS ��оƬNSS �ӵأ� �ɱ�ģʽSPI NSS �ܿ�
						
		byte RWB:1;		// 0����	1��д
		
		byte BSB:5;		// ����ѡ��λ��		// 1��ͨ�üĴ���   8��Socket�Ĵ����飨ѡ��д���棬�ջ��棩
						// 00000	ͨ��
						// XXX01	SocketXXX ѡ��
						// XXX10	SocketXXX ������
						// XXX11	SocketXXX �ջ���
						// ���ౣ���� ���ѡ���˱����Ľ��ᵼ��W5500����
	}CONFIG_Phase;


W5500::W5500() { Init(); };

W5500::W5500(Spi* spi, Pin irq)
{
	Init();
	Init(spi, irq);
}

void W5500::Reset()
{
	if(nRest)
	{
		*nRest = false;		// �͵�ƽ��Ч
		Sys.Delay(600);		// ����500us
		*nRest = true;
	}
}

void W5500::Init()
{
	_spi = NULL;
	nRest = NULL;
	memset(&General_reg,0,sizeof(General_reg));
	memset(&_socket,NULL,sizeof(_socket)/sizeof(_socket[0]));
	PhaseOM = 0x00;
}

void W5500::Init(Spi* spi, Pin irq)
{
	if(irq != P0)
	{	
		debug_printf("W5500::Init IRQ=P%c%d\r\n",_PIN_NAME(irq));
		_IRQ.ShakeTime = 0;
		_IRQ.Floating = false;
		_IRQ.PuPd = InputPort::PuPd_UP;
		_IRQ.Set(irq);
		_IRQ.Register(OnIRQ, this);
	}
	if(spi)
		_spi = spi;
	else 
	{
		debug_printf("SPI ERROR\r\n");
		return;
	}
	_spi->Open();
	Reset();
	
	Frame fra;
	fra.Address = 0x0000;
	
	CONFIG_Phase config_temp;
	config_temp.BSB = 0x00;		// �Ĵ�����
	fra.Config = config_temp.ToByte();
	
	ReadFrame(fra,sizeof(General_reg));
	fra.Data.Read((byte *)&General_reg,0,sizeof(General_reg));
}



bool W5500::WriteFrame(Frame& frame)
{
	SpiScope sc(_spi);
	byte temp = frame.Address>>8;	// ��ַ��λ��ǰ
	_spi->Write(temp);
	temp = frame.Address;
	_spi->Write(temp);
	
	CONFIG_Phase config_temp;		// ���ö�д��OM
	config_temp.Init(frame.Config);
	config_temp.OM = PhaseOM;
	config_temp.RWB = 1;
	frame.Config = config_temp.ToByte();
	_spi->Write(frame.Config);
	
	for(uint i = 0;i < frame.Data.Length;i ++)
	{
		_spi->Write(frame.Data.Read<byte>());
	}
	return true;
}

bool W5500::ReadFrame(Frame& frame,uint length)
{
	SpiScope sc(_spi);
	byte temp = frame.Address>>8;	// ��ַ��λ��ǰ
	_spi->Write(temp);
	temp = frame.Address;
	_spi->Write(temp);
	
	CONFIG_Phase config_temp;		// ���ö�д��OM
	config_temp.Init(frame.Config);
	config_temp.OM = PhaseOM;
	config_temp.RWB = 0;
	frame.Config = config_temp.ToByte();
	_spi->Write(frame.Config);

	for(uint i = 0;i < length; i++)
		frame.Data.Write<byte>(_spi->Write(0xff));
		
	return true;
}

byte W5500::GetSocket()
{
	for(byte i = 0;i < 8;i ++)
	{
		if(_socket[i] == NULL)return i ;
	}
	debug_printf("û�п����Socket������ !\r\n");
	return 0xff;
}

void W5500::Register(byte Index,HardwareSocket* handler)
{
	if(_socket[Index] == NULL)
		{
			debug_printf("Index: %d ��ע�� !\r\n",Index);
			_socket[Index] = handler;
		}
	else
		_socket[Index] = NULL;
}

// irq �жϴ�����
void W5500::OnIRQ(Pin pin, bool down, void* param)
{
	if(!down)return;
	W5500* send = (W5500*)param;
	send->OnIRQ();
}

void W5500::OnIRQ()
{
}

/******************************************************************/

HardwareSocket::HardwareSocket(W5500* thard)
{
	if(!thard)
	{
		_THard = thard;
		Index = _THard->GetSocket();
		if(Index < 8)
			_THard->Register(Index , this);
	}
}

HardwareSocket::~HardwareSocket()
{
}






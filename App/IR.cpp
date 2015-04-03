
#include "IR.h"

IR::IR(PWM * pwm ,Pin Outio,Pin Inio)
{
	_irf = NULL;
	_Outio= NULL;
	
	_stat = Over;
	_mode = idle;
	
	_timer = NULL;
	_timerTick = 0x00000;
	_buff = NULL;
	_length = 0;
	
	if(pwm)_irf = pwm;
	if(Outio!=P0) _Outio = new OutputPort(Outio,false,true);
	if(Inio != P0) _Inio = new InputPort(Inio);
	SetIRL();
}

bool IR::SetMode(IRMode mode)
{
	if(_mode == mode)return true;
	if(mode != idle)
	{
		// ����һ����ʱ��
		if(_timer==NULL)
			_timer = Timer::Create();
		// ���ö�ʱ���Ĳ���
		if(_timer==NULL)return false;
		_timer->SetFrequency(20000);  // 50us
		_timer->Register(_timerHandler,this);
		
		_mode = mode;
		
		if(mode == receive)	_stat = WaitRec;
		else 				_stat = WaitSend;
	}
	else
	{
		if(_timer!=NULL)
		{
			_timer->Stop();
			_timer->Register((EventHandler)NULL,NULL);
//			delete(_timer);		// ��һ��Ҫ�ͷŵ�
			_stat=Over;
		}
	}
	return true;
}

bool IR::Send(byte *sendbuf,int length)
{
	if(!_Outio)return false;
	if(!_irf)return false;
	if(length<2)return false;
	if(*sendbuf != (byte)length)return false;
	
	_timerTick=0x00000;
	_buff = sendbuf+1;
	_length = length-1;
	if(!SetMode(send))return false;
	//_mode = send;
	
	_nestOut = true;
	_irf->Start();
//Ų������һ��	SetIRH();
	_timer->Start();	// ��һ��ִ��ʱ��Ƚϳ�   Ӱ���һ������ ��ô��
	/*SetIRH();*/*_Outio=true;		// �ŵ������� ��ʱ����������ִ��ʱ����ɵ�Ӱ�����Ա�С
	_stat = Sending;
	
	// �ȴ��������
	for(int i=0;i<2;i++) 
	{
		Sys.Sleep(500);  // ��500ms
		if(_stat == Over)break;
	}
	if(_stat == Sending)
		Sys.Sleep(200);  // ������ڽ����� �ٵ���200ms
	if(_stat == Sending)	
	{
		_stat = SendError;	// ���ڽ��� ��ʾ����
		_timer->Stop();
		return false;
	}
	*_Outio=false;
	_irf->Stop();
	
	return true;
}

int IR::Receive(byte *buff)
{
	if(!_Inio)return -2;
	SetMode(receive);
	_timerTick = 0x0000;
	
	_buff = buff;
	_length = 0;
	_timer->Start();
	// �ȴ�5s  ���û���ź� �ж�Ϊ��ʱ
	for(int i=0;i<10;i++) 
	{
		Sys.Sleep(500);  // ��500ms
		if(_stat == Over)break;
	}
	if(_stat == Recing)
		Sys.Sleep(500);  // ������ڽ����� �ٵ���500ms
	if(_stat == Recing)	
		_stat = RecError;	// ���ڽ��� ��ʾ����

	if(_stat == RecError) return -2;
	
	buff[0] = _length;					// ��һ���ֽڷų���
	if(_length == 0)return -1;
	if(_length <=5)return -2;			// ����̫�� Ҳ��ʾ ���ճ���
	return _length;
}

// ��̬�жϲ���
void  IR::_timerHandler(void* sender, void* param)
{
	IR* ir = (IR*)param;
	ir->SendReceHandler(sender,NULL);
}

// �������жϺ���  �������Ҫ����
const bool ioIdleStat = true;
void IR::SendReceHandler(void* sender, void* param)
{
	volatile static bool ioOldStat = ioIdleStat;
	// �ȴ����յ�ʱ��  ֻ���ж�һ�µ�ƽ״̬ ���û���ź�ֱ���˳�
	// ��ʱ ���ж��⴦��
	if(_stat == WaitRec)	
	{
		if(*_Inio == ioIdleStat)	return;
		else 			_stat = Recing;
	}
	
	_timerTick++;
	switch(_mode)
	{
		case receive:
		{
			if(*_Inio != ioOldStat)
			{
				ioOldStat = *_Inio;
				*_buff++ = (byte)_timerTick;
				_length ++;
				if(_length>500)	// ����500�������� ��ʾ����
				{
					_timer ->Stop();
					_stat = RecError;
				}
				_timerTick = 0x0000;
				break;
			}
		}break;
		
		case send:
		{
			if(*_buff == _timerTick)
			{
				if(_nestOut)	/*SetIRH();*/*_Outio=false;
				else 			/*SetIRL();*/*_Outio=true;
				
				_nestOut = !_nestOut;
				
				_timerTick = 0x0000;
				_buff ++;
				_length --;
				if(_length == 0)
				{
					_timerTick=0x0000;
					_irf->Stop();
					_timer ->Stop();
					_stat = Over;
				}
			}
			else return;
		}break;
		default: _mode = idle;break ;
	}
	
	// ������ ���� 25.5ms �Ķ���  ���۷��ͻ��ǽ���
	// ����ģʽֱ�� �رն�ʱ��
	if(_timerTick >254 || _mode == idle)	
	{
		_timerTick=0x0000;
		_timer ->Stop();
		if(_stat == Recing)
		{
			_stat = Over;		// ����ʱ��ʱ�ͱ�ʾ���ս���
		}
	}
}



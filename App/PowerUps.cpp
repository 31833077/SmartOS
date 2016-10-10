
#include "PowerUps.h"
#include "../Config.h"


class PowerUpsCfg : public ConfigBase
{
public:
	byte	Length;			// ���ݳ���
	byte	ReStartCount;	// ��������
	byte	TagEnd;			// ������������ʶ��

	PowerUpsCfg();

	virtual void Init();
};

PowerUpsCfg::PowerUpsCfg() : ConfigBase()
{
	_Name = "PowUps";
	_Start = &Length;
	_End = &TagEnd;
	Init();
}

void PowerUpsCfg::Init()
{
	Length = Size();
	ReStartCount = 0;
}

/****************************************************************/
/****************************************************************/

PowerUps::PowerUps(byte thld, int timeMs, Func act)
{
	ReStThld = thld;
	if (act)		// û�ж��� ֱ�Ӻ���Flash����
	{
		PowerUpsCfg pc;
		pc.Load();

		ReStartCount = ++pc.ReStartCount;
		debug_printf("ReStartCount %d\r\n",ReStartCount);

		pc.Save();

		Act = act;
	}
	// ����������һЩ����
	Sys.AddTask(DelayAct, this, timeMs, -1, "������ֵ");
}

void PowerUps::DelayAct(void* param)
{
	auto* pu = (PowerUps*) param;
	if (pu->Act)
	{
		if (pu->ReStartCount > pu->ReStThld)pu->Act();
		//pu->ReStartCount = 0;

		PowerUpsCfg pc;
		pc.Load();
		pc.ReStartCount = 0;
		pc.Save();
	}
	// ���ʹ�� ��������
	delete pu;
}

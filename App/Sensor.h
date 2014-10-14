#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "Sys.h"
#include "Port.h"


class Sensor
{

public:
	
	
	string Name;  //����
	int Index;   //������

	
	OutputPort* Led;	// ָʾ��
	OutputPort* Buzzer;	// ָʾ��
	
	InputPort*  Key;	// ���밴��
	InputPort* Pir;	   //�����Ӧ
	InputPort* Mag;	   //�Ŵ�
	
	//I2C*   Ifrared    //����ת��
	
	

	// ���캯����ָʾ�ƺͼ̵���һ�㿪©�������Ҫ����
	Sensor() { Init(); }
	Sensor(Pin key, Pin led = P0, bool ledInvert = true, Pin buzzer = P0, bool keyInvert = true);
	Sensor(Pin key, Pin led = P0, Pin relay = P0);
	~Sensor();

	bool GetValue();
	void SetValue(bool value);

	void Register(EventHandler handler, void* param = NULL);


	
private:
	void Init();

	static void OnPress(Pin pin, bool down, void* param);
	void OnPress(Pin pin, bool down);

	EventHandler _Handler;
	void* _Param;
	
private:
	bool _Value; // ״̬
};

#endif

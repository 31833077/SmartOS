#include "ADC.h"
#include "sys.h"
#include "Interrupt.h"
#include "Pin.h"

AnalogInput:: AnalogInput(ADC_Channel line)
{
	if(line<=0x7f)
		Port::SetPort((Pin)line);		//��Ҫǿת
//	else
		
}

void AnalogInput::OnConfig()
{
}


int AnalogInput::Read()
{
	return AnalogValue[_ChannelNum];
}

#include "ADC.h"
#include "sys.h"
#include "Interrupt.h"
#include "Pin.h"

ADConverter::ADC_Channel  _PinList[18];  // ��¼ע��˳�� ����ADC ÿ��ADC������6��ͨ��
//volatile byte  _ADC_ChanelNum[3]={0,0,0};  // ��¼ÿ��ADC �ж��ٸ�ͨ������   ->ADC_NbrOfChannel  �˲�������
volatile int _isChangeEvent=0x00000000;	//ʹ��18λ ��־ͨ���Ƿ��޸�˳��

static ADC_TypeDef * const g_ADCs[3]= {ADC1,ADC2,ADC3};	// flash��

ADC_Stru * adc_IntStrs[3]={NULL,NULL,NULL}; // ����ADC����

byte ADConverter::isSmall()
{	// ����ɳ�
	if(adc_IntStrs[0] == NULL)return 0;
	if(adc_IntStrs[1] == NULL)return 1;
	if(adc_IntStrs[2] == NULL)return 2;
	// �Ѿ��ж����ʱ��  �ж�line��Ŀ
	if(adc_IntStrs[1]->adc_intstr.ADC_NbrOfChannel <= adc_IntStrs[2]->adc_intstr.ADC_NbrOfChannel)
	{
		if(adc_IntStrs[0]->adc_intstr.ADC_NbrOfChannel <= adc_IntStrs[1]->adc_intstr.ADC_NbrOfChannel)return 0;
		else  return 1;
	}
	else
	{
		if(adc_IntStrs[0]->adc_intstr.ADC_NbrOfChannel <= adc_IntStrs[2]->adc_intstr.ADC_NbrOfChannel)return 0;
		else  return 2;
	}
}

void ADConverter::ReallyChannel(ADC_Channel ParameterChannel)
{
	if(ParameterChannel < PA10)
	{
		_ReallyChannel=(byte)ParameterChannel;
		return ;
	}
	if(ParameterChannel < PC6)
	{
		byte _ReallyChannel = (byte)ParameterChannel ;
		_ReallyChannel &= 0x0f;
		_ReallyChannel += 0x0a;
		return ;
	}
	if(ParameterChannel == Vrefint)
	{
		_ReallyChannel=0x10;
		return ;
	}
	if(ParameterChannel == Vrefint)
	{
		_ReallyChannel=0x10;
		//return ;
	}
}

void ADConverter:: RegisterDMA(ADC_Channel lime)	
{
}

ADConverter:: ADConverter(ADC_Channel line)
{
	assert_param( (line < PA10) || ((PC0 <= line) && (line < PC6)) || (0x80 == line)||(line == 0x81) );
	ReallyChannel(line);
	byte _ADC_group = isSmall();	// ѡȡ���������ADC
	ADC_TypeDef * _ADC = g_ADCs[_ADC_group];	// ȡ�ö�������Ӧ��ADC���üĴ�����ַ  ���ñ������
	
	if(adc_IntStrs[_ADC_group] == NULL)		//û�г�ʼ�� ADC ��ʼ������class
	{
		adc_IntStrs[_ADC_group] = new ADC_Stru(_ADC_group);	// ����һ��ADC������ṹ��class   ��ʼ�������ɹ��캯�����
//		ADC_InitTypeDef * adc_intstr = & adc_IntStrs[_ADC_group]->adc_intstr;	// ȡ�ö���
		RCC_ADCCLKConfig(RCC_PCLK2_Div6);// ʱ�ӷ�Ƶ��	  ��Ҫ����ʱ�ӽ��е���    ������
		ADC_DeInit(_ADC);	// ��λ��Ĭ��ֵ
	}
		ADC_InitTypeDef * adc_intstr = & adc_IntStrs[_ADC_group]->adc_intstr; // ȡ�ö���
		ADC_DeInit(_ADC);	// ÿ���޸�ͨ������ʱ���ȸ�λ	// �Ƿ�����Ҫ ��ȷ��   ������
	
		/*  ADC_ScanConvMode:
		 DISABLE	��ͨ��ģʽ
		 ENABLE		��ͨ��ģʽ��ɨ��ģʽ��*/
		if(adc_intstr->ADC_NbrOfChannel == 1)
				adc_intstr->ADC_ScanConvMode=ENABLE;
	/*	ADC_NbrOfChannel:
		����ͨ����   */
		adc_intstr->ADC_NbrOfChannel += 1;			//���һ��ͨ��						
		ADC_Init(ADC1,adc_intstr);
		
		if(line >= Vrefint)	ADC_TempSensorVrefintCmd(ENABLE);	// ���� PVD  ��  �¶�ͨ��
}

//bool ADConverter::AddLine(ADC_Channel line)
//{
//	assert_param( (line < PA10) || ((PC0 <= line) && (line < PC6)) || (0x80 == line)||(line == 0x81) );
//	
//	return true;
//}

void ADConverter::OnConfig()
{
}

int ADConverter::Read()
{
	return adc_IntStrs[_ADC_group]->_AnalogValue[_ChannelNum];
}

ADConverter:: ~ADConverter()
{
}

ADC_Stru::ADC_Stru(byte ADC_group)
{	
	/*	ADC_Mode   ģʽ:
		 ADC_Mode_Independent          		����ģʽ         
		 ADC_Mode_RegInjecSimult           	��ϵ�ͬ������ ע��ͬ��ģʽ        
		 ADC_Mode_RegSimult_AlterTrig    	��ϵ�ͬ������ ���津��ģʽ          
		 ADC_Mode_InjecSimult_FastInterl  	���ͬ��ע�� ���ٽ���ģʽ        
		 ADC_Mode_InjecSimult_SlowInterl   	���ͬ��ע�� ���ٽ���ģʽ      
		 ADC_Mode_InjecSimult              	ע��ͬ��ģʽ       
		 ADC_Mode_RegSimult                	����ͬ��ģʽ
		 ADC_Mode_FastInterl               	���ٽ���ģʽ
		 ADC_Mode_SlowInterl              	���ٽ���ģʽ  
		 ADC_Mode_AlterTrig                	���津��ģʽ	*/
		adc_intstr.ADC_Mode = ADC_Mode_Independent;
		/*  ADC_ScanConvMode  ת��ģʽ:
		 DISABLE	��ͨ��ģʽ
		 ENABLE		��ͨ��ģʽ��ɨ��ģʽ��*/
		adc_intstr.ADC_ScanConvMode = DISABLE;
	/*	ADC_ContinuousConvMode   �Ƿ�����ת����
		 DISABLE	����
		 ENABLE		����*/
		adc_intstr.ADC_ContinuousConvMode = ENABLE;	
	/*	ADC_ExternalTrigConv   ������ʽ:
		 ADC_ExternalTrigConv_T1_CC1                For ADC1 and ADC2 
		 ADC_ExternalTrigConv_T1_CC2                For ADC1 and ADC2 
		 ADC_ExternalTrigConv_T2_CC2                For ADC1 and ADC2 
		 ADC_ExternalTrigConv_T3_TRGO               For ADC1 and ADC2 
		 ADC_ExternalTrigConv_T4_CC4                For ADC1 and ADC2 
		 ADC_ExternalTrigConv_Ext_IT11_TIM8_TRGO    For ADC1 and ADC2 
		 ADC_ExternalTrigConv_T1_CC3                For ADC1, ADC2 and ADC3
		 ADC_ExternalTrigConv_None      	���	For ADC1, ADC2 and ADC3
		 ADC_ExternalTrigConv_T3_CC1                For ADC3 only
		 ADC_ExternalTrigConv_T2_CC3                For ADC3 only
		 ADC_ExternalTrigConv_T8_CC1                For ADC3 only
		 ADC_ExternalTrigConv_T8_TRGO               For ADC3 only
		 ADC_ExternalTrigConv_T5_CC1                For ADC3 only
		 ADC_ExternalTrigConv_T5_CC3                For ADC3 only*/
		adc_intstr.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	/*	ADC_DataAlign  ת��������뷽ʽ:
		 ADC_DataAlign_Right                        
		 ADC_DataAlign_Left     */
		adc_intstr.ADC_DataAlign = ADC_DataAlign_Right;
	/*	ADC_NbrOfChannel:
		����ͨ����   */
		adc_intstr.ADC_NbrOfChannel = 0;	// ��ʼ��Ϊ��	
}

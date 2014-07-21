#include "System.h"

#ifdef STM32F10X
	#include "stm32f10x_spi.h"
	#include "Pin_STM32F1.h"
#else
	#include "stm32f0xx_spi.h"
	#include "Pin_STM32F0.h"
#endif

#include "Pin.h"

static const  Pin  g_Spi_Pins_Map[][4]=  SPI_PINS_FULLREMAP;
/*cs����*/
static const Pin spi_nss[3]=
{
	//һ��ѡȡ Ӳ��  spi nss����   
	PA4		,								//spi1
	PB12	,								//spi2
	PA15	,								//spi3
};
#define SPI_CS_HIGH(spi)		Sys.IO.Write(spi_nss[spi],1)
#define SPI_CS_LOW(spi)			Sys.IO.Write(spi_nss[spi],0)
#define IS_SPI(a)	   if(a>3){return false;}

//#define SPI_NSS_PINS  {4, 28, 15} // PA4, PB12, PA15
//#define SPI_SCLK_PINS {5, 29, 19} // PA5, PB13, PB3
//#define SPI_MISO_PINS {6, 30, 20} // PA6, PB14, PB4
//#define SPI_MOSI_PINS {7, 31, 21} // PA7, PB15, PB5

#ifdef STM32F10X
void gpio_config(const Pin  pin[])
{

}
#else
void gpio_config(const Pin pin[])
{
    Sys.IO.OpenPort(pin[1], GPIO_Mode_AF, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);
    Sys.IO.OpenPort(pin[2], GPIO_Mode_AF, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);
    Sys.IO.OpenPort(pin[3], GPIO_Mode_AF, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);
		//spi����GPIO_AF_0������
    GPIO_PinAFConfig(_GROUP(pin[1]), _PIN(pin[1]), GPIO_AF_0);
    GPIO_PinAFConfig(_GROUP(pin[2]), _PIN(pin[2]), GPIO_AF_0);
    GPIO_PinAFConfig(_GROUP(pin[3]), _PIN(pin[3]), GPIO_AF_0);
}
#endif
bool Spi_config(int spi)
{
	
	const Pin  * p= g_Spi_Pins_Map[spi];
#ifdef STM32F1XX
	
#else
	SPI_InitTypeDef SPI_InitStructure;
#endif	
	IS_SPI(spi)
	gpio_config(p);
#ifdef STM32F10X
  /*ʹ��SPI1ʱ��*/
	switch(spi)
	{
		case SPI_1 :RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE); break;
		case SPI_2 :RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE); break;
		case SPI_3 :RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE); break;
	}	  
  /* ����csn���ţ��ͷ�spi����*/
	SPI_CS_HIGH(spi);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //˫��ȫ˫��
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;	 					//��ģʽ
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;	 				//���ݴ�С8λ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;		 				//ʱ�Ӽ��ԣ�����ʱΪ��
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;						//��1��������Ч��������Ϊ����ʱ��
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		   					//NSS�ź����������
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  //8��Ƶ��9MHz
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;  				//��λ��ǰ
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	printf("   ��ʼ��SPI_%d\r\n",(spi+1));
	switch(spi)
	{								/*				����spi													ʹ��spi  */	
		case SPI_1 : SPI_Init(SPI1, &SPI_InitStructure);  SPI_Cmd(SPI1, ENABLE);	break;
		case SPI_2 : SPI_Init(SPI2, &SPI_InitStructure);  SPI_Cmd(SPI2, ENABLE);	break;
		case SPI_3 : SPI_Init(SPI3, &SPI_InitStructure);  SPI_Cmd(SPI3, ENABLE);	break;	
	}	  
#else
	switch(spi)
	{								
		  case SPI_1 : RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);  	break;
//			case SPI_2 : RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI2, ENABLE); 	break;
//			case SPI_3 : RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI3, ENABLE); 	break;
	}	  
	SPI_CS_HIGH(spi);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//����SPI����ģʽ:����Ϊ��SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;		//ѡ���˴���ʱ�ӵ���̬:ʱ�����ո�
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;	//���ݲ����ڵڶ���ʱ����
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;		//���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRCֵ����Ķ���ʽ
	printf("   ��ʼ��SPI_%d\r\n",(spi+1));
	switch(spi)
	{								/*				����spi													ʹ��spi  */	
		  case SPI_1 : SPI_Init(SPI1, &SPI_InitStructure);  SPI_Cmd(SPI1, ENABLE);	break;
//			case SPI_2 : SPI_Init(SPI2, &SPI_InitStructure);  SPI_Cmd(SPI2, ENABLE);	break;
//			case SPI_3 : SPI_Init(SPI3, &SPI_InitStructure);  SPI_Cmd(SPI3, ENABLE);	break;
	}	  
#endif
	return true;
}
	
bool Spi_Disable(int spi)
{
	IS_SPI(spi)
	SPI_CS_HIGH(spi);		//ʹ�ӻ��ͷ�����
#ifdef STM32F10X
	switch(spi)
	{								/*		ʧ��spi  															�ر�spiʱ��							*/	
		case SPI_1 :		SPI_Cmd(SPI1, DISABLE);		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, DISABLE); break;
		case SPI_2 :		SPI_Cmd(SPI2, DISABLE);		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, DISABLE); break;
		case SPI_3 :		SPI_Cmd(SPI3, DISABLE);		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, DISABLE); break;
	}	 
//	printf("   �ر�SPI_%d\r\n",(spi+1));
	return true;
#else
	switch(spi)
	{								/*		ʧ��spi  															�ر�spiʱ��							*/	
		  case SPI_1 :   SPI_Cmd(SPI1, DISABLE);	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, DISABLE); break;
//			case SPI_2 :   SPI_Cmd(SPI2, DISABLE);	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI2, DISABLE); break;
//			case SPI_3 :   SPI_Cmd(SPI3, DISABLE);	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI3, DISABLE); break;
	}	  
//		printf("   �ر�SPI_%d\r\n",(spi+1));
	return true;
#endif
}

byte SPI_ReadWriteByte8(int spi,byte TxData)
{		
	unsigned char retry=0;				 
	SPI_TypeDef *	p;
		switch(spi)
	{							
		case SPI_1 : p=  SPI1 ; break;

#ifdef STM32F10X
		case SPI_2 : p=  SPI2;  break;
		case SPI_3 : p=  SPI3; 	break;
#endif
	}	 
	while (SPI_I2S_GetFlagStatus(p, SPI_I2S_FLAG_TXE) == RESET) 
		{
		retry++;
		if(retry>200)return 0;		//��ʱ����
		}		
 
#ifdef STM32F10X
	SPI_I2S_SendData(p, TxData);
#else
	SPI_I2S_SendData8(p, HalfWord);
#endif		
	retry=0;
	while (SPI_I2S_GetFlagStatus(p, SPI_I2S_FLAG_RXNE) == RESET) //�Ƿ��ͳɹ�
		{
		retry++;
		if(retry>200)return 0;		//��ʱ����
		}	 
#ifdef STM32F10X
	return SPI_I2S_ReceiveData(p);
#else 						    
	return SPI_ReceiveData8(p); //����ͨ��SPIx������յ�����			
#endif		
}	

ushort SPI_ReadWriteByte16(int spi,ushort HalfWord)
{
 	uint retry=0;				 
	SPI_TypeDef *	p;
	switch(spi)
	{							
		case SPI_1 : p=  SPI1 ; break;

#ifdef STM32F10X
		case SPI_2 : p=  SPI2;  break;
		case SPI_3 : p=  SPI3; 	break;
#endif
	}	 
	while (SPI_I2S_GetFlagStatus(p, SPI_I2S_FLAG_TXE) == RESET)
	{
		retry++;
		if(retry>500)return 0;		//��ʱ����
	}			
 
#ifdef STM32F10X
	SPI_I2S_SendData(p, HalfWord);
#else
	SPI_I2S_SendData16(p, HalfWord);
#endif
	while (SPI_I2S_GetFlagStatus(p, SPI_I2S_FLAG_RXNE) == RESET)
	{
		retry++;
		if(retry>500)return 0;		//��ʱ����
	}			
	
#ifdef STM32F10X
	return SPI_I2S_ReceiveData(p);
#else
	return SPI_I2S_ReceiveData16(p);
#endif
}

void TSpi_Init(TSpi* this)
{
			this->Open  					= Spi_config;
			this->Close 					= Spi_Disable;
			this->WriteReadByte8 	= SPI_ReadWriteByte8;
			this->WriteReadByte16	=	SPI_ReadWriteByte16;
//    this->WriteRead 	= 
//    this->WriteRead16	= 
}


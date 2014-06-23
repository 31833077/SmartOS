

#include "System.h"

#ifdef STM32F1XX
	#include "stm32f10x_spi.h"
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




#define IS_SPI(a)	   {if(3<a<0xff)return false;}



//#define SPI_NSS_PINS  {4, 28, 15} // PA4, PB12, PA15
//#define SPI_SCLK_PINS {5, 29, 19} // PA5, PB13, PB3
//#define SPI_MISO_PINS {6, 30, 20} // PA6, PB14, PB4
//#define SPI_MOSI_PINS {7, 31, 21} // PA7, PB15, PB5



//
// typedef struct TSpi_Def
//{
//	void (*Init)(struct TSpi_Def* this);
//	void (*Uninit)(struct TSpi_Def* this);
//  bool (*WriteRead)(const SPI_CONFIGURATION& Configuration, byte* Write8, int WriteCount, byte* Read8, int ReadCount, int ReadStartOffset);
//  bool (*WriteRead16)(const SPI_CONFIGURATION& Configuration, ushort* Write16, int WriteCount, ushort* Read16, int ReadCount, int ReadStartOffset);
//
//} TSpi;

//extern void TSpi_Init(TSpi* this);
//


#ifdef STM32F1XX
void gpio_config(const Pin  pin[])
{

	
	
}

#else

void gpio_config(const Pin pin[])
{
		//
    Sys.IO.OpenPort(pin[1], GPIO_Mode_AF, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);
    Sys.IO.OpenPort(pin[2], GPIO_Mode_AF, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);
    Sys.IO.OpenPort(pin[3], GPIO_Mode_AF, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);
	
		//spi����GPIO_AF_0������
    GPIO_PinAFConfig(_GROUP(pin[1]), _PIN(pin[1]), GPIO_AF_0);
    GPIO_PinAFConfig(_GROUP(pin[2]), _PIN(pin[2]), GPIO_AF_0);
    GPIO_PinAFConfig(_GROUP(pin[3]), _PIN(pin[3]), GPIO_AF_0);
}

#endif




bool Spi_config(int spi,int clk)
{
	//get pin
	const Pin  * p= g_Spi_Pins_Map[spi];
	
	IS_SPI(spi);
	gpio_config(p);
	
	
#ifdef STM32F1XX

  /*ʹ��SPI1ʱ��*/
	switch(spi)
	{
		  case SPI_1 :RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE); break;
			case SPI_2 :RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI2, ENABLE); break;
			case SPI_3 :RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI3, ENABLE); break;
					
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
	switch(spi)
	{								/*				����spi													ʹ��spi  */	
		  case SPI_1 : SPI_Init(SPI1, &SPI_InitStructure);  SPI_Cmd(SPI1, ENABLE);	break;
			case SPI_2 : SPI_Init(SPI2, &SPI_InitStructure);  SPI_Cmd(SPI2, ENABLE);	break;
			case SPI_3 : SPI_Init(SPI3, &SPI_InitStructure);  SPI_Cmd(SPI3, ENABLE);	break;
					
	}	  
	
#else


//#define SPI_1	0
//#define SPI_2	1
//#define SPI_3	2
//#define SPI_NONE 0XFF
	
	
	
	
	
	
	
	
#endif
	
	
	return true;
	
}
	





void TSpi_Init(TSpi* this)
{
			this->Open  			= Spi_config;
	
//    this->Close 			= 
//    this->WriteRead 	= 
//    this->WriteRead16	= 
}











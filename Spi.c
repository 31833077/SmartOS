

#include "System.h"

#ifdef STM32F1XX
	#include "stm32f10x_spi.h"
#else
	#include "stm32f0xx_spi.h"
	#include "Pin_STM32F0.h"
#endif

#include "Pin.h"



static const  Pin  g_Spi_Pins_Map[][4]=  SPI_PINS_FULLREMAP;
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
void gpio_config(Pin  pin[])
{
	  
		Sys.IO.OpenPort(pin[1],GPIO_Mode_OUT, GPIO_Speed_Level_1 , GPIO_OType_PP ,GPIO_PuPd_NOPULL);			//clk
		Sys.IO.OpenPort(pin[2],GPIO_Mode_OUT, GPIO_Speed_Level_1 , GPIO_OType_PP ,GPIO_PuPd_NOPULL);			//miso
		Sys.IO.OpenPort(pin[3],GPIO_Mode_OUT, GPIO_Speed_Level_1 , GPIO_OType_PP ,GPIO_PuPd_NOPULL);;						//mosi
//		
//    Sys.IO.OpenPort(pin[0], GPIO_Mode_Out_PP, GPIO_Speed_10MHz);				//nss
	
}



#else

void gpio_config(Pin pin[])
{
	

}
#endif









bool Spi_config(int spi,int clk)
{
	//get pin
	void * p= &(g_Spi_Pins_Map[spi]);
	
	
	IS_SPI(spi);
	gpio_config(p);
	
	
	
	
	
#ifdef STM32F1XX

//  /*ʹ��SPI1ʱ��*/
//		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
//		  
//  /* �����Զ���ĺ꣬��������csn���ţ�NRF�������״̬ */
//		NRF_CSN_HIGH(); 
// 
//		SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //˫��ȫ˫��
//		SPI_InitStructure.SPI_Mode = SPI_Mode_Master;	 					//��ģʽ
//		SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;	 				//���ݴ�С8λ
//		SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;		 				//ʱ�Ӽ��ԣ�����ʱΪ��
//		SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;						//��1��������Ч��������Ϊ����ʱ��
//		SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		   					//NSS�ź����������
//		SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  //8��Ƶ��9MHz
//		SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;  				//��λ��ǰ
//		SPI_InitStructure.SPI_CRCPolynomial = 7;
//		SPI_Init(SPI1, &SPI_InitStructure);

//  /* Enable SPI1  */
//		SPI_Cmd(SPI1, ENABLE);
//	
//	
#else

	
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











#include "System.h"
#include "nrf24l01.h"

/********************************************************************************/
//NRF24L01�Ĵ�����������
#define READ_REG_NRF        0x00  //�����üĴ���,��5λΪ�Ĵ�����ַ
#define WRITE_REG_NRF       0x20  //д���üĴ���,��5λΪ�Ĵ�����ַ
#define RD_RX_PLOAD     0x61  //��RX��Ч����,1~32�ֽ�
#define WR_TX_PLOAD     0xA0  //дTX��Ч����,1~32�ֽ�
#define FLUSH_TX        0xE1  //���TX FIFO�Ĵ���.����ģʽ����
#define FLUSH_RX        0xE2  //���RX FIFO�Ĵ���.����ģʽ����
#define REUSE_TX_PL     0xE3  //����ʹ����һ������,CEΪ��,���ݰ������Ϸ���.
#define NOP             0xFF  //�ղ���,����������״̬�Ĵ���	 
/********************************************************************************/
//SPI(NRF24L01)�Ĵ�����ַ
#define CONFIG          0x00  //�����շ�״̬��CRCУ��ģʽ���շ�״̬��Ӧ��ʽ;
															//bit0:1����ģʽ,0����ģʽ;bit1:��ѡ��;bit2:CRCģʽ;bit3:CRCʹ��;
                              //bit4:�ж�MAX_RT(�ﵽ����ط������ж�)ʹ��;bit5:�ж�TX_DSʹ��;bit6:�ж�RX_DRʹ��
#define EN_AA           0x01  //�Զ�Ӧ��������  bit0~5,��Ӧͨ��0~5
#define EN_RXADDR       0x02  //�����ŵ�ʹ��,bit0~5,��Ӧͨ��0~5
#define SETUP_AW        0x03  //���õ�ַ���(��������ͨ��):bit[1:0]     00,3�ֽ�;01,4�ֽ�;02,5�ֽ�;
#define SETUP_RETR      0x04  //�����Զ��ط�;bit3:0,�Զ��ط�������;bit7:4,�Զ��ط���ʱ 250*x+86us
#define RF_CH           0x05  //RFͨ��,bit6:0,����ͨ��Ƶ��;
#define RF_SETUP        0x06  //RF�Ĵ���;bit3:��������(0:1Mbps,1:2Mbps);bit2:1,���书��;bit0:�������Ŵ�������
#define STATUS          0x07  //״̬�Ĵ���;bit0:TX FIFO����־;bit3:1,��������ͨ����(���:6);
															//bit4,�ﵽ�����ط�  bit5:���ݷ�������ж�;bit6:���������ж�;
#define OBSERVE_TX      0x08  //���ͼ��Ĵ���,bit7:4,���ݰ���ʧ������;bit3:0,�ط�������
#define CD              0x09  //�ز����Ĵ���,bit0,�ز����;
#define RX_ADDR_P0      0x0A  //����ͨ��0���յ�ַ,��󳤶�5���ֽ�,���ֽ���ǰ
#define RX_ADDR_P1      0x0B  //����ͨ��1���յ�ַ,��󳤶�5���ֽ�,���ֽ���ǰ
#define RX_ADDR_P2      0x0C  //����ͨ��2���յ�ַ,����ֽڿ�����,���ֽ�,����ͬRX_ADDR_P1[39:8]���;
#define RX_ADDR_P3      0x0D  //����ͨ��3���յ�ַ,����ֽڿ�����,���ֽ�,����ͬRX_ADDR_P1[39:8]���;
#define RX_ADDR_P4      0x0E  //����ͨ��4���յ�ַ,����ֽڿ�����,���ֽ�,����ͬRX_ADDR_P1[39:8]���;
#define RX_ADDR_P5      0x0F  //����ͨ��5���յ�ַ,����ֽڿ�����,���ֽ�,����ͬRX_ADDR_P1[39:8]���;
#define TX_ADDR         0x10  //���͵�ַ(���ֽ���ǰ),ShockBurstTMģʽ��,RX_ADDR_P0��˵�ַ���
#define RX_PW_P0        0x11  //��������ͨ��0��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P1        0x12  //��������ͨ��1��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P2        0x13  //��������ͨ��2��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P3        0x14  //��������ͨ��3��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P4        0x15  //��������ͨ��4��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P5        0x16  //��������ͨ��5��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define NRF_FIFO_STATUS 0x17  //FIFO״̬�Ĵ���;bit0,RX FIFO�Ĵ����ձ�־;bit1,RX FIFO����־;bit2,3,����
                              //bit4,TX FIFO�ձ�־;bit5,TX FIFO����־;bit6,1,ѭ��������һ���ݰ�.0,��ѭ��;
															
#define MAX_TX  		0x10  //�ﵽ����ʹ����ж�
#define TX_OK   		0x20  //TX��������ж�
#define RX_OK   		0x40  //���յ������ж�	

/*����  ����   ��ַ*/
byte TX_ADDRESS[TX_ADR_WIDTH] = TX_Address;  
byte RX_ADDRESS[RX_ADR_WIDTH] = RX_Address; 

/********************************************************************************/

//nRF2401 ״̬  ��ί��ʹ��
enum nRF_state
{
	nrf_mode_rx,
	nrf_mode_tx,
	nrf_mode_free
}
nRF24L01_status=nrf_mode_free;
/********************************************************************************/
//����ԭ��
byte NRF24L01_Read_Buf(byte reg,byte *pBuf,byte len);
byte NRF24L01_Write_Buf(byte reg, byte *pBuf, byte len);
byte NRF24L01_Write_Reg(byte reg,byte value);
byte NRF24L01_Read_Reg(byte reg);
//2401ί�к���
void nRF24L01_irq(Pin pin, bool opk);


/*cs����*/
static const Pin spi_nss[3]=
{
	//һ��ѡȡ Ӳ��  spi nss����   
	PA4		,								//spi1
	PB12	,								//spi2
	PA15	,								//spi3
};

//���  spi  ����������
void nRF_Init(void)
{
#ifdef STM32F10X
	#if Other_nRF_CSN
		Sys.IO.Open (nRF2401_CSN,  GPIO_Mode_Out_PP );
	#else	
		Sys.IO.Open (spi_nss[nRF2401_SPI],  GPIO_Mode_Out_PP );		
		if(nRF2401_SPI==SPI_3)
		{	//PA15��jtag�ӿ��е�һԱ ��Ҫʹ�� ���뿪��remap
			RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE);	
			GPIO_PinRemapConfig( GPIO_Remap_SWJ_JTAGDisable, ENABLE);
		}
	#endif  //Other_nRF_CSN
	#if us_nrf_ce
		Sys.IO.Open (nRF2401_CE,  GPIO_Mode_Out_PP );
	#endif
	//�ж����ų�ʼ��
	//Sys.IO.OpenPort(nRF2401_IRQ_pin, GPIO_Mode_IN_FLOATING,  GPIO_Speed_10MHz , 0, GPIO_PuPd_DOWN );
		Sys.IO.Open (nRF2401_IRQ_pin, GPIO_Mode_IN_FLOATING );
	//�ж���������ί��	
		Sys.IO.Register(nRF2401_IRQ_pin,nRF24L01_irq);
#else
	#if Other_nRF_CSN
		Sys.IO.OpenPort(nRF2401_CSN, GPIO_Mode_OUT, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);
	#else	
		Sys.IO.OpenPort(spi_nss[nRF2401_SPI], GPIO_Mode_OUT, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);

	#endif  //Other_nRF_CSN
	#if us_nrf_ce
		Sys.IO.OpenPort(nRF2401_CE, GPIO_Mode_OUT, GPIO_Speed_10MHz, GPIO_OType_PP,GPIO_PuPd_NOPULL);
	#endif
	//�ж����ų�ʼ��
	Sys.IO.OpenPort(nRF2401_IRQ_pin, GPIO_Mode_IN,  GPIO_Speed_10MHz , GPIO_OType_PP, GPIO_PuPd_UP );
	//�ж���������ί��	
	Sys.IO.Register(nRF2401_IRQ_pin,nRF24L01_irq);
#endif	
	Sys.Spi.Open(nRF2401_SPI);	
	
	NRF24L01_Write_Reg(FLUSH_RX,0xff);//���RX FIFO�Ĵ��� 
	NRF24L01_Write_Reg(FLUSH_TX,0xff);//���RX FIFO�Ĵ��� 
}

//���24L01�Ƿ����
//����ֵ:0���ɹ�;1��ʧ��	
byte NRF24L01_Check(void)
{
	byte buf[5]={0XA5,0XA5,0XA5,0XA5,0XA5};
	byte i;
	Sys.Spi.SetSpeed(nRF2401_SPI,SPI_BaudRatePrescaler_4); //spi�ٶ�Ϊ9Mhz��24L01�����SPIʱ��Ϊ10Mhz��   	 
	NRF24L01_Write_Buf(WRITE_REG_NRF+TX_ADDR,buf,5);//д��5���ֽڵĵ�ַ.	
	NRF24L01_Read_Buf(TX_ADDR,buf,5); //����д��ĵ�ַ  
	for(i=0;i<5;i++)if(buf[i]!=0XA5)break;	 							   
	if(i!=5)return 1;//���24L01����	
	return 0;		 //��⵽24L01
}	

//����NRF24L01����һ������
//txbuf:�����������׵�ַ
//����ֵ:�������״��
byte NRF24L01_TxPacket(byte *txbuf)
{
	byte sta;
 	Sys.Spi.SetSpeed(nRF2401_SPI,SPI_BaudRatePrescaler_16);//spi�ٶ�Ϊ9Mhz��24L01�����SPIʱ��Ϊ10Mhz��   
	NRF_CE_LOW();
  NRF24L01_Write_Buf(WR_TX_PLOAD,txbuf,TX_PLOAD_WIDTH);//д���ݵ�TX BUF  32���ֽ�
 	NRF_CE_HIGH();					//��������	   
//	while(NRF24L01_IRQ!=0);//�ȴ��������
	sta=NRF24L01_Read_Reg(STATUS);  //��ȡ״̬�Ĵ�����ֵ	   
	NRF24L01_Write_Reg(WRITE_REG_NRF+STATUS,sta); //���TX_DS��MAX_RT�жϱ�־
	if(sta&MAX_TX)//�ﵽ����ط�����
	{
		NRF24L01_Write_Reg(FLUSH_TX,0xff);//���TX FIFO�Ĵ��� 
		return MAX_TX; 
	}
	if(sta&TX_OK)//�������
	{
		return TX_OK;
	}
	return 0xff;//����ԭ����ʧ��
}

//����NRF24L01����һ������
//txbuf:�����������׵�ַ
//����ֵ:0��������ɣ��������������
byte NRF24L01_RxPacket(byte *rxbuf)
{
	byte sta;		    							   
	Sys.Spi.SetSpeed(nRF2401_SPI,SPI_BaudRatePrescaler_16); //spi�ٶ�Ϊ9Mhz��24L01�����SPIʱ��Ϊ10Mhz��   
	sta=NRF24L01_Read_Reg(STATUS);  //��ȡ״̬�Ĵ�����ֵ    	 
	NRF24L01_Write_Reg(WRITE_REG_NRF+STATUS,sta); //���TX_DS��MAX_RT�жϱ�־
	if(sta&RX_OK)//���յ�����
	{
		NRF24L01_Read_Buf(RD_RX_PLOAD,rxbuf,RX_PLOAD_WIDTH);//��ȡ����
		NRF24L01_Write_Reg(FLUSH_RX,0xff);//���RX FIFO�Ĵ��� 
		return 0; 
	}	   
	return 1;//û�յ��κ�����
}		

//�ú�����ʼ��NRF24L01��RXģʽ
//����RX��ַ,дRX���ݿ��,ѡ��RFƵ��,�����ʺ�LNA HCURR
//��CE��ߺ�,������RXģʽ,�����Խ���������		   
void NRF24L01_RX_Mode(void)
{
		NRF_CE_LOW();	  
  	NRF24L01_Write_Buf(WRITE_REG_NRF+RX_ADDR_P0,RX_ADDRESS,RX_ADR_WIDTH);//дRX�ڵ��ַ
	
		NRF24L01_Write_Reg(WRITE_REG_NRF+SETUP_AW,0x02);	//���ý��յ�ַ����Ϊ5  **
  	NRF24L01_Write_Reg(WRITE_REG_NRF+EN_AA,0x01);    //ʹ��ͨ��0���Զ�Ӧ��    
  	NRF24L01_Write_Reg(WRITE_REG_NRF+EN_RXADDR,0x01);//ʹ��ͨ��0�Ľ��յ�ַ  	 
  	NRF24L01_Write_Reg(WRITE_REG_NRF+RF_CH,CHANAL);	     //����RFͨ��Ƶ��		  
  	NRF24L01_Write_Reg(WRITE_REG_NRF+RX_PW_P0,RX_PLOAD_WIDTH);//ѡ��ͨ��0����Ч���ݿ�� 	    
  	NRF24L01_Write_Reg(WRITE_REG_NRF+RF_SETUP,0x07); 
  	NRF24L01_Write_Reg(WRITE_REG_NRF+CONFIG, 0x0f);//���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ 
  	NRF_CE_HIGH(); //CEΪ��,�������ģʽ 
}				

//�ú�����ʼ��NRF24L01��TXģʽ
//����TX��ַ,дTX���ݿ��,����RX�Զ�Ӧ��ĵ�ַ,���TX��������,ѡ��RFƵ��,�����ʺ�LNA HCURR
//PWR_UP,CRCʹ��
//��CE��ߺ�,������RXģʽ,�����Խ���������		   
//CEΪ�ߴ���10us,����������.	 
void NRF24L01_TX_Mode(void)
{									
		NRF_CE_LOW();	     
  	NRF24L01_Write_Buf(WRITE_REG_NRF+TX_ADDR,(u8*)TX_ADDRESS,TX_ADR_WIDTH);//дTX�ڵ��ַ 
  	NRF24L01_Write_Buf(WRITE_REG_NRF+RX_ADDR_P0,(u8*)RX_ADDRESS,RX_ADR_WIDTH); //����TX�ڵ��ַ,��ҪΪ��ʹ��ACK	  
	
		NRF24L01_Write_Reg(WRITE_REG_NRF+SETUP_AW,0x02);	//���ý��յ�ַ����Ϊ5  **
  	NRF24L01_Write_Reg(WRITE_REG_NRF+EN_AA,0x01);     //ʹ��ͨ��0���Զ�Ӧ��    
  	NRF24L01_Write_Reg(WRITE_REG_NRF+EN_RXADDR,0x01); //ʹ��ͨ��0�Ľ��յ�ַ  
  	NRF24L01_Write_Reg(WRITE_REG_NRF+SETUP_RETR,0x1a);//�����Զ��ط����ʱ��:500us + 86us;����Զ��ط�����:10��
  	NRF24L01_Write_Reg(WRITE_REG_NRF+RF_CH,CHANAL);       //����RFͨ��
  	NRF24L01_Write_Reg(WRITE_REG_NRF+RF_SETUP,0x07);  
  	NRF24L01_Write_Reg(WRITE_REG_NRF+CONFIG,0x0e);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ,���������ж�
		NRF_CE_HIGH();//CEΪ��,10us����������
		Sys.Delay(1000);
}		  

/****************************************************************/

//SPIд�Ĵ���
//reg:ָ���Ĵ�����ַ
//value:д���ֵ
byte NRF24L01_Write_Reg(byte reg,byte value)
{
	byte status;	
  NRF_CSN_LOW();                   //ʹ��SPI����
  status =Sys.Spi.WriteReadByte8(nRF2401_SPI,reg);//���ͼĴ����� 
  Sys.Spi.WriteReadByte8(nRF2401_SPI,value);      //д��Ĵ�����ֵ
  NRF_CSN_HIGH();                 //��ֹSPI����	   
  return(status);       			//����״ֵ̬
}

//��ȡSPI�Ĵ���ֵ
//reg:Ҫ���ļĴ���
byte NRF24L01_Read_Reg(byte reg)
{
	byte reg_val;	    
 	NRF_CSN_LOW();          //ʹ��SPI����		
  Sys.Spi.WriteReadByte8(nRF2401_SPI,reg);   //���ͼĴ�����
  reg_val=Sys.Spi.WriteReadByte8(nRF2401_SPI,0XFF);//��ȡ�Ĵ�������
  NRF_CSN_HIGH();          //��ֹSPI����		    
  return(reg_val);           //����״ֵ̬
}	

//��ָ��λ�ö���ָ�����ȵ�����
//reg:�Ĵ���(λ��)
//*pBuf:����ָ��
//len:���ݳ���
//����ֵ,�˴ζ�����״̬�Ĵ���ֵ 
byte NRF24L01_Read_Buf(byte reg,byte *pBuf,byte len)
{
	byte status,u8_ctr;	       
  NRF_CSN_LOW();           //ʹ��SPI����
  status=Sys.Spi.WriteReadByte8(nRF2401_SPI,reg);//���ͼĴ���ֵ(λ��),����ȡ״ֵ̬   	   
 	for(u8_ctr=0;u8_ctr<len;u8_ctr++)pBuf[u8_ctr]=Sys.Spi.WriteReadByte8(nRF2401_SPI,0XFF);//��������
  NRF_CSN_HIGH();       //�ر�SPI����
  return status;        //���ض�����״ֵ̬
}

//��ָ��λ��дָ�����ȵ�����
//reg:�Ĵ���(λ��)
//*pBuf:����ָ��
//len:���ݳ���
//����ֵ,�˴ζ�����״̬�Ĵ���ֵ
byte NRF24L01_Write_Buf(byte reg, byte *pBuf, byte len)
{
	byte status,u8_ctr;	   
	NRF_CSN_LOW();	          //ʹ��SPI����
  status =Sys.Spi.WriteReadByte8(nRF2401_SPI,reg);//���ͼĴ���ֵ(λ��),����ȡ״ֵ̬
  for(u8_ctr=0; u8_ctr<len; u8_ctr++)Sys.Spi.WriteReadByte8(nRF2401_SPI,*pBuf++); //д������	 
  NRF_CSN_HIGH();       //�ر�SPI����
  return status;          //���ض�����״ֵ̬
}		

/****************************************************************/

//2401ί�к���  δ���
void nRF24L01_irq(Pin pin, bool opk)
{
	(void)pin;
	(void)opk;
	nRF24L01_status=nrf_mode_tx;
//	switch(nRF24L01_status)
//	{
//		case 	nrf_mode_rx		:	a=b	;		
//				break;
//		case	 nrf_mode_tx	:	a+=b;		
//				break;
//		case	 nrf_mode_free	:	a-=b;		
//				break;
//	}
}

//��ʼ��nRF�����ӿ�
void TnRF_Init(TnRF* this)
{
    this->Open  	= nRF_Init;
    this->Check 	= NRF24L01_Check;
    this->RX_Mode = NRF24L01_RX_Mode;
    this->TX_Mode = NRF24L01_TX_Mode;
    this->Rx_Dat  = NRF24L01_RxPacket;
    this->Tx_Dat 	= NRF24L01_TxPacket;
//	Execute(Spi);
}

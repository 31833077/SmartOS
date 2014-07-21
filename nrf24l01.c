#include "System.h"
#include "nrf24l01.h"

byte RX_BUF[RX_PLOAD_WIDTH];		//�������ݻ���
byte TX_BUF[TX_PLOAD_WIDTH];		//�������ݻ���
/*����  ����   ��ַ*/
byte TX_ADDRESS[TX_ADR_WIDTH] = {0x34,0x43,0x10,0x10,0x01};  
byte RX_ADDRESS[RX_ADR_WIDTH] = {0x34,0x43,0x10,0x10,0x01}; 
//nRF2401 ״̬  ��ί��ʹ��
enum nRF_state
{
	nrf_mode_rx,
	nrf_mode_tx,
	nrf_mode_free
}
nRF24L01_status=nrf_mode_free;
//byte nRF24L01_status;

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

/**
  * @brief  SPI�� I/O����
  * @param  ��
  * @retval ��
  */   //void TIO_Open(Pin pin, GPIOMode_TypeDef mode)
void nRF_Init(void)
{
#ifdef STM32F10X
	#if Other_nRF_CSN
		Sys.IO.Open (nRF2401_CSN,  GPIO_Mode_Out_PP );
	#else	
		Sys.IO.Open (spi_nss[nRF2401_SPI],  GPIO_Mode_Out_PP );
	#endif  //Other_nRF_CSN
	#if us_nrf_ce
		Sys.IO.Open (nRF2401_CE,  GPIO_Mode_Out_PP );
	#endif
	//�ж����ų�ʼ��
	//Sys.IO.OpenPort(nRF2401_IRQ_pin, GPIO_Mode_IN_FLOATING,  GPIO_Speed_10MHz , 0, GPIO_PuPd_DOWN );
		Sys.IO.Open (nRF2401_IRQ_pin,  GPIO_Mode_Out_PP );
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
	Sys.IO.OpenPort(nRF2401_IRQ_pin, GPIO_Mode_IN,  GPIO_Speed_10MHz , GPIO_OType_PP, GPIO_PuPd_DOWN );
	//�ж���������ί��	
	Sys.IO.Register(nRF2401_IRQ_pin,nRF24L01_irq);
#endif	
	Sys.Spi.Open(nRF2401_SPI);	
}

/**
  * @brief   ������NRF�ļĴ�����д��һ������
  * @param   
  *		@arg reg : NRF������+�Ĵ�����ַ
  *		@arg pBuf���洢�˽�Ҫд��д�Ĵ������ݵ����飬�ⲿ����
  * 	@arg bytes: pBuf�����ݳ���
  * @retval  NRF��status�Ĵ�����״̬
  */
byte SPI_NRF_WriteBuf(byte reg ,byte *pBuf,byte bytes)
{
	byte status,byte_cnt;
	NRF_CE_LOW();
   	 /*�õ�CSN��ʹ��SPI����*/
	NRF_CSN_LOW();			
	 /*���ͼĴ�����*/	
	status = Sys.Spi.WriteReadByte8(nRF2401_SPI,reg);
  	  /*�򻺳���д������*/
	for(byte_cnt=0;byte_cnt<bytes;byte_cnt++)
	//	SPI_NRF_RW(*pBuf++);	//д���ݵ������� 	 
		Sys.Spi.WriteReadByte8(nRF2401_SPI,*pBuf++);
	/*CSN���ߣ����*/
	NRF_CSN_HIGH();			
  	return (status);	//����NRF24L01��״̬ 		
}

/**
  * @brief   ������NRF�ļĴ�����д��һ������
  * @param   
  *		@arg reg : NRF������+�Ĵ�����ַ
  *		@arg pBuf�����ڴ洢���������ļĴ������ݵ����飬�ⲿ����
  * 	@arg bytes: pBuf�����ݳ���
  * @retval  NRF��status�Ĵ�����״̬
  */
byte SPI_NRF_ReadBuf(byte reg,byte *pBuf,byte bytes)
{
 	byte status, byte_cnt;
	  NRF_CE_LOW();
	/*�õ�CSN��ʹ��SPI����*/
	NRF_CSN_LOW();
	/*���ͼĴ�����*/		
	status = Sys.Spi.WriteReadByte8(nRF2401_SPI,reg); 
 	/*��ȡ����������*/
	for(byte_cnt=0;byte_cnt<bytes;byte_cnt++)		  
	  pBuf[byte_cnt] = Sys.Spi.WriteReadByte8(nRF2401_SPI,NOP); //��NRF24L01��ȡ����  
	 /*CSN���ߣ����*/
	NRF_CSN_HIGH();	
 	return status;		//���ؼĴ���״ֵ̬
}

/**
  * @brief  ��Ҫ����NRF��MCU�Ƿ���������
  * @param  ��
  * @retval SUCCESS/ERROR ��������/����ʧ��
  */
byte TnRF_Check(void)
{
	byte buf[5]={0xC2,0xC2,0xC2,0xC2,0xC2};
	byte buf1[5];
	byte i; 
	/*д��5���ֽڵĵ�ַ.  */  
	SPI_NRF_WriteBuf(NRF_WRITE_REG+TX_ADDR,buf,5);
	/*����д��ĵ�ַ */
	SPI_NRF_ReadBuf(TX_ADDR,buf1,5); 
	/*�Ƚ�*/               
	for(i=0;i<5;i++)
	{
		if(buf1[i]!=0xC2)
		break;
	} 
	if(i==5)
		return SUCCESS ;        //MCU��NRF�ɹ����� 
	else
		return ERROR ;        //MCU��NRF����������
}

/**
  * @brief   ���ڴ�NRF�ض��ļĴ�����������
  * @param   
  *		@arg reg:NRF������+�Ĵ�����ַ
  * @retval  �Ĵ����е�����
  */
byte SPI_NRF_ReadReg(byte reg)
{
 	byte reg_val;
	NRF_CE_LOW();
	/*�õ�CSN��ʹ��SPI����*/
 	NRF_CSN_LOW();
  	 /*���ͼĴ�����*/
	 Sys.Spi.WriteReadByte8(nRF2401_SPI,reg); 
	 /*��ȡ�Ĵ�����ֵ */
	reg_val =  Sys.Spi.WriteReadByte8(nRF2401_SPI,NOP);
   	/*CSN���ߣ����*/
	NRF_CSN_HIGH();		
	return reg_val;
}	

/**
  * @brief   ������NRF�ض��ļĴ���д������
  * @param   
  *		@arg reg:NRF������+�Ĵ�����ַ
  *		@arg dat:��Ҫ��Ĵ���д�������
  * @retval  NRF��status�Ĵ�����״̬
  */
byte SPI_NRF_WriteReg(byte reg,byte dat)
{
 	byte status;
	 NRF_CE_LOW();
	/*�õ�CSN��ʹ��SPI����*/
    NRF_CSN_LOW();
	/*��������Ĵ����� */
	status = Sys.Spi.WriteReadByte8(nRF2401_SPI,reg);
	 /*��Ĵ���д������*/
    Sys.Spi.WriteReadByte8(nRF2401_SPI,dat); 
	/*CSN���ߣ����*/	   
  	NRF_CSN_HIGH();	
	/*����״̬�Ĵ�����ֵ*/
   	return(status);
}

/**
  * @brief  ���ò��������ģʽ
  * @param  ��
  * @retval ��
  */
void NRF_RX_Mode(void)
{
	NRF_CE_LOW();	
	SPI_NRF_WriteBuf(NRF_WRITE_REG+RX_ADDR_P0,RX_ADDRESS,RX_ADR_WIDTH);//дRX�ڵ��ַ
	SPI_NRF_WriteReg(NRF_WRITE_REG+EN_AA,0x01);    //ʹ��ͨ��0���Զ�Ӧ��    
	SPI_NRF_WriteReg(NRF_WRITE_REG+EN_RXADDR,0x01);//ʹ��ͨ��0�Ľ��յ�ַ    
	SPI_NRF_WriteReg(NRF_WRITE_REG+RF_CH,CHANAL);      //����RFͨ��Ƶ��    
	SPI_NRF_WriteReg(NRF_WRITE_REG+RX_PW_P0,RX_PLOAD_WIDTH);//ѡ��ͨ��0����Ч���ݿ��      
   //SPI_NRF_WriteReg(NRF_WRITE_REG+RF_SETUP,0x0f); //����TX�������,0db����,2Mbps,���������濪��  
	SPI_NRF_WriteReg(NRF_WRITE_REG+RF_SETUP,0x07);  //����TX�������,0db����,1Mbps,���������濪��  
	SPI_NRF_WriteReg(NRF_WRITE_REG+CONFIG, 0x0f);  //���û�������ģʽ�Ĳ���;  PWR_UP,  EN_CRC, 16BIT_CRC, ����ģʽ 
/*CE���ߣ��������ģʽ*/	
	NRF_CE_HIGH();
	nRF24L01_status=nrf_mode_rx;
}    

/**
  * @brief  ���÷���ģʽ
  * @param  ��
  * @retval ��
  */
void NRF_TX_Mode(void)
{  
	NRF_CE_LOW();		
	SPI_NRF_WriteBuf(NRF_WRITE_REG+TX_ADDR,TX_ADDRESS,TX_ADR_WIDTH);    //дTX�ڵ��ַ 
	SPI_NRF_WriteBuf(NRF_WRITE_REG+RX_ADDR_P0,RX_ADDRESS,RX_ADR_WIDTH); //����TX�ڵ��ַ,��ҪΪ��ʹ��ACK   
	SPI_NRF_WriteReg(NRF_WRITE_REG+EN_AA,0x01);     //ʹ��ͨ��0���Զ�Ӧ��    
	SPI_NRF_WriteReg(NRF_WRITE_REG+EN_RXADDR,0x01); //ʹ��ͨ��0�Ľ��յ�ַ  
	SPI_NRF_WriteReg(NRF_WRITE_REG+SETUP_RETR,0x1a);//�����Զ��ط����ʱ��:500us + 86us;����Զ��ط�����:10��
	SPI_NRF_WriteReg(NRF_WRITE_REG+RF_CH,CHANAL);       //����RFͨ��ΪCHANAL
 // SPI_NRF_WriteReg(NRF_WRITE_REG+RF_SETUP,0x0f);  //����TX�������,0db����,2Mbps,���������濪��   
	SPI_NRF_WriteReg(NRF_WRITE_REG+RF_SETUP,0x07);  //����TX�������,0db����,1Mbps,���������濪��   
	SPI_NRF_WriteReg(NRF_WRITE_REG+CONFIG,0x0e);    //���û�������ģʽ�Ĳ���;  PWR_UP,  EN_CRC,  16BIT_CRC,  ����ģʽ,   ���������ж�
/*CE���ߣ����뷢��ģʽ*/	
	NRF_CE_HIGH();
	nRF24L01_status=nrf_mode_tx;
  Sys.Sleep(500); //CEҪ����һ��ʱ��Ž��뷢��ģʽ
}

/*
�˴��еȴ��жϵ� ����ѭ��   ע��       
��Ҫ�޸�
*/

/**
  * @brief   ���ڴ�NRF�Ľ��ջ������ж�������
  * @param   
  *		@arg rxBuf �����ڽ��ո����ݵ����飬�ⲿ����	
  * @retval 
  *		@arg ���ս��
  */
byte NRF_Rx_Dat(byte *rxbuf)
{
	byte state; 
	/*�ȴ������ж�*/
//	while(NRF_Read_IRQ()!=0); 
	NRF_CE_HIGH();	 //�������״̬
	NRF_CE_LOW();  	 //�������״̬
	/*��ȡstatus�Ĵ�����ֵ  */               
	state=SPI_NRF_ReadReg(STATUS);
	/* ����жϱ�־*/      
	SPI_NRF_WriteReg(NRF_WRITE_REG+STATUS,state);
	/*�ж��Ƿ���յ�����*/
	if(state&RX_DR)                                 //���յ�����
	{
	  SPI_NRF_ReadBuf(RD_RX_PLOAD,rxbuf,RX_PLOAD_WIDTH);//��ȡ����
	  SPI_NRF_WriteReg(FLUSH_RX,NOP);          //���RX FIFO�Ĵ���
	  return RX_DR; 
	}
	else    
		return ERROR;                    //û�յ��κ�����
}

/**
  * @brief   ������NRF�ķ��ͻ�������д������
  * @param   
  *		@arg txBuf���洢�˽�Ҫ���͵����ݵ����飬�ⲿ����	
  * @retval  ���ͽ�����ɹ�����TXDS,ʧ�ܷ���MAXRT��ERROR
  */
byte NRF_Tx_Dat(byte *txbuf)
{
	byte state;  
	 /*ceΪ�ͣ��������ģʽ1*/
	NRF_CE_LOW();
	/*д���ݵ�TX BUF ��� 32���ֽ�*/						
	SPI_NRF_WriteBuf(WR_TX_PLOAD,txbuf,TX_PLOAD_WIDTH);
      /*CEΪ�ߣ�txbuf�ǿգ��������ݰ� */   
 	NRF_CE_HIGH();
	  /*�ȴ���������ж� */                            
//	while(NRF_Read_IRQ()!=0); 	
	/*��ȡ״̬�Ĵ�����ֵ */                              
	state = SPI_NRF_ReadReg(STATUS);
	 /*���TX_DS��MAX_RT�жϱ�־*/                  
	SPI_NRF_WriteReg(NRF_WRITE_REG+STATUS,state); 	
	SPI_NRF_WriteReg(FLUSH_TX,NOP);    //���TX FIFO�Ĵ��� 
	 /*�ж��ж�����*/    
	if(state&MAX_RT)                     //�ﵽ����ط�����
			 return MAX_RT; 
	else if(state&TX_DS)                  //�������
		 	return TX_DS;
	else						  
			return ERROR;                 //����ԭ����ʧ��
} 

//2401ί�к���  δ���
void nRF24L01_irq(Pin pin, bool opk)
{
	byte a=3 ,b=5;
	(void)pin;
	(void)opk;
	switch(nRF24L01_status)
	{
		case 	nrf_mode_rx		:	a=b	;		
				break;
		case	 nrf_mode_tx	:	a+=b;		
				break;
		case	 nrf_mode_free	:	a-=b;		
				break;
	}
}

//��ʼ��nRF�����ӿ�
void TnRF_Init(TnRF* this)
{
    this->Open  	= nRF_Init;
    this->Check 	= TnRF_Check;
    this->RX_Mode = NRF_RX_Mode;
    this->TX_Mode = NRF_TX_Mode;
    this->Rx_Dat  = NRF_Rx_Dat;
    this->Tx_Dat 	= NRF_Tx_Dat;
}

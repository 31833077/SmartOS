#ifndef __NRF24L01_H__
#define __NRF24L01_H__



/********************  *************  ***********************/
/********************  *************  ***********************/


//ʹ���ĸ�spi��Ϊ nrf ͨ�ſ�
#define 	nRF2401_SPI					SPI_1


//�ж�����
#define 	nRF2401_IRQ_pin		 PA1


//�ж����ż��
#define NRF_Read_IRQ()		  Sys.IO.Read(nRF2401_IRQ_pin)  


//�Ƿ�ʹ�÷�Ĭ��csn����
#define 	Other_nRF_CSN 			0

//�Ƿ�ʹ��CE����
#define		us_nrf_ce						0



//CE���Ų���
#if us_nrf_ce

	//CE���Ŷ���
	#define   nRF2401_CE					PE10
	#define 	NRF_CE_LOW()				Sys.IO.Write(nRF2401_CE,0)
	#define 	NRF_CE_HIGH()				Sys.IO.Write(nRF2401_CE,1)
	
#else

	#define 	NRF_CE_LOW()				
	#define 	NRF_CE_HIGH()				
#endif


//csn���Ų���
#if	!Other_nRF_CSN

			//һ�����   csn������nss����
	#define  	NRF_CSN_LOW()				Sys.IO.Write(spi_nss[nRF2401_SPI],0)	
	#define  	NRF_CSN_HIGH()			Sys.IO.Write(spi_nss[nRF2401_SPI],1)	
						
			//�Զ���csn����
#else

	//CSN���Ŷ���
	#define   nRF2401_CSN						PE10
	#define 	NRF_CSN_LOW()					Sys.IO.Write(nRF2401_CSN,0)
	#define 	NRF_CSN_HIGH()				Sys.IO.Write(nRF2401_CSN,1)

#endif



//���建������С  ��λ  byte
#define RX_PLOAD_WIDTH				5
#define TX_PLOAD_WIDTH				5

extern unsigned char RX_BUF[];		//�������ݻ���
extern unsigned char TX_BUF[];		//�������ݻ���



/********************  *************  ***********************/
/********************  *************  ***********************/


/*���Ͱ���С*/
#define TX_ADR_WIDTH	5
#define RX_ADR_WIDTH	5


// ����һ����̬���͵�ַ
/*��ʼ��ַ����cȥ����*/
extern unsigned char TX_ADDRESS[];  		
extern unsigned char RX_ADDRESS[]; 


#define CHANAL 				40	//Ƶ��ѡ�� 


/********************  *************  ***********************/
/********************  *************  ***********************/


// SPI(nRF24L01) commands ,	NRF��SPI����궨�壬���NRF����ʹ���ĵ�
#define NRF_READ_REG    0x00  // Define read command to register
#define NRF_WRITE_REG   0x20  // Define write command to register
#define RD_RX_PLOAD 0x61  // Define RX payload register address
#define WR_TX_PLOAD 0xA0  // Define TX payload register address
#define FLUSH_TX    0xE1  // Define flush TX register command
#define FLUSH_RX    0xE2  // Define flush RX register command
#define REUSE_TX_PL 0xE3  // Define reuse TX payload register command
#define NOP         0xFF  // Define No Operation, might be used to read status register

// SPI(nRF24L01) registers(addresses) ��NRF24L01 ��ؼĴ�����ַ�ĺ궨��
#define CONFIG      0x00  // 'Config' register address
#define EN_AA       0x01  // 'Enable Auto Acknowledgment' register address
#define EN_RXADDR   0x02  // 'Enabled RX addresses' register address
#define SETUP_AW    0x03  // 'Setup address width' register address
#define SETUP_RETR  0x04  // 'Setup Auto. Retrans' register address
#define RF_CH       0x05  // 'RF channel' register address
#define RF_SETUP    0x06  // 'RF setup' register address
#define STATUS      0x07  // 'Status' register address
#define OBSERVE_TX  0x08  // 'Observe TX' register address
#define CD          0x09  // 'Carrier Detect' register address
#define RX_ADDR_P0  0x0A  // 'RX address pipe0' register address
#define RX_ADDR_P1  0x0B  // 'RX address pipe1' register address
#define RX_ADDR_P2  0x0C  // 'RX address pipe2' register address
#define RX_ADDR_P3  0x0D  // 'RX address pipe3' register address
#define RX_ADDR_P4  0x0E  // 'RX address pipe4' register address
#define RX_ADDR_P5  0x0F  // 'RX address pipe5' register address
#define TX_ADDR     0x10  // 'TX address' register address
#define RX_PW_P0    0x11  // 'RX payload width, pipe0' register address
#define RX_PW_P1    0x12  // 'RX payload width, pipe1' register address
#define RX_PW_P2    0x13  // 'RX payload width, pipe2' register address
#define RX_PW_P3    0x14  // 'RX payload width, pipe3' register address
#define RX_PW_P4    0x15  // 'RX payload width, pipe4' register address
#define RX_PW_P5    0x16  // 'RX payload width, pipe5' register address
#define FIFO_STATUS 0x17  // 'FIFO Status Register' register address




/********************     �ж����    ***********************/
/********************  *************  ***********************/



#define MAX_RT      0x10 //�ﵽ����ط������жϱ�־λ

#define TX_DS		0x20 //��������жϱ�־λ	  // 

#define RX_DR		0x40 //���յ������жϱ�־λ



/********************  *************  ***********************/
/********************  *************  ***********************/


/*	�˴�����ԭ�Ͳ���Ҫ    ֱ��ʹ��Sys.nRF.xxxx();
//��ʼ��		����ί������
void SPI_NRF_Init(void);
//����Ƿ������ӵ�Ӳ��
byte NRF_Check(void);
//���ý���ģʽ
void NRF_RX_Mode(void);
//��������
byte NRF_Rx_Dat(byte *rxbuf);
//���÷���ģʽ
void NRF_TX_Mode(void);
//��������
byte NRF_Tx_Dat(byte *txbuf);
//2401ί�к���
void nRF24L01_irq(Pin pin, bool opk);
*/


#endif

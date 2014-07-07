#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <stdio.h>

#include "stm32.h"


/* ���Ͷ��� */
typedef char            sbyte;
typedef unsigned char   byte;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef char*           string;
typedef unsigned char   bool;
#define true            1
#define false           0


/* ���ڶ��� */
#define COM1 0
#define COM2 1
#define COM3 2
#define COM4 3
#define COM5 4
#define COM_NONE 0xFF

/*Spi����*/
//SPI1..���ָ�ʽ��st���ͻ  
#define SPI_1	0
#define SPI_2	1
#define SPI_3	2
#define SPI_NONE 0XFF


/* ���Ŷ��� */
typedef ushort			Pin;
#include "Pin.h"

/* �������� */
/* �Խṹ��������һ�����ͷ����β����ͬʱ����һ��ȫ�ֵĳ�ʼ������ */
#define _class(name) typedef struct T##name##_Def\
{\
	void (*Init)(struct T##name##_Def* this);
	
//	void (*Uninit)(struct T##name##_Def* this);

#define _class_end(name) } T##name;\
extern void T##name##_Init(T##name* this);


/* ���Ķ��� */
_class(Core)
    void (*Printf)(const string format, ...);
    /*void (*LcdPrintf)(const string format,...);
    void* (*Malloc)(uint len);
    void (*Free)(void* ptr);*/
    //uint (*WaitForEvents)(uint wakeupSystemEvents, uint timeout_Milliseconds);
    //uint (*ComputeCRC)(const void* rgBlock, int nLength, uint crc);
_class_end(Core)

/* ���� */
_class(Boot)
_class_end(Boot)

// ��ȡί��
typedef void (*IOReadHandler)(Pin , bool );
/* IO�� */
_class(IO)
    void (*Open)(Pin , GPIOMode_TypeDef );
    // mode=GPIO_Mode_IN/GPIO_Mode_OUT/GPIO_Mode_AF/GPIO_Mode_AN
    // speed=GPIO_Speed_50MHz/GPIO_Speed_2MHz/GPIO_Speed_10MHz
    // type=GPIO_OType_PP/GPIO_OType_OD
		
#ifdef STM32F0XX
   void (*OpenPort)(Pin , GPIOMode_TypeDef , GPIOSpeed_TypeDef , GPIOOType_TypeDef ,GPIOPuPd_TypeDef );
#else
    void (*OpenPort)(Pin , GPIOMode_TypeDef , GPIOSpeed_TypeDef );
#endif
    void (*Close)(Pin );
    bool (*Read)(Pin );
    void (*Write)(Pin , bool );
    void (*Register)(Pin , IOReadHandler );
		void (* SetShakeTime)(byte time_ms);	/*����ȥ������   Ĭ����ʱ70ms*/
_class_end(IO)

// ��ȡί��
typedef void (*UsartReadHandler)(int com, byte data);
/* ���� */
_class(Usart)
    bool (*Open)(int com, int baudRate);
    bool (*Open2)(int com, int baudRate, int parity, int dataBits, int stopBits);
    void (*Close)(int com);
    void (*Write)(int com, const string data, int size);
    int  (*Read)(int com, string data, uint size);
    void (*Flush)(int com);
    void (*Register)(int com, UsartReadHandler handler);
	void (*SetRemap)(int com);
    //int  (*BytesInBuffer)(int com, bool fRx);
    //void (*DiscardBuffer)(int com, bool fRx);
_class_end(Usart)

/* �ڴ� */
/*_class(Mem)
    int (*snprintf)(string buffer, uint len, const string format, ...);
    int (*stricmp)(const string dst, const string src);
    int (*strncmp)(const string str1, const string str2, uint num);
    uint (*strlen)(const string str);
    void *(*memcpy)(void * dst, const void * src, uint len);
    void *(*memset)(void * dst, int value, uint len);
_class_end(Mem)*/

/* Flash�洢 */
_class(Flash)
    int (*Erase)(uint address, uint count);
    int (*Read)(uint address, uint count,byte *buffer);
    int (*Write)(uint address, uint count,byte *buffer);
_class_end(Flash)

/* ģ���� */
_class(Analog)
	//void (*DA_Write)(ANALOG_CHANNEL channel, int level);
	//bool (*AD_Initialize)(ANALOG_CHANNEL channel, int precisionInBits);
	//int (*AD_Read)(ANALOG_CHANNEL channel);
_class_end(Analog)

/* �������� */
_class(Spi)
		bool 		(*Open)(int);
		bool 		(*Close)(int);
		byte 		(*WriteReadByte8)(int spi,byte TxData);
		ushort 	(*WriteReadByte16)(int spi,ushort TxData);					
    //bool (*WriteRead)(const SPI_CONFIGURATION& Configuration, byte* Write8, int WriteCount, byte* Read8, int ReadCount, int ReadStartOffset);
    //bool (*WriteRead16)(const SPI_CONFIGURATION& Configuration, ushort* Write16, int WriteCount, ushort* Read16, int ReadCount, int ReadStartOffset);
_class_end(Spi)


/* nRF24L01+ */
_class(nRF)
		void (*Open)(void);
		bool (*Check)(void);
		void (*RX_Mode)(void);
		void (*TX_Mode)(void);
		byte (*Rx_Dat)(byte *rxbuf);
		byte (*Tx_Dat)(byte *txbuf);
_class_end(nRF)



/*typedef struct
{
	void (*Init)(void);
	void (*Uninit)(void);

    bool (*Initialize)();
    bool (*Uninitialize)();
    bool (*Execute)(ushort address,byte *inBuffer,int inCount,byte *outBuffer,int outCount,uint clockRateKhz,int timeout);
} TI2c;

typedef struct
{
	void (*Init)(void);
	void (*Uninit)(void);

    //bool (*Initialize)(PWM_CHANNEL channel);
    //bool (*Uninitialize)(PWM_CHANNEL channel);
    //bool (*ApplyConfiguration)(PWM_CHANNEL channel, Pin pin, uint& period, uint& duration, PWM_SCALE_FACTOR &scale, bool invert);
    //bool (*Start)(PWM_CHANNEL channel, Pin pin);
    //void (*Stop)(PWM_CHANNEL channel, Pin pin);
    //Pin (*GetPinForChannel)(PWM_CHANNEL channel);
} TPwm;

typedef struct
{
	void (*Init)(void);
	void (*Uninit)(void);

    void (*Clear)(uint color);
    void (*SetPixel)(int x,int y,uint color);
    uint (*GetPixel)(int x,int y);
    void (*DrawLine)(int x1,int y1,int x2,int y2,uint color);
    void (*DrawRectangle)(int x,int y,int width,int height,uint color);
    void (*DrawEllipse)(int x,int y,int width,int height,uint color);
    void (*DrawImage)(int x,int y,byte *bytData);
    void (*DrawImageEx)(int x,int y,byte *bytData,uint MaskColor);
    void (*DrawString)(int x,int y, string s,uint color);
    void (*DrawStringEx)(int x,int y,uint color,byte *fontdata,int width,int height,int count);
    void (*FillRectangle)(int x,int y,int width,int height,uint color);
    void (*FillEllipse)(int x,int y,int width,int height,uint color);
    void (*GetFrameBufferEx)(byte *bytData,uint offset,uint size);
    void (*SuspendLayout)();
    void (*ResumeLayout)();
} TLcd;
*/

/* ��־ */
/*_class(Log)
    int MessagePort;    // ��Ϣ�ڣ�Ĭ��0��ʾUSART1

    void (*WriteLine)(const string format, ...);    // ���һ����־���Զ�����
    void (*DebugLine)(const string format, ...);    // ���һ����־��Sys.Debugʱ��Ч
_class_end(Log)
*/

// ȫ��ϵͳ��
typedef struct
{
	void (*Init)(void);
	void (*Uninit)(void);
	
    bool Debug;  // �Ƿ����
	uint Clock;  // ϵͳʱ��
#if GD32F1
    uint CystalClock;   // ����ʱ��
#endif
    byte MessagePort;    // ��Ϣ�ڣ�Ĭ��0��ʾUSART1
    uint ID[3]; // оƬID
    uint FlashSize; // оƬFlash����

	//TBoot Boot;
	TCore Core;
	TIO IO;
	TUsart Usart;
/*TMem Mem;
	TFlash Flash;
	TAnalog Analog;*/
	TSpi Spi;
	TnRF	nRF;
/*TI2c I2c;
	TPwm Pwm;
	TLcd Lcd;
	TLog Log;*/

    void (*Sleep)(uint ); // ���뼶�ӳ�
    void (*Delay)(uint ); // ΢�뼶�ӳ�
    void (*DisableInterrupts)();    // �ر��ж�
    void (*EnableInterrupts)();     // ���ж�
} TSystem;

// ����ȫ�ֵ�Sys������
extern TSystem Sys;

// ʹ�ú���ģ��ĺ궨��
#define using(module) Sys.module.Init = T##module##_Init;

#endif //_SYSTEM_H_

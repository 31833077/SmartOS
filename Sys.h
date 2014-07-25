#ifndef _Sys_H_
#define _Sys_H_

#include <stdio.h>
#include "stm32.h"

/* ���Ͷ��� */
typedef char            sbyte;
typedef unsigned char   byte;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef char*           string;
//typedef unsigned char   bool;
#define true            1
#define false           0

/*Spi����*/
//SPI1..���ָ�ʽ��st���ͻ  
#define SPI_1	0
#define SPI_2	1
#define SPI_3	2
#define SPI_NONE 0XFF

/* ���Ŷ��� */
typedef ushort			Pin;
#include "Pin.h"

// ϵͳ��
class TSys
{
public:
    bool Debug;  // �Ƿ����
	uint Clock;  // ϵͳʱ��
#if GD32F1
    uint CystalClock;    // ����ʱ��
#endif
    byte MessagePort;    // ��Ϣ�ڣ�Ĭ��0��ʾUSART1
    uint ID[3];      // оƬID
    uint FlashSize;  // оƬFlash����
    void Init();     // ��ʼ��ϵͳ
    void Sleep(uint ms); // ���뼶�ӳ�
    void Delay(uint us); // ΢�뼶�ӳ�
    void DisableInterrupts();    // �ر��ж�
    void EnableInterrupts();     // ���ж�
};

extern TSys Sys;

#endif //_Sys_H_

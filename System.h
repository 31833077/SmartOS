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
//typedef unsigned char   bool;
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

#endif //_SYSTEM_H_

#include "System.h"

#if STM32F1XX
#include "stm32f10x_exti.h"
#else
#include "stm32f0xx_exti.h"
#endif

#ifndef BIT
    #define BIT(x)	(1 << (x))
#endif

/* �ж�״̬�ṹ�� */
/* һ��16���ж��ߣ���ζ��ͬһ����ÿһ��ֻ����һ������ʹ���ж� */
typedef struct TIntState
{
    Pin Pin;
    IOReadHandler Handler;
    bool OldValue;
} IntState;
// 16���ж���
static IntState State[16];

#ifdef STM32F0XX

#define _RCC_AHB(PIN) (RCC_AHBENR_GPIOAEN << (PIN >> 4))

// �򿪶˿�
// mode=GPIO_Mode_IN/GPIO_Mode_OUT/GPIO_Mode_AF/GPIO_Mode_AN
// speed=GPIO_Speed_50MHz/GPIO_Speed_2MHz/GPIO_Speed_10MHz
// type=GPIO_OType_PP/GPIO_OType_OD
void TIO_OpenPort(Pin pin, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed, GPIOOType_TypeDef type)
{
    GPIO_InitTypeDef p;
    
    // ��ʼ���ṹ�壬���������Ϊ�˽�ʡ����ռ䣬Ҳ���Բ���
    GPIO_StructInit(&p);
    
    // ��ʱ��
    RCC_AHBPeriphClockCmd(_RCC_AHB(pin), ENABLE);
    //RCC->AHBENR |= _RCC_AHB(pin);

    p.GPIO_Pin = _PORT(pin);
    p.GPIO_Speed = speed;
    p.GPIO_Mode = mode;
    p.GPIO_OType = type;	
    GPIO_Init(_GROUP(pin), &p);
}

// �����÷�ʽ�򿪶˿�
void TIO_Open(Pin pin, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef p;
    GPIO_StructInit(&p);
    
    // ��ʱ��
    RCC_AHBPeriphClockCmd(_RCC_AHB(pin), ENABLE);

    p.GPIO_Pin = _PORT(pin);
    p.GPIO_Mode = mode;

    GPIO_Init(_GROUP(pin), &p);
}

// �رն˿ڡ���Ϊ��������
void TIO_Close(Pin pin)
{
    TIO_OpenPort(pin, GPIO_Mode_IN, GPIO_Speed_2MHz, GPIO_OType_PP);
    // ���������ر���������飬������
    //GPIO_DeInit(_GROUP(pin));
}
#else

#define _RCC_APB2(PIN) (RCC_APB2Periph_GPIOA << (PIN >> 4))

// �򿪶˿�
// mode=GPIO_Mode_IN/GPIO_Mode_OUT/GPIO_Mode_AF/GPIO_Mode_AN
// speed=GPIO_Speed_50MHz/GPIO_Speed_2MHz/GPIO_Speed_10MHz
void TIO_OpenPort(Pin pin, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed)
{
    GPIO_InitTypeDef p;
    
    // ��ʼ���ṹ�壬���������Ϊ�˽�ʡ����ռ䣬Ҳ���Բ���
    GPIO_StructInit(&p);
    
    // ��ʱ��
    RCC_APB2PeriphClockCmd(_RCC_APB2(pin), ENABLE);
    //RCC->APB2ENR |= _RCC_AHB(pin);

    p.GPIO_Pin = _PORT(pin);
    p.GPIO_Speed = speed;
    p.GPIO_Mode = mode;
    GPIO_Init(_GROUP(pin), &p);
}

// �����÷�ʽ�򿪶˿�
void TIO_Open(Pin pin, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef p;
    GPIO_StructInit(&p);
    
    // ��ʱ��
    RCC_APB2PeriphClockCmd(_RCC_APB2(pin), ENABLE);

    p.GPIO_Pin = _PORT(pin);
    p.GPIO_Mode = mode;

    GPIO_Init(_GROUP(pin), &p);
}

// �رն˿ڡ���Ϊ��������
void TIO_Close(Pin pin)
{
    TIO_Open(pin, GPIO_Mode_IN_FLOATING);
}
#endif

// ���ö˿�״̬
void TIO_Write(Pin pin, bool value)
{
    if(value)
        GPIO_SetBits(_GROUP(pin), _PORT(pin));
    else
        GPIO_ResetBits(_GROUP(pin), _PORT(pin));
}

// ��ȡ�˿�״̬
bool TIO_Read(Pin pin)
{
    GPIO_TypeDef* group = _GROUP(pin);
    return (group->IDR >> (pin & 0xF)) & 1;
}

// ע��ص�
void TIO_Register(Pin pin, IOReadHandler handler)
{
    //byte port = _PORT(pin);
    byte pins = pin & 0x0F;
    IntState* state = &State[pins];
    EXTI_InitTypeDef   EXTI_InitStructure;
    NVIC_InitTypeDef   NVIC_InitStructure;
    
    // ע���ж��¼�
    if(handler)
    {
        // ����Ƿ��Ѿ�ע�ᵽ���������
        if(state->Pin != pin && state->Pin != P0)
        {
#if DEBUG
            printf("EXTI%d can't register to P%c%d, it has register to P%c%d\r\n", pins, _PIN_NAME(pin), _PIN_NAME(state->Pin));
#endif
            return;
        }
        
        state->Pin = pin;
        state->Handler = handler;

        /* ��ʱ�� */
#ifdef STM32F0XX
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

        SYSCFG_EXTILineConfig(pin>>4, pins);
#else
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

        GPIO_EXTILineConfig(pin>>4, pins);
#endif

        /* Configure EXTI0 line */
        //EXTI_InitStructure.EXTI_Line = EXTI_Line0;
        EXTI_InitStructure.EXTI_Line = EXTI_Line0 << pins;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        /* Enable and set EXTI0 Interrupt to the lowest priority */
#ifdef STM32F10X
        //NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
        if(pins < 5)
            NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn + 6;
        else if(pins < 10)
            NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
        else
            NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;

        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
#else
        NVIC_InitStructure.NVIC_IRQChannel = EXTI0_1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPriority = 0x00;
#endif
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }
    else
    {
        // ȡ��ע��
        state->Pin = P0;
        state->Handler = 0;
    }
}

#define IT 1
#ifdef IT
void GPIO_ISR (int num)  // 0 <= num <= 15
{
    IntState* state = &State[num];
    uint bit = 1 << num;
    bool value;
    //byte line = EXTI_Line0 << num;

    // ���δָ��ί�У��򲻴���
    if(!state->Handler) return;
    
    //if(EXTI_GetITStatus(line) == RESET) return;
    
    do {
        //value = TIO_Read(state->Pin); // ��ȡ����״̬
        EXTI->PR = bit;   // ���ù���λ
        value = TIO_Read(state->Pin); // ��ȡ����״̬
        
        Sys.Sleep(20); // ���ⶶ��
    } while (EXTI->PR & bit); // ����ٴι������ظ�

    //EXTI_ClearITPendingBit(line);

    if(state->Handler)
    {
        state->Handler(state->Pin, value);
    }
}

void EXTI0_IRQHandler (void) { GPIO_ISR(0); } // EXTI0
void EXTI1_IRQHandler (void) { GPIO_ISR(1); } // EXTI1
void EXTI2_IRQHandler (void) { GPIO_ISR(2); } // EXTI2
void EXTI3_IRQHandler (void) { GPIO_ISR(3); } // EXTI3
void EXTI4_IRQHandler (void) { GPIO_ISR(4); } // EXTI4

void EXTI9_5_IRQHandler (void) // EXTI5 - EXTI9
{
    uint pending = EXTI->PR & EXTI->IMR & 0x03E0; // pending bits 5..9
    int num = 5; pending >>= 5;
    do {
        if (pending & 1) GPIO_ISR(num);
        num++; pending >>= 1;
    } while (pending);
}

void EXTI15_10_IRQHandler (void) // EXTI10 - EXTI15
{
    uint pending = EXTI->PR & EXTI->IMR & 0xFC00; // pending bits 10..15
    int num = 10; pending >>= 10;
    do {
        if (pending & 1) GPIO_ISR(num);
        num++; pending >>= 1;
    } while (pending);
}
#endif

// IO�˿ں�����ʼ��
void TIO_Init(TIO* this)
{
    int i = 0;
    
    this->Open = TIO_Open;
    this->OpenPort = TIO_OpenPort;
    this->Write = TIO_Write;
    this->Read = TIO_Read;
    this->Register = TIO_Register;
    
    for(i=0; i<16; i++) TIO_Register(i, 0);
}

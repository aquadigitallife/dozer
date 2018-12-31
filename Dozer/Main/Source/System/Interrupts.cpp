#include "Global.h"

ISR(NMI_Handler)
{
	// Сбрасываем флаг прерывания от CSS
	if (RCC->CIR & RCC_CIR_CSSF)
		RCC->CIR |= RCC_CIR_CSSC;

	// TODO Пытаемся восстановить тактирование

	// TODO Можно настройку тактирования сделать в виде задачи
	// и здесь выставлять светофор инициализации

	while (1);
}

ISR(HardFault_Handler)
{
	volatile uint32_t HFSR_Val = *(uint32_t*)0xE000ED2C;
	volatile uint8_t  BFSR_Val = *(uint8_t*)0xE000ED29;
	volatile uint8_t  MFSR_Val = *(uint8_t*)0xE000ED28;

	volatile SCB_Type scb = *SCB;

	(void)HFSR_Val;
	(void)BFSR_Val;
	(void)MFSR_Val;
	(void)scb;

	while (1) ;
}

ISR(MemManage_Handler)
{
	volatile uint32_t HFSR_Val = *(uint32_t*)0xE000ED2C;
	volatile uint8_t  BFSR_Val = *(uint8_t*)0xE000ED29;
	volatile uint8_t  MFSR_Val = *(uint8_t*)0xE000ED28;

	volatile SCB_Type scb = *SCB;

	(void)HFSR_Val;
	(void)BFSR_Val;
	(void)MFSR_Val;
	(void)scb;

	while (1) ;
}

ISR(BusFault_Handler)
{
	volatile uint32_t HFSR_Val = *(uint32_t*)0xE000ED2C;
	volatile uint8_t  BFSR_Val = *(uint8_t*)0xE000ED29;
	volatile uint8_t  MFSR_Val = *(uint8_t*)0xE000ED28;

	volatile SCB_Type scb = *SCB;

	(void)HFSR_Val;
	(void)BFSR_Val;
	(void)MFSR_Val;
	(void)scb;

	while (1) ;
}

ISR(UsageFault_Handler)
{
	volatile uint32_t HFSR_Val = *(uint32_t*)0xE000ED2C;
	volatile uint8_t  BFSR_Val = *(uint8_t*)0xE000ED29;
	volatile uint8_t  MFSR_Val = *(uint8_t*)0xE000ED28;

	volatile SCB_Type scb = *SCB;

	(void)HFSR_Val;
	(void)BFSR_Val;
	(void)MFSR_Val;
	(void)scb;

	while (1) ;
}


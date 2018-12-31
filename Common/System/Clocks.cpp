#include "Global.h"

uint32_t InitRCC()
{
	uint32_t Res = 1;

	volatile uint32_t HSE_Status = 0;

#ifdef HSE_EXIST
	// Включаем HSE
	RCC->CR |= RCC_CR_HSEON;

	volatile uint32_t HSE_Counter = 0;
	// Ждём пока HSE включится или выйдет таймаут
	do
	{
		HSE_Status = RCC->CR & RCC_CR_HSERDY;
		HSE_Counter++;
	} while (HSE_Status == 0 && HSE_Counter < HSE_STARTUP_TIMEOUT);

	// Проверяем, включился ли HSE
	if (RCC->CR & RCC_CR_HSERDY)
		HSE_Status = 1;
	else
		HSE_Status = 0;

	Res = (HSE_Status != 0);
#endif

	// Включаем буфер предвыборки, так как flash не может работать на высокой частоте.
	// Так же включаем пять циклов ожидания (для частот больше 150 МГц)
	// Дополнительно включаем кэш инструкций и данных
	FLASH->ACR |=  FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |=  FLASH_ACR_LATENCY_5WS;

	// Настраиваем деление частоты на шинах
	RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
	RCC->CFGR |=  RCC_CFGR_HPRE_DIV1  | // AHB - без делителя
	              RCC_CFGR_PPRE2_DIV2 | // APB2 - делитель 2
	              RCC_CFGR_PPRE1_DIV4;  // APB1 - делитель 4

	// Коэф. умножения частоты
	RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP |
	                  RCC_PLLCFGR_PLLQ |
	                  RCC_PLLCFGR_PLLSRC);

	// PLLP:
	// 00 = /2
	// 01 = /4
	// 10 = /6
	// 11 = /8

	// Если HSE запустился
	if (HSE_Status)
	{
		RCC->PLLCFGR |=  ( 25 <<  0) | // PLLM = /25
		                 (360 <<  6) | // PLLN = *192
		                 (  0 << 16) | // PLLP = /2
		                 (  8 << 24) | // PLLQ = /8
		// Источник частоты для ФАПЧ - внешний резонатор
		                 RCC_PLLCFGR_PLLSRC_HSE;

		// Включаем контроль кварца
		RCC->CR |= RCC_CR_CSSON;
	}
	else
	{
		RCC->PLLCFGR |=  ( 16 <<  0) | // PLLM = /16
		                 (360 <<  6) | // PLLN = *192
		                 (  0 << 16) | // PLLP = /2
		                 (  8 << 24) | // PLLQ = /8
		// Источник частоты для ФАПЧ - RC цепочка
		                 RCC_PLLCFGR_PLLSRC_HSI;
	}

	// Ждём, пока PLL включится
	RCC->CR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));
	
	// Включаем "Over-drive mode" для корректной работы на частоте более 168 МГц
	if (F_SYS > 168000000UL)
	{
		PWR->CR |= PWR_CR_ODEN;
		while (!(PWR->CSR & PWR_CSR_ODRDY));

		PWR->CR |= PWR_CR_ODSWEN;
		while (!(PWR->CSR & PWR_CSR_ODSWRDY));
	}

	// Выбираем PLL как источник тактовой частоты
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// Ждём, пока PLL выберется как источник тактовой частоты
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

	return Res;
}

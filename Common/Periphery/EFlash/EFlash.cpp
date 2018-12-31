#include "Global.h"

namespace EFLASH
{

uint8_t C_EFLASH::CheckLock()
{
	return ((FLASH->CR & FLASH_CR_LOCK) == FLASH_CR_LOCK);
}

uint8_t C_EFLASH::CheckLockOPT()
{
#if defined(STM32F10X_MD) || defined(STM32F0XX)

	return !((FLASH->CR & FLASH_CR_OPTWRE) == FLASH_CR_OPTWRE);

#elif defined(STM32F411xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)

	return ((FLASH->OPTCR & FLASH_OPTCR_OPTLOCK) == FLASH_OPTCR_OPTLOCK);

#endif

	return 0;
}

void C_EFLASH::WaitBusy()
{
	while (FLASH->SR & FLASH_SR_BSY)
	{
#ifdef IWDG_RESET
		IWDG_RESET();
#endif
	}
}

void C_EFLASH::WaitEOP()
{
	while (!(FLASH->SR & FLASH_SR_EOP))
	{
#ifdef IWDG_RESET
		IWDG_RESET();
#endif
	}
}

int32_t C_EFLASH::Unlock()
{
	WaitBusy();

	// Читаем флаг блокировки
	if (!CheckLock())
	{
		// Память уже разблокированна
		return ERR_OK;
	}

	// Разблокируем flash
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;

	// Разблокировалась ли память?
	if (CheckLock())
	{
		// Разблокировка не получилась
		return ERR_CANT_UNLOCK;
	}

	return ERR_OK;
}

void C_EFLASH::Lock()
{
	FLASH->CR = FLASH_CR_LOCK;
}

int32_t C_EFLASH::Unlock_OPT()
{
	// Разблокируем запись в основную память
	int32_t Res = Unlock();
	if (Res)
		return Res;

	if (!CheckLockOPT())
		return ERR_OK; // Запись уже разрешена

	// Разблокируем optin bytes

#if defined(STM32F10X_MD) || defined(STM32F0XX)

	FLASH->OPTKEYR = 0x45670123;
	FLASH->OPTKEYR = 0xCDEF89AB;

#elif defined(STM32F411xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)

	FLASH->OPTKEYR = 0x08192A3B;
	FLASH->OPTKEYR = 0x4C5D6E7F;

#endif

	if (CheckLockOPT())
	{
		// Разблокировка не получилась
		return ERR_CANT_UNLOCK;
	}

	return ERR_OK;
}

void C_EFLASH::Lock_OPT()
{
#if defined(STM32F10X_MD) || defined(STM32F0XX)
	FLASH->CR &= ~FLASH_CR_OPTWRE;
#elif defined(STM32F411xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
	FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;
#endif
	Lock();
}

#if defined(STM32F10X_MD) || defined(STM32F0XX)
int32_t C_EFLASH::RewriteOB(const OB_TypeDef* const NewOB)
{
	WaitBusy();
	if (FLASH->SR & FLASH_SR_EOP)
		FLASH->SR = FLASH_SR_EOP;

	// Стираем option bytes
	FLASH->CR |= FLASH_CR_OPTER;
	FLASH->CR |= FLASH_CR_STRT;

	// Ждём окончания
	WaitBusy();
	FLASH->CR &= ~(FLASH_CR_OPTER);

	volatile uint16_t*    ob = (uint16_t*)OB_BASE;
	volatile uint16_t* newob = (uint16_t*)NewOB;

	FLASH->CR |= FLASH_CR_OPTPG;

	for (uint8_t i = 0; i < sizeof(OB_TypeDef) / sizeof(uint16_t); i++)
	{
		ob[i] = newob[i];
		WaitBusy();
		FLASH->SR = FLASH_SR_EOP;

		if ((ob[i] & 0xFF) != (newob[i] & 0xFF))
		{
			while (1)
				__NOP();
		}
	}

	FLASH->CR &= ~FLASH_CR_OPTPG;

	return ERR_OK;
}
#endif

#if defined(STM32F411xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)

int32_t C_EFLASH::RewriteOB(const uint8_t RDP /* = 0xAA*/, const uint8_t BOR /* = 3*/, const uint8_t WDG_SW /* = 1*/)
{
	WaitBusy();

	if (CheckLockOPT())
		return ERR_LOCK;

	if (RDP == 0xCC)
		return ERR_OK; // Не даём заблокировать контроллер навегда

	uint32_t NewOB = 0x0FFF00C0UL | (RDP << 8) | ((BOR & 3) << 2) | ((WDG_SW & 1) << 5);
	FLASH->OPTCR = NewOB | FLASH_OPTCR_OPTSTRT;

	WaitBusy();

	return ERR_OK;

	(void)NewOB;
}

#endif

#if defined(STM32F10X_MD) || defined(STM32F0XX)

uint32_t C_EFLASH::GetPageAddr(const uint32_t PageIndex)
{
	return FLASH_BASE + PAGE_SIZE * PageIndex;
}

#endif

#if defined(STM32F411xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)

int32_t C_EFLASH::SectorErase(const uint8_t SectorIndex)
{
	if (CheckLock())
		return ERR_LOCK;

	WaitBusy();

	uint32_t SR = 0;

	// Запускаем стирание сектора
	FLASH->CR = (((SectorIndex % SECTOR_COUNT) & 0x0F) << 3) | FLASH_CR_SER | FLASH_CR_PSIZE_1;
	FLASH->CR |= FLASH_CR_STRT;

	// Ждём окончания
	WaitBusy();
	FLASH->CR = 0;

	SR = FLASH->SR;
	(void)(SR);

	if (SR)
		while (1);

	return ERR_OK;
}

#elif defined(STM32F10X_MD) || defined(STM32F0XX)

int32_t C_EFLASH::PageErase(const uint32_t Addr)
{
	if (CheckLock())
		return ERR_LOCK;

	WaitBusy();
	if (FLASH->SR & FLASH_SR_EOP)
		FLASH->SR = FLASH_SR_EOP;

	uint8_t WPE; // Write protection error
	uint8_t PE ; // Programming error

	// Запускаем стирание страницы
	FLASH->CR = FLASH_CR_PER;
	FLASH->AR = Addr;
	FLASH->CR |= FLASH_CR_STRT;

	WaitEOP();
	FLASH->CR = 0;
	FLASH->SR = FLASH_SR_EOP;

	WPE = (FLASH->SR & FLASH_SR_WRPRTERR) == FLASH_SR_WRPRTERR;
	PE  = (FLASH->SR & FLASH_SR_PGERR   ) == FLASH_SR_PGERR   ;

	(void)(WPE);
	(void)(PE );

	FLASH->SR = FLASH_SR_WRPRTERR | FLASH_SR_PGERR;

	return ERR_OK;
}
#endif

void C_EFLASH::ReadData(const uint32_t Addr, const void* Data, const uint32_t Size)
{
	WaitBusy();

	// TODO Проверь скорость, если читать по 32 бита за раз

	volatile uint8_t *FlashData = (volatile uint8_t*)(Addr);
	volatile uint8_t *UserData  = (volatile uint8_t*)(Data);

	for (uint32_t i = 0; i < Size; i++)
		UserData[i] = FlashData[i];
}

#if defined(STM32F411xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)

int32_t C_EFLASH::WriteData(const uint32_t Addr, const void* Data, const uint32_t Size)
{
	if (CheckLock())
		return ERR_LOCK;

	// Ждём освобождения памяти
	WaitBusy();

	uint32_t SR = 0;

	// Рзрешаем программирование
	// Пишем по 32 бита для более быстрой записи
	FLASH->CR = FLASH_CR_PG | FLASH_CR_PSIZE_1;

	uint32_t FullSize = Size / sizeof(uint32_t); // Размер для записи в словах
	uint8_t  TailSize = Size - FullSize * sizeof(uint32_t); // Хвост в байтах

	volatile uint32_t *FlashData32 = (volatile uint32_t*)(Addr);
	volatile uint32_t *UserData32  = (volatile uint32_t*)(Data);
	volatile uint32_t  CheckData;

	for (uint32_t i = 0; i < FullSize; i++)
	{
		FlashData32[i] = UserData32[i];

		WaitBusy();

		CheckData = FlashData32[i];

		if (CheckData != UserData32[i])
		{
			while (1)
				__NOP();
		}

		SR = FLASH->SR;

		if (SR)
		{
			while (1)
				__NOP();
		}
	}

	// Переключаемся на запись по байтам
	FLASH->CR &= ~FLASH_CR_PSIZE;

	volatile uint8_t *FlashData8   = (volatile uint8_t*)(Addr) + FullSize;
	volatile uint8_t *UserData8    = (volatile uint8_t*)(Data) + FullSize;

	for (uint8_t i = 0; i < TailSize; i++)
	{
		FlashData8[i] = UserData8[i];

		WaitBusy();

		SR = FLASH->SR;
		(void)(SR);

		if (SR)
			while (1);

	}

	FLASH->CR = 0;

	return ERR_OK;
}

#elif defined(STM32F10X_MD) || defined(STM32F0XX)

int32_t C_EFLASH::WriteData(const uint32_t Addr, const void* Data, const uint32_t Size)
{
	if (CheckLock())
		return ERR_LOCK;

	// Ждём освобождения памяти
	WaitBusy();
	if (FLASH->SR & FLASH_SR_EOP)
		FLASH->SR = FLASH_SR_EOP;

	uint8_t WPE; // Write protection error
	uint8_t PE ; // Programming error

	// Рзрешаем программирование
	FLASH->CR = FLASH_CR_PG;

	// Для STM32F103C8 можно записывать по 16 бит за раз
	volatile uint16_t *FlashData  = (volatile uint16_t*)(Addr);
	volatile uint16_t *UserData16 = (volatile uint16_t*)(Data);
	volatile uint8_t  *UserData8  = (volatile uint8_t* )(Data);
	volatile uint16_t  CheckData;

	uint32_t HW_Size = Size >> 1; // Количество записей в полусловах
	uint32_t Tail = Size & 1; // Если размер данных не чётный, то последний байт пишется по другому

	for (uint32_t i = 0; i < HW_Size; i++)
	{
		FlashData[i] = UserData16[i];

		WaitEOP();
		FLASH->SR = FLASH_SR_EOP;

		WPE = (FLASH->SR & FLASH_SR_WRPRTERR) == FLASH_SR_WRPRTERR;
		PE  = (FLASH->SR & FLASH_SR_PGERR   ) == FLASH_SR_PGERR   ;

		(void)(WPE);
		(void)(PE );

		CheckData = FlashData[i];
		if (CheckData != UserData16[i])
		{
			while (1)
				__NOP();
		}
	}

	if (Tail)
	{
		FlashData[HW_Size] = UserData8[Size-1];

		WaitEOP();
		FLASH->SR = FLASH_SR_EOP;

		WPE = (FLASH->SR & FLASH_SR_WRPRTERR) == FLASH_SR_WRPRTERR;
		PE  = (FLASH->SR & FLASH_SR_PGERR   ) == FLASH_SR_PGERR   ;

		(void)(WPE);
		(void)(PE );

		CheckData = FlashData[HW_Size];
		if ((CheckData & 0xFF) != UserData8[Size-1])
		{
			while (1)
				__NOP();
		}
	}

	FLASH->CR = 0;
	FLASH->SR = FLASH_SR_WRPRTERR | FLASH_SR_PGERR;

	return ERR_OK;
}

#endif

int32_t C_EFLASH::SelfDesroy()
{
	if (CheckLock())
		return ERR_LOCK;

	// Ждём освобождения память
	WaitBusy();
	if (FLASH->SR & FLASH_SR_EOP)
		FLASH->SR = FLASH_SR_EOP;

	// Запускаем стирание всех страниц памяти
	FLASH->CR = FLASH_CR_MER;
	FLASH->CR |= FLASH_CR_STRT;

	// Ждём окончания, но этот код уже стёрт...
	WaitEOP();
	FLASH->SR = FLASH_SR_EOP;
	FLASH->CR = 0;

	return ERR_OK;
}

} // EFLASH

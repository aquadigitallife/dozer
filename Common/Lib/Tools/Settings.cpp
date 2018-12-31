#include "Global.h"

#if !defined(STM32F437xx)
#error "Unknow MCU!"
#endif

#if defined(STM32F437xx)

#endif

#if defined(STM32F437xx)
	const uint8_t SET_SECTOR_INDEX = 4;
	const uint32_t SET_AREA_SIZE  = EFLASH::SECTOR_SIZE[SET_SECTOR_INDEX];
	const uint32_t SET_AREA_START = EFLASH::SECTOR_START[SET_SECTOR_INDEX];
#endif

static EFLASH::C_EFLASH EFlash;

static uint8_t CheckSet(volatile STR_SET* const set)
{
	// Проверяем заголовок
	if (set->Head == ((SET_VER << 16) | FW_Info->DevType))
	{
		// Заголовок корректный. Проверяем целостность
		// TODO Не работает сквозной расчёт до обнуления контрольной суммы
		const uint16_t crc = CRC16_Table((const void*)set, sizeof(STR_SET) - sizeof(set->CheckSumm));
		if (crc == set->CheckSumm)
			return 1;
	}

	return 0;
}

static void MakeSet(STR_SET* const set)
{
	set->Head = ((SET_VER << 16) | FW_Info->DevType);
	set->CheckSumm = 0;
	set->CheckSumm = CRC16_Table(set, sizeof(STR_SET) - sizeof(set->CheckSumm)); // Поле CRC в расчёте не участвует
}

static int32_t FindLastSet()
{
	volatile STR_SET *set = (volatile STR_SET*)SET_AREA_START;
//	STR_SET set2 = {};
//	EFlash.ReadData(SET_AREA_START, &set2, sizeof(STR_SET));

	// Ищем последнюю структуру с настройками
	int32_t SetIndex = -1;
	const uint32_t FindStepCount = SET_AREA_SIZE / sizeof(STR_SET);
	for (uint32_t i = 0; i < FindStepCount; i++)
	{
		if (!CheckSet(set))
		{
			// Останаливаем перебор на первой неудачной структуре
			break;
		}

		SetIndex = i;
		set++;
	}

	return SetIndex;
}

static int32_t ClearSetArea()
{
	// Очищаем все старые сохранения

	int32_t Err = 0;

	Err = EFlash.Unlock();
	if (Err)
		return Err;

#if defined(STM32F10X_MD)
	Err = EFlash.PageErase(SET_AREA_START);
#elif defined(STM32F411xE)
	Err = EFlash.SectorErase(SET_SECTOR_INDEX);
#endif

	EFlash.Lock();

	return Err;
}

static uint32_t GetSetAddr(const uint32_t Index)
{
	uint32_t Offset = sizeof(STR_SET) * Index;
	if (Offset > (SET_AREA_SIZE - sizeof(STR_SET)))
		return 0xFFFFFFFFUL;

	return SET_AREA_START + Offset;
}

int32_t LoadSet(STR_SET* const Set)
{
	int32_t SetIndex = FindLastSet();
	if (SetIndex < 0)
	{
		return 1; // Сохранённых настроек нет или формат изменился
	}

	uint32_t SetAddr = GetSetAddr(SetIndex);
	if (SetAddr < SET_AREA_START || SetAddr >= (SET_AREA_START + SET_AREA_SIZE))
	{
		return 2; // Адрес начала некорректный
	}

	const STR_SET* set = (const STR_SET*)SetAddr;

	memcpy(Set, set, sizeof(STR_SET));
	return 0;
}

int32_t SaveSet(const STR_SET* const Set)
{
	const int32_t SetIndex = FindLastSet();
	uint32_t SetAddr = 0xFFFFFFFFUL;

	if (SetIndex >= 0)
	{
		SetAddr = GetSetAddr(SetIndex+1);
	}

	if (SetAddr < SET_AREA_START || SetAddr >= (SET_AREA_START + SET_AREA_SIZE))
	{
		// Настроек нет или всё заполнено. Очишаем всё
		ClearSetArea();

		SetAddr = GetSetAddr(0);
	}

	STR_SET TmpSet = *Set;
	MakeSet(&TmpSet);

	int32_t Err = EFlash.Unlock();
	if (Err == 0)
	{
		Err = EFlash.WriteData(SetAddr, &TmpSet, sizeof(STR_SET));
		EFlash.Lock();
	}

	return Err;
}

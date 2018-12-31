#define GLOBAL_EXPORT
#include "Global.h"

int main()
{
#ifdef DEBUG
	// В отладке собака не тикает
	DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;

	volatile uint32_t ReserEvent = (RCC->CSR >> 25) & 0x0F;

	if (ReserEvent & (1 << 4))
	{
		volatile int _ = 0;
		_++;
	}

	RCC->CSR |= RCC_CSR_RMVF;
#endif

//#ifndef DEBUG
//	// Если защиты от чтения нет или BOR не установлен или стороженой таймер не аппаратный
//	if (((FLASH->OPTCR & FLASH_OPTCR_RDP_Msk    ) == (0xAAUL << FLASH_OPTCR_RDP_Pos    )) ||
//		((FLASH->OPTCR & FLASH_OPTCR_BOR_LEV_Msk) != (1      << FLASH_OPTCR_BOR_LEV_Pos)) ||
//		((FLASH->OPTCR & FLASH_OPTCR_WDG_SW_Msk ) != (0      << FLASH_OPTCR_WDG_SW_Pos ))
//		)
//	{
//		EFLASH::C_EFLASH EFlash;
//
//		if (EFlash.Unlock_OPT() == EFLASH::ERR_OK)
//		{
//			IWDG_RESET(); // Запись долгая
//
//			// !!! Level 2 !!!
//			//EFlash.RewriteOB(0xCC, 1, 0);
//			EFlash.Lock_OPT();
//		}
//
//		while (1);
//	}
//#endif

	// В настройках линковщика установлено начало ОЗУ на 0x20000004 и размер уменьшен на 4 байта.
	// Так что первые 4 байта в нашем распоряжении, используем их для обмена информацией с загрузчиком о способе загрузки
	BootStatus = (uint32_t*)(0x20000000);
	*BootStatus = 0;

	// Устанавливаем указатели на заголовок прошивки и информацию от загрузчика
	FW_Info = (volatile STR_FW_HEAD*)(MAIN_FW_START - sizeof(STR_FW_HEAD));
	BL_Info = (volatile STR_BL_DATA*)(MAIN_FW_START - sizeof(STR_FW_HEAD) - sizeof(STR_BL_DATA));

	Init();

	vTaskStartScheduler();

	while (1);
}

void GoToBootloader()
{
	*BootStatus = BOOT_TO_LOADER;

	NVIC_SystemReset();
}

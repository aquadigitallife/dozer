
namespace EFLASH
{
#if defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
const uint8_t SECTOR_COUNT = 12;

const uint32_t SECTOR_START[SECTOR_COUNT] = {
	0x08000000, // 0
	0x08004000, // 1
	0x08008000, // 2
	0x0800C000, // 3
	0x08010000, // 4
	0x08020000, // 5
	0x08040000, // 6
	0x08060000, // 7
	0x08080000, // 8
	0x080A0000, // 9
	0x080C0000, // 10
	0x080E0000, // 11
};

const uint32_t SECTOR_SIZE[SECTOR_COUNT] = {
	 16 * 1024, // 0
	 16 * 1024, // 1
	 16 * 1024, // 2
	 16 * 1024, // 3
	 64 * 1024, // 4
	128 * 1024, // 5
	128 * 1024, // 6
	128 * 1024, // 7
	128 * 1024, // 8
	128 * 1024, // 9
	128 * 1024, // 10
	128 * 1024, // 11
};	
	
	
#elif defined(STM32F411xx)
const uint8_t SECTOR_COUNT = 8;

const uint32_t SECTOR_START[SECTOR_COUNT] = {
	0x08000000, // 0
	0x08004000, // 1
	0x08008000, // 2
	0x0800C000, // 3
	0x08010000, // 4
	0x08020000, // 5
	0x08040000, // 6
	0x08060000, // 7
};

const uint32_t SECTOR_SIZE[SECTOR_COUNT] = {
	 16 * 1024, // 0
	 16 * 1024, // 1
	 16 * 1024, // 2
	 16 * 1024, // 3
	 64 * 1024, // 4
	128 * 1024, // 5
	128 * 1024, // 6
	128 * 1024, // 7
};
#elif defined(STM32F10X_MD)

// Актуально для STM32F10X
const uint32_t PAGE_SIZE = 0x0400; // Размер страницы - 1Кбайт

#elif defined(STM32F0XX)

// Актуально для STM32F030
const uint32_t PAGE_SIZE   = 0x0400; // Размер страницы - 1Кбайт
const uint32_t SECTOR_SIZE = PAGE_SIZE * 4;

#endif

const int32_t ERR_OK          = 0;
const int32_t ERR_LOCK        = 1;
const int32_t ERR_CANT_UNLOCK = 2;

class C_EFLASH
{
private:
	uint8_t CheckLock();
	uint8_t CheckLockOPT();

	void WaitBusy();
	void WaitEOP();

public:
	int32_t Unlock();
	void    Lock();

	int32_t Unlock_OPT();
	void    Lock_OPT();

#if defined(STM32F10X_MD) || defined(STM32F0XX)
	int32_t RewriteOB(const OB_TypeDef* const NewOB);
#endif

#if defined(STM32F411xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
	int32_t RewriteOB(const uint8_t RDP = 0xAA, const uint8_t BOR = 3, const uint8_t WDG_SW = 1);
#endif

#if defined(STM32F10X_MD) || defined(STM32F0XX)
	uint32_t GetPageAddr(const uint32_t PageIndex);
#endif

#if defined(STM32F411xx) || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
	int32_t SectorErase(const uint8_t SectorIndex);
#elif defined(STM32F10X_MD) || defined(STM32F0XX)
	int32_t PageErase(const uint32_t Addr);
#endif

	void    ReadData (const uint32_t Addr, const void* Data, const uint32_t Size);
	int32_t WriteData(const uint32_t Addr, const void* Data, const uint32_t Size);

	int32_t SelfDesroy();
};

} // EFLASH

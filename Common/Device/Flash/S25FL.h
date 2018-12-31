
namespace S25FL
{
	// Описание семейства микросхем памяти S25FL

	// Регистры
	// Status Register 1 (SR1)
	const uint8_t REG_SR1_SRWD  = (1 << 7); // Защита регистра от записи
	const uint8_t REG_SR1_P_ERR = (1 << 6); // Ошибка программирования
	const uint8_t REG_SR1_E_ERR = (1 << 5); // Ошибка стирания
	const uint8_t REG_SR1_BP    = (7 << 2); // Диапазон защиты памяти от записи
	const uint8_t REG_SR1_BP2   = (1 << 4); //
	const uint8_t REG_SR1_BP1   = (1 << 3); //
	const uint8_t REG_SR1_BP0   = (1 << 2); //
	const uint8_t REG_SR1_WEL   = (1 << 1); // Разрешение записи
	const uint8_t REG_SR1_WIP   = (1 << 0); // Операция записи в процессе

	// Status Register 2 (SR2)
	const uint8_t REG_SR2_ES    = (1 << 1); // Стирание приостановлено
	const uint8_t REG_SR2_PS    = (1 << 0); // Запись приостановленна

	// Configuration Register 1 (CR1)
	const uint8_t REG_CR1_LC     = (3 << 6); // Код задержки
	const uint8_t REG_CR1_LC1    = (1 << 7); //
	const uint8_t REG_CR1_LC0    = (1 << 6); //
	const uint8_t REG_CR1_TBPROT = (1 << 5); // [OTP] Выбор зоны защиты
	const uint8_t REG_CR1_BPNV   = (1 << 3); // [OTP] Запрет изменения битов защиты
	const uint8_t REG_CR1_QUAD   = (1 << 1); // Включение режима QSPI
	const uint8_t REG_CR1_FREEZE = (1 << 0); // Временный запрет на изменение режимов защиты

	// ECC Status Register (ECCSR)
	const uint8_t REG_ECCSR_EECC  = (1 << 2); // Найдена единичная ошибка
	const uint8_t REG_ECCSR_EECCD = (1 << 1); // Единичкая ошибка скорректирована
	const uint8_t REG_ECCSR_ECCDI = (1 << 0); // ECC активировано и работает

	// Команды
	// Read Device Identification
	const uint8_t CMD_READ_ID    = 0x90; // Read Electronic Manufacturer Signature
	const uint8_t CMD_RDID       = 0x9F; // Read ID (JEDEC Manufacturer ID and JEDEC CFI)
	const uint8_t CMD_RES        = 0xAB; // Read Electronic Signature
	const uint8_t CMD_RSFDP      = 0x5A; // Read Serial Flash Discoverable Parameters
	// Register Access
	const uint8_t CMD_RDSR1      = 0x05; // Read Status Register-1
	const uint8_t CMD_RDSR2      = 0x07; // Read Status Register-2
	const uint8_t CMD_RDCR       = 0x35; // Read Configuration Register-1
	const uint8_t CMD_WRR        = 0x01; // Write Register (Status-1, Configuration-1)
	const uint8_t CMD_WRDI       = 0x04; // Write Disable
	const uint8_t CMD_WREN       = 0x06; // Write Enable
	const uint8_t CMD_CLSR       = 0x30; // Clear Status Register-1 - Erase/Prog. Fail Reset
	const uint8_t CMD_ECCRD      = 0x18; // ECC Read (4-byte address
	const uint8_t CMD_ABRD       = 0x14; // AutoBoot Register Read
	const uint8_t CMD_ABWR       = 0x15; // AutoBoot Register Write
	const uint8_t CMD_BRRD       = 0x16; // Bank Register Read
	const uint8_t CMD_BRWR       = 0x17; // Bank Register Write
	const uint8_t CMD_BRAC       = 0xB9; // Bank Register Access (Legacy Command formerly used for Deep Power Down)
	const uint8_t CMD_DLPRD      = 0x41; // Data Learning Pattern Read
	const uint8_t CMD_PNVDLR     = 0x43; // Program NV Data Learning Register
	const uint8_t CMD_WVDLR      = 0x4A; // Write Volatile Data Learning Register
	// Read Flash Array
	const uint8_t CMD_READ       = 0x03; // Read (3- or 4-byte address)
	const uint8_t CMD_4READ      = 0x13; // Read (4-byte address)
	const uint8_t CMD_FAST_READ  = 0x0B; // Fast Read (3- or 4-byte address)
	const uint8_t CMD_4FAST_READ = 0x0C; // Fast Read (4-byte address)
	const uint8_t CMD_DOR        = 0x3B; // Read Dual Out (3- or 4-byte address)
	const uint8_t CMD_4DOR       = 0x3C; // Read Dual Out (4-byte address)
	const uint8_t CMD_QOR        = 0x6B; // Read Quad Out (3- or 4-byte address)
	const uint8_t CMD_4QOR       = 0x6C; // Read Quad Out (4-byte address)
	const uint8_t CMD_DIOR       = 0xBB; // Dual I/O Read (3- or 4-byte address)
	const uint8_t CMD_4DIOR      = 0xBC; // Dual I/O Read (4-byte address)
	const uint8_t CMD_DDRDIOR    = 0xBD; // DDR Dual I/O Read (3- or 4-byte address)
	const uint8_t CMD_4DDRDIOR   = 0xBE; // DDR Dual I/O Read (4-byte address)
	const uint8_t CMD_QIOR       = 0xEB; // Quad I/O Read (3- or 4-byte address)
	const uint8_t CMD_4QIOR      = 0xEC; // Quad I/O Read (4-byte address)
	const uint8_t CMD_DDRQIOR    = 0xED; // DDR Quad I/O Read (3- or 4-byte address)
	const uint8_t CMD_4DDRQIOR   = 0xEE; // DDR Quad I/O Read (4-byte address)
	// Program Flash Array
	const uint8_t CMD_PP         = 0x02; // Page Program (3- or 4-byte address)
	const uint8_t CMD_4PP        = 0x12; // Page Program (4-byte address)
	const uint8_t CMD_QPP        = 0x32; // Quad Page Program (3- or 4-byte address)
	const uint8_t CMD_QPP_ALT    = 0x38; // Quad Page Program - Alternate instruction (3- or 4-byte address)
	const uint8_t CMD_4QPP       = 0x34; // Quad Page Program (4-byte address)
	const uint8_t CMD_PGSP       = 0x85; // Program Suspend
	const uint8_t CMD_PGRS       = 0x8A; // Program Resume
	// Erase Flash Array
	const uint8_t CMD_BE         = 0x60; // Bulk Erase
	const uint8_t CMD_BE_ALT     = 0xC7; // Bulk Erase (alternate command)
	const uint8_t CMD_SE         = 0xD8; // Erase sector (3- or 4-byte address)
	const uint8_t CMD_4SE        = 0xDC; // Erase sector (4-byte address)
	const uint8_t CMD_ERSP       = 0x75; // Erase Suspend
	const uint8_t CMD_ERRS       = 0x7A; // Erase Resume
	// One Time Program Array
	const uint8_t CMD_OTPP       = 0x42; //OTP Program
	const uint8_t CMD_OTPR       = 0x4B; //OTP Read
	// Advanced Sector Protection
	const uint8_t CMD_DYBRD      = 0xE0; // DYB Read
	const uint8_t CMD_DYBWR      = 0xE1; // DYB Write
	const uint8_t CMD_PPBRD      = 0xE2; // PPB Read
	const uint8_t CMD_PPBP       = 0xE3; // PPB Program
	const uint8_t CMD_PPBE       = 0xE4; // PPB Erase
	const uint8_t CMD_ASPRD      = 0x2B; // ASP Read
	const uint8_t CMD_ASPP       = 0x2F; // ASP Program
	const uint8_t CMD_PLBRD      = 0xA7; // PPB Lock Bit Read
	const uint8_t CMD_PLBWR      = 0xA6; // PPB Lock Bit Write
	const uint8_t CMD_PASSRD     = 0xE7; // Password Read
	const uint8_t CMD_PASSP      = 0xE8; // Password Program
	const uint8_t CMD_PASSU      = 0xE9; // Password Unlock
	// Reset
	const uint8_t CMD_RESET      = 0xF0; // Software Reset
	const uint8_t CMD_MBR        = 0xFF; // Mode Bit Reset
} // namespace S25FL

namespace S25FL512S
{
	using namespace S25FL;

	// Размеры секторов, страниц. Общий объём памяти
	const uint32_t SECTOR_SIZE  = 256*1024;
	const uint32_t PAGE_SIZE    = 512;
	const uint32_t SECTOR_COUNT = 256;
	const uint32_t SIZE         = SECTOR_SIZE * SECTOR_COUNT;
	const uint32_t PAGE_COUNT   = SIZE / PAGE_SIZE;

	// Специфичные команды
	// Read Flash Array
	const uint8_t CMD_DDRFR      = 0x0D; // DDR Fast Read (3- or 4-byte address)
	const uint8_t CMD_4DDRFR     = 0x0E; // DDR Fast Read (4-byte address)

	const uint16_t ASK_READ_ID    = 0x0119; // Ответ на команду CMD_READ_ID (0x90)
}

namespace S25FL127S
{
	using namespace S25FL;

	// Размеры секторов, страниц. Общий объём памяти
	const uint32_t SECTOR_SIZE  = 64*1024;
	const uint32_t PAGE_SIZE    = 512;
	const uint32_t SECTOR_COUNT = 256;
	const uint32_t SIZE         = SECTOR_SIZE * SECTOR_COUNT;
	const uint32_t PAGE_COUNT   = SIZE / PAGE_SIZE;

	// Специфичные биты регистров
	// Configuration Register 1 (CR1)
	const uint8_t REG_CR1_TBPARM = (1 << 2); // [OTP] Расположение блока 4-х килобайтных секторов 0 - в начале (по-умолчанию), 1 - в конце

	// Status Register 2 (SR2)
	const uint8_t REG_SR2_BES    = (1 << 7); // [OTP] Размер блока стирания 256Kb(1) или 64Kb(0), по умолчанию 0
	const uint8_t REG_SR2_PBW    = (1 << 6); // [OTP] Буффер записи 512B(1) или 256B(0), по умолчанию 0
	const uint8_t REG_SR2_IO3R   = (1 << 5); // [OTP] IO3 как RESET(1) или HOLD(0), по умолчанию 0

	// Специфичные команды
	// Erase Flash Array
	const uint8_t CMD_P4E        = 0x20; // Parameter 4-kB, sector Erase (3- or 4-byte address)
	const uint8_t CMD_4P4E       = 0x21; // Parameter 4-kB, sector Erase (4-byte address)

	const uint16_t ASK_READ_ID    = 0x0117; // Ответ на команду CMD_READ_ID (0x90)
}

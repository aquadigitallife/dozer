
namespace W25Q
{
	// Описание семейства микросхем памяти W25Q

	// Регистры
	// Status Register 1 (SR1)
	const uint8_t REG_SR1_SRP0  = (1 << 7); // Защита регистра от записи
	const uint8_t REG_SR1_SEC   = (1 << 6); // Защита сектора
	const uint8_t REG_SR1_TB    = (1 << 5); // Врехняя (0) или нижняя (1) граница защиты
	const uint8_t REG_SR1_BP    = (7 << 2); // Диапазон защиты памяти от записи
	const uint8_t REG_SR1_BP2   = (1 << 4); //
	const uint8_t REG_SR1_BP1   = (1 << 3); //
	const uint8_t REG_SR1_BP0   = (1 << 2); //
	const uint8_t REG_SR1_WEL   = (1 << 1); // Разрешение записи
	const uint8_t REG_SR1_WIP   = (1 << 0); // Операция записи в процессе

	// Status Register 2 (SR2)
	const uint8_t REG_SR2_SUS    = (1 << 7); // Статус приостановки записи/стирания
	const uint8_t REG_SR2_CMP    = (1 << 6); // ???
	const uint8_t REG_SR2_LB     = (7 << 3); // Биты защиты OTP
	const uint8_t REG_SR2_LB3    = (1 << 5); // 
	const uint8_t REG_SR2_LB2    = (1 << 4); // 
	const uint8_t REG_SR2_LB1    = (1 << 3); // 
	const uint8_t REG_SR2_QE     = (1 << 1); // Включение QSPI
	const uint8_t REG_SR2_SRP1   = (1 << 0); // Защита регистра от записи
	
	// Status Register 2 (SR2)
	const uint8_t REG_SR2_H_R    = (1 << 7); // Выбор между HOLD (0) или RESET (1)
	const uint8_t REG_SR2_DRV    = (3 << 5); // ??? Сила выходного драйвера
	const uint8_t REG_SR2_DRV1   = (1 << 6); // 
	const uint8_t REG_SR2_DRV0   = (1 << 5); // 
	const uint8_t REG_SR2_WPS    = (1 << 2); // Выбор защиты от записи

	const uint16_t ASK_READ_ID   = 0xEF17; // Ответ на команду CMD_READ_ID (0x90)

	// Команды
	// Read Device Identification
	const uint8_t CMD_READ_ID    = 0x90; // Read Electronic Manufacturer Signature
	const uint8_t CMD_RDID       = 0x9F; // Read ID (JEDEC Manufacturer ID and JEDEC CFI)
	const uint8_t CMD_RES        = 0xAB; // Read Electronic Signature
	const uint8_t CMD_RSFDP      = 0x5A; // Read Serial Flash Discoverable Parameters
	// Register Access
	const uint8_t CMD_RDSR1      = 0x05; // Read Status Register-1
	const uint8_t CMD_RDSR2      = 0x35; // Read Status Register-2
	const uint8_t CMD_RDSR3      = 0x15; // Read Status Register-3
	const uint8_t CMD_WRSR1      = 0x01; // Write Status Register-1
	const uint8_t CMD_WRSR2      = 0x31; // Write Status Register-2
	const uint8_t CMD_WRSR3      = 0x11; // Write Status Register-3
	const uint8_t CMD_WRDI       = 0x04; // Write Disable
	const uint8_t CMD_WREN       = 0x06; // Write Enable
	// Read Flash Array
	const uint8_t CMD_READ       = 0x03; // Read
	const uint8_t CMD_FAST_READ  = 0x0B; // Fast Read
	const uint8_t CMD_DOR        = 0x3B; // Read Dual Out
	const uint8_t CMD_QOR        = 0x6B; // Read Quad Out
	// Program Flash Array
	const uint8_t CMD_PP         = 0x02; // Page Program
	const uint8_t CMD_QPP        = 0x32; // Quad Page Program
	// Erase Flash Array
	const uint8_t CMD_P4E        = 0x20; // Parameter 4-kB, sector Erase
	const uint8_t CMD_BE         = 0x60; // Bulk Erase
	const uint8_t CMD_BE_ALT     = 0xC7; // Bulk Erase (alternate command)
	const uint8_t CMD_SE         = 0xD8; // Erase sector
} // namespace W25Q

namespace W25Q128FV
{
	using namespace W25Q;

	// Размеры секторов, страниц. Общий объём памяти
	const uint32_t SECTOR_SIZE  = 64*1024;
	const uint32_t PAGE_SIZE    = 256;
	const uint32_t SECTOR_COUNT = 256;
	const uint32_t SIZE         = SECTOR_SIZE * SECTOR_COUNT;
	const uint32_t PAGE_COUNT   = SIZE / PAGE_SIZE;
}

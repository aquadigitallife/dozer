#include "Global.h"

//////////////////////////////////////////////////////////////////////////
// CRC16 /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#ifdef USE_CRC16
// Полином 0x1021
uint16_t CRC16(const void *Data, const size_t Size)
{
	uint16_t crc = 0;

	uint8_t *dat = (uint8_t*)(Data);
	size_t n = Size;

	while (n--)
	{
		crc ^= *dat++ << 8;

		for (uint8_t i = 0; i < 8; i++)
			crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
	}

	return crc;
}
#endif

#ifdef USE_CRC16_TABLE
uint16_t CRC16_Table(const void *Data, const size_t Size)
{
	uint16_t crc = 0;

	uint8_t *dat = (uint8_t*)(Data);
	size_t n = Size;

	while (n--)
		crc = CRC16_TABLE[((crc >> 8) ^ *dat++) & 0xFF] ^ (crc << 8);

	return crc;
}
#endif

#if 0
void CRC16_MakeTable()
{
	uint16_t crc_table[256] = {};

	for (uint16_t i = 0; i < 256; i++)
	{
		crc_table[i] = i << 8;

		for (uint8_t j = 0; j < 8; j++)
			crc_table[i] = (crc_table[i] & 0x8000) ? ((crc_table[i] << 1) ^ 0x1021) : (crc_table[i] << 1);
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
// CRC32 /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#ifdef USE_CRC32
uint32_t CRC32(const void *Data, size_t Size)
{
	return 0;
}
#endif

#ifdef USE_CRC32_TABLE
uint32_t CRC32_Table(const void *Data, const size_t Size)
{
	uint32_t crc = 0;

	uint8_t *dat = (uint8_t*)(Data);
	size_t n = Size;

	while (n--)
		crc = CRC32Table[(crc ^ *dat++) & 0xFF] ^ (crc >> 8);

	return crc;
}
#endif

#if 0
void CRC32_MakeTable()
{
	uint32_t crc_table[256] = {};

	// Рассчёт таблицы
	for (uint16_t i = 0; i < 256; i++)
	{
		crc_table[i] = i;

		for (uint8_t j = 0; j < 8; j++)
			crc_table[i] = (crc_table[i] & 1) ? ((crc_table[i] >> 1) ^ 0xEDB88320UL) : (crc_table[i] >> 1);
	}
}
#endif

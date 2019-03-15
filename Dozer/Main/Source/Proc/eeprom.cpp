/*
	Модуль для работы с EEPROM
*/

#include "Global.h"

#define MAX_EEPROM_ADDR	0x1FFF	// Максимальный адрес EEPROM
#define PAGE_SIZE	0x20		// размер страницы памяти EEPROM в байтах

/*
	Функция записи в EEPROM в первую страницу.
	Контроль страниц не отлаживался, поэтому закоментирован
	addr - адрес EEPROM начала записи массива байт data
	len - размер массива байт data
	data - указатель на массив байт для записи в EEPROM
*/
BaseType_t ee_write(uint16_t addr, uint16_t len, const void *data)
{

	uint16_t plen = len;
	uint8_t *pdata = (uint8_t*)data;
	uint16_t eaddr = addr + len;
	uint16_t paddr = addr;
	uint16_t pages = (eaddr >> 5) - (paddr >> 5);
	
	if (eaddr > MAX_EEPROM_ADDR) return pdFAIL;
	while (pages--) {
		uint16_t slen = ((paddr + PAGE_SIZE) & 0xFFE0) - paddr;
		plen = plen - slen;
		i2c(0xA0, paddr, slen, pdata);
		paddr += slen; pdata += slen;
	}
	if (plen) i2c<uint16_t>(0xA0, paddr, plen, pdata);

//	return i2c<uint16_t>(0xA0, addr, len, data);
	return pdPASS;
}


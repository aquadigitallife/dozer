
union u16
{
	uint16_t Val;
	uint8_t Raw[2];
};

union u32
{
	uint32_t Val;
	uint8_t Raw[4];
};

// Для прошивок загрузчика и основной
#pragma pack (push, 1)

struct STR_BL_DATA
{
	// Информация о загрузчике
	uint32_t Ver;         // Версия загрузчика
	uint16_t DevType;     // Тип устройства
	uint8_t Reserved[10]; // Зарезервированно
};

struct STR_FW_HEAD
{
	// Заголовок прошивки и информация о ней

	uint32_t Gamma;       // Гамма для создания непохожего обража прошивки
	uint32_t CheckSumm;   // CRC32
	uint32_t Ver;         // Версия прошивки
	uint16_t DevType;     // Тип устройства
	uint8_t Reserved[ 2]; // Зарезервированно
};

#pragma pack (pop)

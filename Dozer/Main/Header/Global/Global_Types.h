
///////////////////////////////////////////////////////////////////////////////
// Обмен данными между задач

struct STR_FLASH_CMD
{
	FLASH_CMD Cmd;
	uint32_t  Arg;

	// Откуда брать или куда класть данные
	void* DataPtr;           // У указателя приоритет
	QueueHandle_t DataQueue;
	uint32_t Size;           // Размер данных

	QueueHandle_t ResQueue; // Очередь с результатом окончания операции
};

struct STR_WDG_CMD
{
	uint8_t  TaskID;    // От какой задачи пришло событие
	uint32_t TaskTime;  // Через сколько выполнить сброс
	uint32_t TaskParam; // Дополнительный параметр для отладки
};

///////////////////////////////////////////////////////////////////////////////
//Кнопки

struct STR_BUT
{
	BUT_STATE State;
	uint16_t PressTime;   // Время задержки нажатия
	uint16_t ReleaseTime; // Время задержки отпускания
	uint16_t HoldTime;    // Время удерживания в нажатом положении
};

struct STR_BUTTONS
{
	STR_BUT Up;
	STR_BUT Play;
	STR_BUT Down;
};

///////////////////////////////////////////////////////////////////////////////
// Состояние системы питания
struct STR_POWER_PARAM
{
	// Наблюдение за питанием
//	STR_BAT_INFO BatInfo;
//	// Аккумуляторы для работы фильтра
//	uint32_t FltAccBat;
//	uint32_t FltOutBat;
//	uint32_t FltAccMCU;
//	uint32_t FltOutMCU;
//	uint32_t FltAccLev;
//	uint32_t FltOutLev;
};

///////////////////////////////////////////////////////////////////////////////
// Текущая конфигурация устройства
struct STR_CONFIG
{

};

// Сохранённые настройки
const uint16_t SET_VER = 0x0001; // Версия должна увеличиваться при каждом изменении структуры

#pragma pack (push, 1)
struct STR_SET // Размер структуры должен быть кратен 4, т.к. были замечены проблемы при записи/чтении
{
	uint32_t Head;              // Заголовок
	uint8_t Reserved[2];
	uint16_t CheckSumm;         // CRC-16
};
#pragma pack (pop)

///////////////////////////////////////////////////////////////////////////////

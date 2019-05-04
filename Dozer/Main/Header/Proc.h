
void LED_TaskProc(void *Param);		// процесс мигания светодиодом "status"
void FlashProc(void *Param);		// процесс работы с CompactFlash

// Для сторожевой собаки
void WDG_TaskProc(void *Param);
void ResetWatchdog(uint32_t &LastResetTime, const uint8_t TaskID, const uint32_t Time, const uint8_t Param, const bool Force = false);

/*-----------------------Заслонка, крыльчатка-------------------*/
void Motor0Proc(void *Param);	// ручное управление заслонкой
void Motor0Cycle(void *Param);	// управление заслонкой во время рассеивания
void Motor1Proc(void *Param);	// управление крыльчаткой во время рассеяния
/*-------------------------Опрос кнопок-------------------------*/
void ButtonsProc(void *Param);
/*-----------------------------BLEUart------------------------------*/
StreamBufferHandle_t InitBLEUart(size_t xBufferSizeBytes);	// инициализация USART для связи с BLE
void BLEUartTx(uint32_t len, uint8_t *data);				// функция передачи данных в BLE
int32_t BLEUartRx(uint32_t len, uint8_t *data);				// функция приёма данных из BLE
int32_t BLEUartPeek(void);									// функция проверки наличия сообщений от BLE
/*------------------------------BLE------------------------------*/
extern SemaphoreHandle_t ble_write_lock;
bool is_ble_ready(void);
void BLEProc(void *Param);	
/*------------------------------I2C-----------------------------*/
void Init_I2C(void);	// инициализация контроллера I2C

template <typename T>	// функции приёма/передачи по шине I2C. T может быть либо uint8_t (для RTC) либо uint16_t (для EEPROM)
BaseType_t i2c(uint8_t dev, T addr, uint32_t len, const void* data);	// если LSB dev == 1 то чтение если 0 - запись
/*-----------------------------RTC-------------------------------*/
/* Структура даты и времени в модуле BLE */
struct date_time
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} __packed;

struct dozer_action
{
	uint8_t	actual;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	double	doze;
};

extern struct dozer_action next_action;

void ble_update_rtc(const struct date_time *arg);	// функция установки RTC со смартфона
void get_sys_date(struct date_time *dest);
int get_minutes(uint8_t hours, uint8_t minutes);
void add_minutes(struct dozer_action *action, int mins);
bool is_action_trigged(struct dozer_action *action, struct date_time *now);
int action_compare(struct dozer_action *act1, struct dozer_action *act2);
uint8_t day_inc(uint8_t day);
void RTCProc(void *Param);	// процесс выдачи сообщений о текущем времени каждую секунду
/*------------------------------SPI------------------------------*/
void start_SPI(void);				// старт SPI-транзакции
void set_spi_handling_task(void);	// функция назначения текущей задачи контроллеру SPI
void clear_spi_handling_task(void);	// функция освобождения контроллера SPI от задачи
void Init_SPI(void);				// инициализация контроллера SPI
/*-----------------------------AD7799----------------------------*/
void AD7799Proc(void *Param);		// основной фильтр показаний АЦП тензодатчика
int32_t set_ad7799_zero(void);		// обнуление начального смещения и запись его в EEPROM
double get_weight(void);			// функция чтения текущего веса
double get_doze(void);				// функция чтения заданной дозы

#ifdef __cplusplus
void set_doze(int32_t val);			// установка дозы через BLE
void set_doze(double val);
#endif

/*-----------------------------EEPROM----------------------------*/
#define MAX_EEPROM_ADDR	0x1FFF	// Максимальный адрес EEPROM
#define EEPROM_SIZE	0x2000
#define TENSO_OFFSET_ADDR	0x0000	// адрес хранения начального смещения АЦП
#define TANK_NUM_ADDR	0x0008		// 51 байт
#define TANK_NUM_MAX_LEN	50
#define INN_ADDR		0x003B		// 13 байт
#define INN_MAX_LEN			12
#define PSW_ADDR		0x0048
#define PSW_MAX_LEN			50

BaseType_t ee_write(uint16_t addr, uint16_t len, const void *data);		// функция записи в EEPROM
portINLINE BaseType_t ee_read(uint16_t addr, uint16_t len, const void *data)	// функция чтения из EEPROM
{
	return i2c<uint16_t>(0xA1, addr, len, data);
}
/*-----------------------------RTUUart------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

void RTUUartTx(size_t len, void *data);						// передача данных по UART RTU
StreamBufferHandle_t InitRTUUart(size_t xBufferSizeBytes);	// инициализация UART RTU
size_t RTUUartRx(size_t len, void *data);					// приём данных по UART RTU
size_t RTUUartPeek(void);									// Проверка наличия сообщений от RTU

#ifdef __cplusplus
};
#endif

/*-----------------------------GSMUart------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

void GSMUartTx(size_t len, void *data);						// передача данных по UART GSM
StreamBufferHandle_t InitGSMUart(size_t xBufferSizeBytes);	// инициализация UART GSM
size_t GSMUartRx(size_t len, void *data);					// приём данных по UART GSM
size_t GSMUartPeek(void);									// Проверка наличия сообщений от GSM
void GSMUartClean(void);
#ifdef __cplusplus
};
#endif

/*-----------------------------GSM----------------------------------*/
#define UNDEFINED_MODE	0
#define MANUAL_MODE		1
#define INTERVAL_MODE	2
#define SHEDULE_MODE	3
void tank_change(void);
void httpsProc(void *Param);
/*----------------------------Motors--------------------------------*/
extern uint8_t motor1_on, purge_on;
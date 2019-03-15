/*
	Модуль работы с AD7799
*/

#include "Global.h"
#include "spi.h"
#include "SampleFilter.h"
/*Константы команд коммуникационного регистра */
#define CR_WEN			0x00				// запись в комм. регистр разрешена
#define CR_WDIS			0x80				// запись в комм. регистр запрещена
#define CR_RD			0x40				// следующая операция будет чтение регистра
#define CR_WR			0x00				// следующая операция будет запись регистра
#define CR_ADDR_MASK	((uint8_t)0x38)		// маска адреса регистра
#define CR_ADDR_CR		((uint8_t)0x00)		// адрес CONTROL reg.
#define CR_ADDR_SR		((uint8_t)0x00)		// адрес STATUS reg.
#define CR_ADDR_MR		((uint8_t)(1 << 3))	// адрес MODE reg.
#define CR_ADDR_CON		((uint8_t)(2 << 3))	// адрес CONFIG reg.
#define CR_ADDR_DR		((uint8_t)(3 << 3))	// адрес DATA reg.
#define CR_ADDR_ID		((uint8_t)(4 << 3))	// адрес ID устройства
#define CR_ADDR_IO		((uint8_t)(5 << 3))	// адрес управления портами ввода-вывода
#define CR_ADDR_OFFSET	((uint8_t)(6 << 3))	// адрес регистра текущего смещения
#define CR_ADDR_SCALE	((uint8_t)(7 << 3))	// адрес регистра текущей крутизны
#define CR_CREAD		0x04				// признак непрерывного чтения
/* Константы команд регистра MODE */
#define MR_MODE_CONT	0x00	// непрерывное чтение
#define MR_MODE_SINGLE	0x20	// однократное чтение
#define MR_MODE_IDLE	0x40	// режим ожидания
#define MR_MODE_PD		0x60	// режим малого энергопотребления
#define MR_MODE_IZERO	0x80	// режим внутреннего сброса нуля
#define MR_MODE_IFSCALE	0xA0	// режим внутреннего масштабирования
#define MR_MODE_SZERO	0xC0	// режим системного сброса нуля
#define MR_MODE_SFSCALE	0xE0	// режим системного масштабирования
#define MR_PSW_ON		0x10	// встроенный ключ включить
#define MR_PSW_OFF		0x00	// встроенный ключ выключить
#define MR_RATE_470HZ	0x01	// это и далее - частоты преобразования
#define MR_RATE_242HZ	0x02
#define MR_RATE_123HZ	0x03
#define MR_RATE_62HZ	0x04
#define MR_RATE_50HZ	0x05
#define MR_RATE_39HZ	0x06
#define MR_RATE_33HZ2	0x07
#define MR_RATE_19HZ6	0x08
#define MR_RATE_16HZ7	0x09
#define MR_RATE_12HZ5	0x0B
#define MR_RATE_10HZ	0x0C
#define MR_RATE_8HZ33	0x0D
#define MR_RATE_6HZ25	0x0E
#define MR_RATE_4HZ17	0x0F
/*
	Функции чтения регистров AD7799
	T может принимать значения uint8_t или int32_t (для 24-битных значений)
	аргумент addr - адрес регистра для чтения, одна из констант CR_ADDR_XX
*/
template <typename T>
T ad7799_rd(uint8_t addr)
{
	uint32_t ulNotifiedValue;	// переменная для функции xTaskNotifyWait
	
	static const T null_val = 0;	// значение, передаваемое по SPI при чтении (д.б. всегда ноль)
	T retval = 0;					// возвращаемое значение
	T pretval = 0;					// считанное из АЦП значение
	
	uint8_t cr = CR_WEN | CR_RD | addr;		// формируем команду чтения для записи в коммуникационный регистр
	
	AD7799_CS_SELECT;				// опускаем чип-селект
	set_spi_handling_task();		// привязываем текущую задачу к обработчику прерываний SPI
	while (!IS_AD7799_CS_SELECT);	// ожидаем нулевое значение на чип-селект

	// (1) Настраиваем DMA на передачу команды *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&cr);	
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(cr));
	// Настраиваем DMA на приём dummy байта
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&pretval);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(cr));

	// (2) Стартуем обмен по SPI **************************************************
	start_SPI();
	
	// Ожидаем окончания передачи (сигнал от обработчика прерывания).
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );
	
	// (1) Настраиваем DMA на передачу нулей *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&null_val);
	if (sizeof(T) > 2)	// если T == int32_t, принимаем 3 байта
		LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, 3);
	else	// иначе кол-во байт равно размеру типа (2 или 1)
		LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(T));
	// приём байт в переменную pretval
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&pretval);
	if (sizeof(T) > 2)	// кол-во байт на приём аналогично передаче
		LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, 3);
	else
		LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(T));
	// (2) Стартуем обмен по SPI **************************************************
	
	start_SPI();
	// Ожидаем окончания приёма (сигнал от обработчика прерывания).
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );
	// поднимаем чип-селект
	AD7799_CS_DESELECT;
	clear_spi_handling_task();	// освобождаем контроллер SPI от задачи
	// меняем порядок следования байт на младшего индейца
	if (sizeof(T) > 2) {
		((uint8_t*)&retval)[0] = ((uint8_t*)&pretval)[2];
		((uint8_t*)&retval)[1] = ((uint8_t*)&pretval)[1];
		((uint8_t*)&retval)[2] = ((uint8_t*)&pretval)[0];
	}
	if (sizeof(T) == 2) {
		((uint8_t*)&retval)[0] = ((uint8_t*)&pretval)[1];
		((uint8_t*)&retval)[1] = ((uint8_t*)&pretval)[0];
	}
	return retval;
}

/*
	Функция записи регистров АЦП
	аргумент addr - адрес регистра для записи, одна из констант CR_ADDR_XX
	аргумент value - записываемое значение регистра
*/
void ad7799_wr(uint8_t addr, uint8_t value)
{
	uint32_t ulNotifiedValue;	// переменная для функции xTaskNotifyWait
	
	uint8_t tmp;	// переменная для приёма незначащего байта во время передачи
	
	const uint8_t pval = value;					// сохраняем записываемое значение в лок. переменной
	const uint8_t cr = CR_WEN | CR_WR | addr;	// формируем команду на запись
	
	AD7799_CS_SELECT;					// опускаем чип-селект
	set_spi_handling_task();			// связываем контроллер SPI с текущей задачей
	while (!IS_AD7799_CS_SELECT);		// ожидаем пока опустится чип-селект

	// (1) Настраиваем DMA на передачу команды в коммуникационный регистр *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&cr);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(cr));
	// Настраиваем DMA на приём незначащего байта во время передачи
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&tmp);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(tmp));
	// (2) Стартуем передачу по SPI **************************************************
	start_SPI();
	
	// Ожидаем окончания передачи.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );
	
	// (1) Настраиваем DMA на передачу записываемого значения в регистр *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&pval);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(pval));
	// Настраиваем DMA на приём незначащего байта во время передачи
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&tmp);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(tmp));
	// (2) Стартуем передачу по SPI **************************************************
	start_SPI();
	
	// Ожидаем окончания передачи.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );

	AD7799_CS_DESELECT;	// поднимаем чип-селект
	clear_spi_handling_task();	// освобождаем контроллер от задачи
}

static double flt113 = 0.0;		// Выходное значение основного фильтра
static double flt10 = 0.0;		// Достаточно быстрый фильтр 1-го порядка.
								// Служит для сглаживания шумов младших разрядов АЦП и ограничения спектра для основного
								// фильтра с частотой дискретизации 2 Гц.
static double flt0 = 0.0;		// Переменная для хранения начального смещения АЦП.

double doze = 0.0;				// Доза. Задаётся через BLE
static double weight = 0.0; 	// Остаток корма в бункере
/*
	Процесс предварительной фильтрации значений АЦП
*/
void AD7799Flt(void *Param)
{
	uint8_t tb = 0xFF;
	double val;
	
	// Считываем начальное смещение АЦП из EEPROM
	ee_read((uint16_t)TENSO_OFFSET_ADDR, sizeof(double), &flt0);
	
	// проверяем на корректность значения (потом переделать на проверку КС)
	for (unsigned int i = 0; i < sizeof(double); i++) tb &= ((uint8_t*)&flt0)[i];
	if (tb == 0xFF) flt0 = 0.0;	// если память "чистая" устанавливаем нач. смещение АЦП = 0
	
	val = (double)ad7799_rd<uint32_t>(CR_ADDR_DR);		// читаем первое значение АЦП
	while (1) {
		flt10 += (val - flt10)/10.0;					// реализация предварительного фильтра
		while (!IS_ADC_DATA_RDY) taskYIELD();			// ожидаем очередное значение
		val = (double)ad7799_rd<uint32_t>(CR_ADDR_DR);	// читаем очередное значение
	}
}

static SampleFilter	flt;	// структура основного фильтра (коэффициенты и прочее)
/*
	Процесс основного фильтра и инициализации АЦП.
*/
void AD7799Proc(void *Param)
{
	TaskHandle_t Flt_TaskHandle;	// дескриптор процесса предварительного фильтра
	// Настраиваем АЦП на непрерывное чтение с частотой преобразования 16,7 Гц
	ad7799_wr(CR_ADDR_MR, (MR_MODE_CONT | MR_PSW_OFF | MR_RATE_16HZ7));
	// Создаём процесс предварительной фильтрации
	xTaskCreate(AD7799Flt, "", configMINIMAL_STACK_SIZE, 0, TASK_PRI_LED, &Flt_TaskHandle);
	// инициализируем основной фильтр
	SampleFilter_init(&flt);

	while (1) {
		static uint8_t flag = 0;	// делитель частоты обновления фильтра на 2 (для выдачи значений веса в BLE) 
		int32_t iweight;	// вес в формате integer
		SampleFilter_put(&flt, flt10);	// подаём на вход фильтра выход предварительного фильтра
		
		flt113 = SampleFilter_get(&flt);	// считываем выходное значение фильтра
		weight = (flt113 - flt0)/51.02466;	// вычисляем вес: вычитаем смещение, делим на крутизну
		iweight = (int32_t)weight;			// вес в формате integer для передачи в BLE
		LED_ERR_BLINK;						// сигнализация для отладки
		flag ^= 0xFF;	// делитель частоты на 2
		// С частотой в 2 раза меньшей частоты обновления фильтра, обновляем значение веса в БД BLE
		if (flag == 0) if (RTC_Queue != NULL) xQueueSendToBack(AD7799_Queue, &iweight, 0);
		// Частота обновления фильтра 2Гц
		vTaskDelay(MS_TO_TICK(500));
	}
}

/*
	Функция обновления и записи начального смещения.
	Вызывается по команде от BLE
*/
int32_t set_ad7799_zero(void)
{
	flt0 = flt113;	// принимаем за начальное смещение текущее значение фильтра
	ee_write((uint16_t)TENSO_OFFSET_ADDR, sizeof(double), &flt0);	// записываем значение в EEPROM
	return (int32_t)flt0;	// возвращаем записанное значение
}
/*
	Функция возвращает текущее значение веса корма в кормушке
	Предназначена для вызова из других модулей
*/
double get_weight(void)
{
	return weight;
}

/*
	Функция установки запрашиваемой дозы
	Вызывается по команде из BLE
*/
void set_doze(int32_t val)
{
	doze = ((double)val);	
}

/*
	Функция чтения установленной дозы.
	Предназначена для вызова из других модулей
*/
double get_doze(void)
{
	return doze;
}







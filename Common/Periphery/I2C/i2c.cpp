/*
	Модуль содержит функции для работы с контроллером I2C
*/

#include "Global.h"

/* Параметры сигнала I2C: частота 50 кГц, скважность 2 */
#define I2C_SPEEDCLOCK		50000
#define I2C_DUTYCYCLE		LL_I2C_DUTYCYCLE_2
/* Признаки завершения передачи по DMA*/
#define I2C_COMPLETE		0x00000001UL	// передача завершена без ошибок
#define I2C_ERROR			0x00000002UL	// с щибками

/**
  * @brief Константы направления передачи (запись/чтение)
  */
#define I2C_REQUEST_WRITE	0x00
#define I2C_REQUEST_READ	0x01

/* Обработчики прерываний */
#ifdef __cplusplus
extern "C" {
#endif
void I2C3_EV_IRQHandler(void) __attribute__((interrupt));		// прерывание при событиях на шине I2C
void DMA1_Stream4_IRQHandler(void) __attribute__((interrupt));	// прерываение по окончании DMA записи по I2C
void DMA1_Stream2_IRQHandler(void) __attribute__((interrupt));	// прерывание по окончании DMA чтения по I2C
void I2C3_ER_IRQHandler(void) __attribute__((interrupt));		// прерывание при ошибке на шине I2C

#ifdef __cplusplus
};
#endif

SemaphoreHandle_t i2c_lock;						// мьютекс для защиты одновременного использования I2C разными задачами

__IO uint8_t  ubMasterRequestDirection  = 0;	// переменная для хранения бита приём/передача
__IO uint8_t  ubMasterNbDataToReceive   = 1;	// длина передаваемого/принимаемого сообщения

uint8_t device_addr;							// "чистый" адрес устройства (без бита приём/передача)
uint8_t ubDirection = 0;						// запрошенный тип транзакции (запись/чтение)
const void *pdata;								// указатель на принимаемые/передаваемые данные

TaskHandle_t xHandlingTask;						// дескриптор привязанной к контроллеру задачи


/*
	Функция обработчика прерывания по событию на шине i2c
*/
void I2C3_EV_IRQHandler(void)
{
  /* Событие - "старт-бит передан" */
  if(LL_I2C_IsActiveFlag_SB(I2C3))
  {
    /* передаём адрес устройства */
    LL_I2C_TransmitData8(I2C3, device_addr | ubMasterRequestDirection);
  }
  /* Событие - адрес устройства передан */
  else if(LL_I2C_IsActiveFlag_ADDR(I2C3))
  {
	  if (LL_I2C_GetTransferDirection(I2C3) == LL_I2C_DIRECTION_WRITE) {	// если адрес был передан с битом "запись"
    /* */
		LL_I2C_EnableDMAReq_TX(I2C3);										// Стартуем DMA передачу по I2C 
	  } else {																// если адрес был передан с битом "чтение"
		  if(ubMasterNbDataToReceive == 1) {								// если нужно прочитать только 1 байт

			LL_I2C_AcknowledgeNextData(I2C3, LL_I2C_NACK);					// по завершению приёма байта выдать на линию NACK
		  }
		  else if(ubMasterNbDataToReceive == 2) {							// если нужно принять 2 байта

			LL_I2C_AcknowledgeNextData(I2C3, LL_I2C_NACK);					// заряжаем ответ NACK

			LL_I2C_EnableBitPOS(I2C3);										// выставляем Pos
		  } else {															// если нужно принять больше 2-х байт
			LL_I2C_EnableLastDMA(I2C3);										// выдадим NACK по последней DMA-транзакции
		  }
		  LL_I2C_EnableDMAReq_RX(I2C3);										// запускаем DMA-чтение
	  }
 	  LL_I2C_ClearFlag_ADDR(I2C3);											// сбрасываем признак прерывания по передаче адреса устройства
  }
  else if (LL_I2C_IsActiveFlag_BTF(I2C3)) {									// если последний байт передан
	  if (ubDirection == I2C_REQUEST_READ) {								// если запрошено чтение информации
		LL_I2C_GenerateStartCondition(I2C3);								// значит передавали адрес начала массива на чтение, выставляем repeated start
	  } else {																// иначе запрошена запись информации и запись завершена
		LL_I2C_GenerateStopCondition(I2C3);									// выставляем stop condition
		if (xHandlingTask != NULL) {										// если к контроллеру привязана задача
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			xTaskNotifyFromISR( xHandlingTask, I2C_COMPLETE, eSetBits, &xHigherPriorityTaskWoken );	// сигнализируем ей о завершении процедуры
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
	  }
  }
}


/*
	Функция обработчика прерывания при ошибке на линии I2C
*/

void I2C3_ER_IRQHandler(void)
{
	// Сбрасываем все флаги ошибок
	LL_I2C_ClearFlag_BERR(I2C3);
	LL_I2C_ClearFlag_AF(I2C3);
	LL_I2C_ClearFlag_ARLO(I2C3);
	
	// Останавливаем все связанные с I2C DMA
	
	LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_4);
	LL_DMA_ClearFlag_HT4(DMA1);
    LL_DMA_ClearFlag_TC4(DMA1);
	
	LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_2);
	LL_DMA_ClearFlag_HT2(DMA1);
    LL_DMA_ClearFlag_TC2(DMA1);
	// Выставляем стоп-условие
	LL_I2C_GenerateStopCondition(I2C3);
	// Если к I2C привязана задача, сигнализируем ей об окончании процедуры
	if (xHandlingTask != NULL) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR( xHandlingTask, I2C_ERROR, eSetBits, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

/*
	Прерывание после окончания DMA-записи
*/
void DMA1_Stream4_IRQHandler(void)
{

  if(LL_DMA_IsActiveFlag_TC4(DMA1))												// транзакция DMA завершена
  {
    LL_DMA_ClearFlag_TC4(DMA1);													// сбрасываем признак завершения

	if (ubDirection == I2C_REQUEST_READ) {										// если запрошено чтение информации
		LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_2, (uint32_t)pdata);		// настраиваем "читающий" поток DMA
		LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_2, ubMasterNbDataToReceive);
		LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);
		ubMasterRequestDirection = I2C_REQUEST_READ;
	} else {																	// если запрошена запись информации
		if (ubMasterNbDataToReceive) {											// если первое завершение записи (передан адрес)
		LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_4, (uint32_t)pdata);		// перенастраиваем "пишущий" поток DMA
		LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_4, ubMasterNbDataToReceive);
		LL_I2C_EnableDMAReq_TX(I2C3);
		LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_4);
		ubMasterNbDataToReceive = 0;											// сбрасываем для предотвращения повторного выполнения данного кода
		}
	}
  }
}

/*
	Прерывание после окончания DMA-чтения
*/
void DMA1_Stream2_IRQHandler(void)
{
  if(LL_DMA_IsActiveFlag_TC2(DMA1))							// транзакция DMA завершена
  {
    LL_DMA_ClearFlag_TC2(DMA1);								// сбрасываем признак завершения
	LL_I2C_GenerateStopCondition(I2C3);						// выставляем стоп-условие
	if (xHandlingTask != NULL) {							// если к I2C привязана задача
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR( xHandlingTask, I2C_COMPLETE, eSetBits, &xHigherPriorityTaskWoken );	// оповещаем её о завершении процедуры
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
  }
}

/*
	Процедура инициализации DMA-контроллера для I2C
*/
static void Configure_DMA(void)
{
	/* (2) Настраиваем прерывания от передающих и принимающих потоков DMA */
	NVIC_SetPriority(DMA1_Stream4_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);
	NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  
	NVIC_SetPriority(DMA1_Stream2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+4);
	NVIC_EnableIRQ(DMA1_Stream2_IRQn);

	/* (3) Конфигурируем передающий поток DMA */

	LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_4, LL_DMA_CHANNEL_3);	// включаем поток 4 канал 3

	LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_4, LL_DMA_DIRECTION_MEMORY_TO_PERIPH | \		// направление потока: память -> периферия
												LL_DMA_PRIORITY_HIGH               | \		// высокий приоритет
												LL_DMA_MODE_NORMAL                 | \		// режим NORMAL
												LL_DMA_PERIPH_NOINCREMENT          | \		// Адрес периферии не инкрементируется
												LL_DMA_MEMORY_INCREMENT            | \		// Адрес в памяти инкрементируется
												LL_DMA_PDATAALIGN_BYTE             | \		// Выравнивание на стороне периферии - побайтно
												LL_DMA_MDATAALIGN_BYTE);					// Выравнивание на стороне памяти - побайтно
	LL_DMA_ConfigAddresses(	DMA1,															// Устанавливаем указатели адресов DMA:
							LL_DMA_STREAM_4,
							(uint32_t)NULL,													// Указатель в памяти будет установлен позже
							(uint32_t)LL_I2C_DMA_GetRegAddr(I2C3),							// Указатель в периферии I2C3->DR
							LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_4));


	/* (4) Конфигурируем принимающий поток DMA */
	LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_2, LL_DMA_CHANNEL_3);					// Включаем поток 2 канал 3

	LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_2, LL_DMA_DIRECTION_PERIPH_TO_MEMORY  | \		// направление потока: периферия -> память
												LL_DMA_PRIORITY_HIGH				| \		// высокий приоритет
												LL_DMA_MODE_NORMAL					| \		// режим NORMAL
												LL_DMA_PERIPH_NOINCREMENT			| \		// Адрес периферии не инкрементируется
												LL_DMA_MEMORY_INCREMENT				| \		// Адрес памяти инкрементируется
												LL_DMA_PDATAALIGN_BYTE				| \		// Выравнивание на стороне периферии - побайтно
												LL_DMA_MDATAALIGN_BYTE);					// Выравнивание на стороне памяти - побайтно

	LL_DMA_ConfigAddresses(	DMA1,															// Устанавливаем указатели адресов DMA
							LL_DMA_STREAM_2,
							(uint32_t)LL_I2C_DMA_GetRegAddr(I2C3),							// Указатель в периферии I2C3->DR
							(uint32_t)NULL,													// Указатель в памяти будет установлен позже
							LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_2));

	/* (5) Разрешаем прерывания от потоков DMA */
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_4);
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_2);
}

/*
	Функция конфигурирования I2C контроллера
*/
void Configure_I2C_Master(void)
{
	LL_RCC_ClocksTypeDef rcc_clocks;	// структура для чтения системных частот

	/* (1) Настраиваем SCL и SDA пины контроллера I2C3 **********************/

	/* Конфигурируем вывод SCL GPIOA.9 как : AF4, High Speed, Open drain, Pull up */
	LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_8, LL_GPIO_AF_4);
	LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_8, LL_GPIO_PULL_UP);

	/* Настраиваем вывод SDA GPIOC.9 как : AF4, High Speed, Open drain, Pull up */
	LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetAFPin_8_15(GPIOC, LL_GPIO_PIN_9, LL_GPIO_AF_4);
	LL_GPIO_SetPinSpeed(GPIOC, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetPinOutputType(GPIOC, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_9, LL_GPIO_PULL_UP);

	/* (2) Подаём на I2C3 тактовую частоту *************************************/

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C3);

	/* (3) Конфигурируем прерывания по событию и ошибке для I2C3 **********************************************/

	NVIC_SetPriority(I2C3_EV_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+5);  
	NVIC_EnableIRQ(I2C3_EV_IRQn);

	NVIC_SetPriority(I2C3_ER_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+5);  
	NVIC_EnableIRQ(I2C3_ER_IRQn);

	/* (4) Настраиваем частоту и форму сигнала I2C ********************************/
  
	/* Отключаем контроллер */
	LL_I2C_Disable(I2C3);

	/* Читаем системные частоты процессора */
	LL_RCC_GetSystemClocksFreq(&rcc_clocks);

	/* Настраиваем частоту и форму I2C SCL сигнала */
	LL_I2C_ConfigSpeed(I2C3, rcc_clocks.PCLK1_Frequency, I2C_SPEEDCLOCK, LL_I2C_DUTYCYCLE_2);

	/* (1) Включаем контроллер **********************************************************/
	LL_I2C_Enable(I2C3);

	/* (2) Разрешаем прерывания от I2C3 контроллера
		*  - Прерывание по событию
		*  - Прерывание по ошибке
	*/
	LL_I2C_EnableIT_EVT(I2C3);
	LL_I2C_EnableIT_ERR(I2C3);
}

/*
	Процедура инициализации I2C (контроллер + DMA)
*/
void Init_I2C(void)
{
	i2c_lock = xSemaphoreCreateMutex();	// инициализируем мьютекс
	Configure_I2C_Master();				// инициализируем контроллер
	Configure_DMA();					// инициализируем DMA
}

/*
	Процедура чтения/записи по шине i2c
	dev - адрес i2c устройства (если LSB == 0 - запись, LSB == 1 - чтение)
	addr - адрес в памяти устройства куда нужно записать (откуда нужно считать)
	len - длина массива data в байтах
	data - указатель на массив байт который ножно записать/прочитать из устройства
	T может быть uint8_t (при работе с RTC) или uint16_t (при работе с EEPROM)
*/
template <typename T>
BaseType_t i2c(uint8_t dev, T addr, uint32_t len, const void* data)
{
	static T reg_addr;															// локальная переменная хранения адреса в памяти устройства
	uint32_t ulNotifiedValue;													// переменная для функции xTaskNotifyWait
	if (xSemaphoreTake( i2c_lock, portMAX_DELAY ) == pdFAIL) return pdFAIL;		// захватываем мьютекс
	
	reg_addr = addr;															// сохраняем адрес локально
	device_addr = dev & ~I2C_REQUEST_READ;										// получаем "чистый" адрес устройства
	ubDirection = dev & I2C_REQUEST_READ;										// получаем запрашиваемый тип транзакции (запись/чтение)
	pdata = data;																// сохраняем указатель на буфер данных локально
	xHandlingTask = xTaskGetCurrentTaskHandle();								// привязываем текущую задачу к контроллеру
	/* (1) Устанавливаем указатель DMA и счётчик байт  *******************/

	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_4, (uint32_t)&reg_addr);			// указатель DMA на адрес в памяти устройства
	LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_4, sizeof(T));						// счётчик байт равен длине адреса
	ubMasterNbDataToReceive = len;												// количество байт на передачу равно размеру буфера данных
	/* (2) Включаем поток DMA (будет ожидать запроса контроллера) **********/
 	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_4);

	/* (3) Разрешаем выдачу ACK после каждого байта ************************/
	LL_I2C_AcknowledgeNextData(I2C3, LL_I2C_ACK);

	/* устанавливаем бит чтение/запись на запись */
	ubMasterRequestDirection = I2C_REQUEST_WRITE;

	/* Выставляем старт-условие на шину I2C */
	LL_I2C_GenerateStartCondition(I2C3);
  
	// Ожидаем окончания передачи/приёма
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, 100 );
	// Освобождаем мьютекс
	xSemaphoreGive( i2c_lock );
	
	return pdPASS;
}

template
BaseType_t i2c<uint8_t>(uint8_t dev, uint8_t addr, uint32_t len, const void* data);		// экземпляр функции для 8-ми битного адреса
template
BaseType_t i2c<uint16_t>(uint8_t dev, uint16_t addr, uint32_t len, const void* data);	// экземпляр функции для 16-и битного адреса
/*
	Модуль для работы с контроллером SPI
*/

#include "Global.h"
#include "spi.h"

#ifdef __cplusplus
extern "C" {
#endif
void DMA2_Stream5_IRQHandler(void) __attribute__((interrupt));	// обработчик прерывания от передающего потока DMA
void DMA2_Stream6_IRQHandler(void) __attribute__((interrupt));	// обработчик прерывания от приёмного потока DMA

#ifdef __cplusplus
};
#endif

static TaskHandle_t xHandlingTask = NULL;						// дескриптор связанной с контроллером задачи

/*
	Функция обработчика прерывания от приёмного потока DMA
*/
void DMA2_Stream6_IRQHandler(void)
{
	if(LL_DMA_IsActiveFlag_TC6(DMA2)) {							// если транзакция DMA завершена
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;			// переменная для функции xTaskNotifyFromISR

		LL_DMA_ClearFlag_TC6(DMA2);								// сбрасываем признак окончания транзакции
  /* Если к контроллеру привязана задача, уведомляем её об окончании приёма данных */
		if (xHandlingTask != NULL) {
			xTaskNotifyFromISR( xHandlingTask, SPI_RD_COMPLETE, eSetBits, &xHigherPriorityTaskWoken );
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
	}
}

/*
	Функция обработчика прерывания от передающего потока DMA
*/
void DMA2_Stream5_IRQHandler(void)
{
	if(LL_DMA_IsActiveFlag_TC5(DMA2)) {
		LL_DMA_ClearFlag_TC5(DMA2);		// сбрасываем флаг признака завершения транзакции
	}
}

/*
	Функция конфигурирования контроллера DMA для SPI
*/
void Configure_DMA(void)
{

	/* (2) Конфигурируем прерывания от передающих и приёмных потоков DMA */
	NVIC_SetPriority(DMA2_Stream5_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1);
	NVIC_EnableIRQ(DMA2_Stream5_IRQn);
	NVIC_SetPriority(DMA2_Stream6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1);
	NVIC_EnableIRQ(DMA2_Stream6_IRQn);

	/* (3) Конфигурируем приёмный поток DMA */
	LL_DMA_ConfigTransfer(DMA2,
						LL_DMA_STREAM_6,
						LL_DMA_DIRECTION_PERIPH_TO_MEMORY | LL_DMA_PRIORITY_HIGH | LL_DMA_MODE_NORMAL |		// направление: периферия -> память
						LL_DMA_PERIPH_NOINCREMENT | LL_DMA_MEMORY_INCREMENT |								// адрес в периферии не инкрементируется в памяти инкрементируется
						LL_DMA_PDATAALIGN_BYTE | LL_DMA_MDATAALIGN_BYTE);									// выравнивание побайтно
	LL_DMA_ConfigAddresses(DMA2,																				// выставляем указатели адресов в памяти и периферии
						LL_DMA_STREAM_6,
						LL_SPI_DMA_GetRegAddr(SPI6), (uint32_t)NULL,										// для периферии SPI6->DR, для памяти потом
						LL_DMA_GetDataTransferDirection(DMA2, LL_DMA_STREAM_6));


	LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_6, LL_DMA_CHANNEL_1);										// включаем поток 6, канал 1

	/* (4) Конфигурируем передающий поток DMA */
	LL_DMA_ConfigTransfer(DMA2,
						LL_DMA_STREAM_5,
						LL_DMA_DIRECTION_MEMORY_TO_PERIPH | LL_DMA_PRIORITY_HIGH | LL_DMA_MODE_NORMAL |		// направление: память -> периферия
						LL_DMA_PERIPH_NOINCREMENT | LL_DMA_MEMORY_INCREMENT |								// адрес в периферии не инкрементируется, в памяти инкрементируется
						LL_DMA_PDATAALIGN_BYTE | LL_DMA_MDATAALIGN_BYTE);									// выравнивание побайтно
	LL_DMA_ConfigAddresses(DMA2, LL_DMA_STREAM_5, (uint32_t)NULL, LL_SPI_DMA_GetRegAddr(SPI6),				// указатель адреса для периферии SPI6->DR, для памяти позже
						LL_DMA_GetDataTransferDirection(DMA2, LL_DMA_STREAM_5));

	LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_5, LL_DMA_CHANNEL_1);										// включаем поток 5 канал 1

	/* (5) Разрешаем прерывания от потоков DMA */
	LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_5);
	LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_6);
}

/*
	Функция конфигурирования контроллера SPI
*/
void Configure_SPI(void)
{
  /* Конфигурируем SCK Pin */
  LL_GPIO_SetPinMode(GPIOG, LL_GPIO_PIN_13, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOG, LL_GPIO_PIN_13, LL_GPIO_AF_5);				// Альт. функция 5
  LL_GPIO_SetPinSpeed(GPIOG, LL_GPIO_PIN_13, LL_GPIO_SPEED_FREQ_HIGH);		// высокая скорость
  LL_GPIO_SetPinPull(GPIOG, LL_GPIO_PIN_13, LL_GPIO_PULL_UP);				// подтяжка вверх

  /* Конфигурируем MOSI Pin */
  LL_GPIO_SetPinMode(GPIOG, LL_GPIO_PIN_14, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOG, LL_GPIO_PIN_14, LL_GPIO_AF_5);				// Альт. функция 5
  LL_GPIO_SetPinSpeed(GPIOG, LL_GPIO_PIN_14, LL_GPIO_SPEED_FREQ_HIGH);		// высокая скорость
  LL_GPIO_SetPinPull(GPIOG, LL_GPIO_PIN_14, LL_GPIO_PULL_DOWN);				// подтяжка вниз

  /* Конфигурируем MISO Pin */
  LL_GPIO_SetPinMode(GPIOG, LL_GPIO_PIN_12, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOG, LL_GPIO_PIN_12, LL_GPIO_AF_5);				// Альт. функция 5
  LL_GPIO_SetPinSpeed(GPIOG, LL_GPIO_PIN_12, LL_GPIO_SPEED_FREQ_HIGH);		// высокая скорость
  LL_GPIO_SetPinPull(GPIOG, LL_GPIO_PIN_12, LL_GPIO_PULL_DOWN);				// подтяжка вниз

  /* Подаём на контроллер SPI6 тактовую частоту */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI6);

  /* Конфигурируем параметры сигнала SCK */
  LL_SPI_SetBaudRatePrescaler(SPI6, LL_SPI_BAUDRATEPRESCALER_DIV256);		// задаём частоту сигнала SCK
  LL_SPI_SetTransferDirection(SPI6,LL_SPI_FULL_DUPLEX);						// режим работы full duplex
  LL_SPI_SetClockPhase(SPI6, LL_SPI_PHASE_2EDGE);							// считывание данных по заднему фронту
  LL_SPI_SetClockPolarity(SPI6, LL_SPI_POLARITY_HIGH);						// при паузе - лог. 0
  /* По умолчанию старший байт первый */
  //LL_SPI_SetTransferBitOrder(SPI1, LL_SPI_MSB_FIRST);
  LL_SPI_SetDataWidth(SPI6, LL_SPI_DATAWIDTH_8BIT);							// ширина данных 8 бит.
  LL_SPI_SetNSSMode(SPI6, LL_SPI_NSS_SOFT);									// чип-селект програмно
  LL_SPI_SetMode(SPI6, LL_SPI_MODE_MASTER);									// режим spi-мастер

  /* Enable SPI1 */
  LL_SPI_Enable(SPI6);														// включаем spi
}

/*
	Функция инициализации SPI + DMA
*/
void Init_SPI(void)
{
	Configure_DMA();		
	Configure_SPI();
}

/*
	Запуск обмена по шине SPI
*/
void start_SPI(void)
{
  LL_SPI_EnableDMAReq_RX(SPI6);					// разрешаем запросы DMA от контроллера на приём
  LL_SPI_EnableDMAReq_TX(SPI6);					// ... на передачу
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_6);	// включаем приёмный поток
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_5);	// включаем передающий поток
}

/*
	Функция привязки текущей задачи к контроллеру
*/
void set_spi_handling_task(void)
{
	xHandlingTask = xTaskGetCurrentTaskHandle();
}
/*
	Функция освобождения контроллера от текущей задачи
*/
void clear_spi_handling_task(void)
{
	xHandlingTask = NULL;
}
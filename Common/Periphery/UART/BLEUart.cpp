/*
	Модуль для работы с UART BLE
*/
#include "Global.h"

#define sbiSTREAM_BUFFER_TRIGGER_LEVEL_1	( ( BaseType_t ) 1 )	// начальный порог ожидания байт в стримбуфере = 1

#ifdef __cplusplus
extern "C" {
#endif

void DMA1_Stream3_IRQHandler(void) __attribute__((interrupt));	// обработчик прерывания передающего потока DMA
void USART3_IRQHandler(void) __attribute__((interrupt));		// обработчик прерывания контроллера UART по приёму байта

#ifdef __cplusplus
};
#endif

/* дескриптор стримбуфера для передачи данных из обработчика прерывания в задачу */
static StreamBufferHandle_t BLEStreamBuffer = NULL;

static bool tx_on = false;
/*
	Функция инициализации BLE UART
*/
static void InitBLEUartEngine(void)
{
	LL_RCC_ClocksTypeDef rcc_clocks;	// структура для чтения системных частот
	
  /* Конфигурируем Tx Pin как : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOD, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOD, LL_GPIO_PIN_8, LL_GPIO_AF_7);
  LL_GPIO_SetPinSpeed(GPIOD, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOD, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOD, LL_GPIO_PIN_8, LL_GPIO_PULL_UP);
  
  /* Конфигурируем Rx Pin как : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOD, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOD, LL_GPIO_PIN_9, LL_GPIO_AF_7);
  LL_GPIO_SetPinSpeed(GPIOD, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOD, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOD, LL_GPIO_PIN_9, LL_GPIO_PULL_UP);

  /* (1) Конфигурируем прерывания от UART */
  NVIC_SetPriority(USART3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);  
  NVIC_EnableIRQ(USART3_IRQn);
  /* (2) Подаём тактовую частоту на контроллер UART ****************/
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);

  /* Отключаем UART перед конфигурированием */
  /* Закоментировано, поскольку отключен после резета */
  // LL_USART_Disable(USART2);

  /* Настраиваем двунаправленный обмен */
  LL_USART_SetTransferDirection(USART3, LL_USART_DIRECTION_TX_RX);

  /* 8 бит данных, 1 стартовый бит, 1 стоповый бит, без контроля чётности */
  LL_USART_ConfigCharacter(USART3, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);

  /* Без аппаратного контроля потока */
  /* Значение устанавливается после сброса */
  // LL_USART_SetHWFlowCtrl(USART2, LL_USART_HWCONTROL_NONE);

  /* 16-ти кратный оверсемплинг */
  /* Значение устанавливается после сброса */
  // LL_USART_SetOverSampling(USART2, LL_USART_OVERSAMPLING_16);

	LL_RCC_GetSystemClocksFreq(&rcc_clocks);
  /* Частота обмена 115200 б/с при частоте APB шины установленной в SystemCoreClock/4 Hz */
  LL_USART_SetBaudRate(USART3, rcc_clocks.PCLK1_Frequency, LL_USART_OVERSAMPLING_16, 115200); 

  /* (4) Включаем USART3 **********************************************************/
  LL_USART_Enable(USART3);

  /* Разрешаем прерывания по приёму байта */
  LL_USART_EnableIT_RXNE(USART3);

}

/*
	Функция инициализации контроллера DMA для USART3
*/
static void InitBLEUartDMA(void)
{
	/* (1) Подаём тактовую частоту на контроллер DMA1 */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1); 

  /* (2) Конфигурируем прерывание от передающего потока DMA */
  NVIC_SetPriority(DMA1_Stream3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);
  NVIC_EnableIRQ(DMA1_Stream3_IRQn);

  /* (3) Конфигурируем передающий поток DMA */
  LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_3, LL_DMA_CHANNEL_4);				// Включаем поток 3 канал 4
  LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_3, 
                        LL_DMA_DIRECTION_MEMORY_TO_PERIPH | 						// направление память -> периферия
                        LL_DMA_PRIORITY_LOW               | 						// приоритет низкий
                        LL_DMA_MODE_NORMAL                | 						// режим NORMAL
                        LL_DMA_PERIPH_NOINCREMENT         | 						// адрес в периферии не инкрементируем
                        LL_DMA_MEMORY_INCREMENT           | 						// адрес в памяти инкрементируем
                        LL_DMA_PDATAALIGN_BYTE            | 						// выравнивание на стороне периферии - побайтное
                        LL_DMA_MDATAALIGN_BYTE);									// выравнивание на стороне памяти - побайтовое
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_STREAM_3,
                         (uint32_t)NULL,											// указатель в памяти установим позже
                         LL_USART_DMA_GetRegAddr(USART3),							// указатель в периферии USART3->DR
                         LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_3));
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_3, 0);									// размер транзакции установим позже
  
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_3);										// разрешаем прерывание от передающего потока DMA

}

/*
	Функция обработчика прерывания от DMA
*/
void DMA1_Stream3_IRQHandler(void)
{
    LL_DMA_ClearFlag_TC3(DMA1);		// сбрасываем признак окончания передачи по DMA
	tx_on = false;
}

/*
	Функция обработчика прерывания USART по приёму байта
*/
void USART3_IRQHandler(void)
{
  __IO uint8_t received_char;	// переменная хранения принятого значения
  
  /* Если установлен флаг принятого байта */
  if(LL_USART_IsActiveFlag_RXNE(USART3) && LL_USART_IsEnabledIT_RXNE(USART3))
  {
  /* Читаем принятый байт. При этом флаг прерывания автоматически сбрасывается */
	received_char = LL_USART_ReceiveData8(USART3);

	/* Записываем принятый байт в стримбуфер. */
	if (BLEStreamBuffer)
		xStreamBufferSendFromISR( BLEStreamBuffer, (const uint8_t*)&received_char, 1, NULL );
  }
}

/*
	Функция инициализации BLE UART
	xBufferSizeBytes - размер создаваемого для приёма байт стрингбуфера
	Функция возвращает указатель на созданный стримбуфер
*/
StreamBufferHandle_t InitBLEUart(size_t xBufferSizeBytes)
{
	BLEStreamBuffer = xStreamBufferCreate( xBufferSizeBytes, sbiSTREAM_BUFFER_TRIGGER_LEVEL_1 );	// создаём стримбуфер
	InitBLEUartEngine();		// инициализируем контроллер
	InitBLEUartDMA();			// инициализируем DMA
	return BLEStreamBuffer;
}

/*
	Функция передачи данных по UART BLE
	len - длина передаваемого массива в байтах
	data - указатель на передаваемый массив
*/
void BLEUartTx(uint32_t len, uint8_t *data)
{
  while (tx_on) taskYIELD();
  tx_on = true;
  while (!LL_USART_IsActiveFlag_TC(USART3)) taskYIELD();

  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_3, (uint32_t)data);	// Устанавливаем указатель DMA на передаваемый массив
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_3, len);					// указываем передаваемое кол-во байт

  // Разрешаем UART-запросы к DMA
  LL_USART_EnableDMAReq_TX(USART3);

  // Включаем передающий поток DMA
  LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_3);
  
  while (tx_on) taskYIELD();	// Ожидаем пока не завершится передача. 
}
/*
	Функция приёма данных из UART BLE
	len - размер принимаемого массива байт
	data - указатель на приёмный буфер
*/
int32_t BLEUartRx(uint32_t len, uint8_t *data)
{
	size_t plen = xStreamBufferBytesAvailable(BLEStreamBuffer) + xStreamBufferSpacesAvailable(BLEStreamBuffer);
	if (len == 0) return 0;
//	xStreamBufferReset( BLEStreamBuffer );
//	xStreamBufferSetTriggerLevel( BLEStreamBuffer, len );			// устанавливаем новый порог ожидания в стримбуфере, равный длине принимающего массива
	return xStreamBufferReceive( BLEStreamBuffer, data, len < plen ? len : plen, portMAX_DELAY);	// читаем байты из стримбуфера
}
/*
	Функция проверки наличия данных в стримбуфере
	Возвращает количество байт в буфере
*/
int32_t BLEUartPeek(void)
{
	return xStreamBufferBytesAvailable(BLEStreamBuffer);
}

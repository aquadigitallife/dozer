/*
	Модуль для работы с UART RTU
*/
#include "Global.h"

#define sbiSTREAM_BUFFER_TRIGGER_LEVEL_1	( ( BaseType_t ) 1 )	// начальный порог ожидания байт в стримбуфере = 1

#ifdef __cplusplus
extern "C" {
#endif

void DMA1_Stream1_IRQHandler(void) __attribute__((interrupt));	// обработчик прерывания передающего потока DMA
void UART7_IRQHandler(void) __attribute__((interrupt));			// обработчик прерывания контроллера UART по приёму байта

#ifdef __cplusplus
};
#endif

/* дескриптор стримбуфера для передачи данных из обработчика прерывания в задачу */
static StreamBufferHandle_t RTUStreamBuffer = NULL;

/*
	Функция инициализации RTU UART
*/
static void InitRTUUartEngine(void)
{
  /* Конфигурируем Tx Pin как : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOE, LL_GPIO_PIN_8, LL_GPIO_AF_8);
  LL_GPIO_SetPinSpeed(GPIOE, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOE, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOE, LL_GPIO_PIN_8, LL_GPIO_PULL_UP);
  
  /* Конфигурируем Rx Pin как : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_0_7(GPIOE, LL_GPIO_PIN_7, LL_GPIO_AF_8);
  LL_GPIO_SetPinSpeed(GPIOE, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOE, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOE, LL_GPIO_PIN_7, LL_GPIO_PULL_UP);

  /* (1) Конфигурируем прерывания от UART */
  NVIC_SetPriority(UART7_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);  
  NVIC_EnableIRQ(UART7_IRQn);
  /* (2) Подаём тактовую частоту на контроллер UART ****************/
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART7);

  /* Отключаем UART перед конфигурированием */
  /* Закоментировано, поскольку отключен после резета */
  // LL_USART_Disable(UART7);

  /* Настраиваем двунаправленный обмен */
  LL_USART_SetTransferDirection(UART7, LL_USART_DIRECTION_TX_RX);

  /* 8 бит данных, 1 стартовый бит, 1 стоповый бит, без контроля чётности */
  LL_USART_ConfigCharacter(UART7, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);

  /* Без аппаратного контроля потока */
  /* Значение устанавливается после сброса */
  // LL_USART_SetHWFlowCtrl(USART2, LL_USART_HWCONTROL_NONE);

  /* 16-ти кратный оверсемплинг */
  /* Значение устанавливается после сброса */
  // LL_USART_SetOverSampling(USART2, LL_USART_OVERSAMPLING_16);

  /* Частота обмена 921600 б/с при частоте APB шины установленной в SystemCoreClock/4 Hz */
  LL_USART_SetBaudRate(UART7, SystemCoreClock/4, LL_USART_OVERSAMPLING_16, 921600); 

  /* (4) Включаем USART7 **********************************************************/
  LL_USART_Enable(UART7);

  /* Разрешаем прерывания по приёму байта */
  LL_USART_EnableIT_RXNE(UART7);
}

/*
	Функция инициализации контроллера DMA для UART7
*/
static void InitRTUUartDMA(void)
{
	/* (1) Подаём тактовую частоту на контроллер DMA1 */
//  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1); 

  /* (2) Конфигурируем прерывание от передающего потока DMA */
  NVIC_SetPriority(DMA1_Stream1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);
  NVIC_EnableIRQ(DMA1_Stream1_IRQn);

  /* (3) Конфигурируем передающий поток DMA */
  LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_1, LL_DMA_CHANNEL_5);				// Включаем поток 1 канал 5
  LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_1, 
                        LL_DMA_DIRECTION_MEMORY_TO_PERIPH | 						// направление память -> периферия 
                        LL_DMA_PRIORITY_LOW               | 						// приоритет низкий 
                        LL_DMA_MODE_NORMAL                | 						// режим NORMAL
                        LL_DMA_PERIPH_NOINCREMENT         | 						// адрес в периферии не инкрементируем
                        LL_DMA_MEMORY_INCREMENT           | 						// адрес в памяти инкрементируем
                        LL_DMA_PDATAALIGN_BYTE            | 						// выравнивание на стороне периферии - побайтное 
                        LL_DMA_MDATAALIGN_BYTE);									// выравнивание на стороне памяти - побайтовое
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_STREAM_1,
                         (uint32_t)NULL,											// указатель в памяти установим позже
                         LL_USART_DMA_GetRegAddr(UART7),							// указатель в периферии USART3->DR
                         LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_1));
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_1, 0);									// размер транзакции установим позже
  
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_1);										// разрешаем прерывание от передающего потока DMA

}

/*
	Функция обработчика прерывания от DMA
*/
void DMA1_Stream1_IRQHandler(void)
{
    LL_DMA_ClearFlag_TC1(DMA1);		// сбрасываем признак окончания передачи по DMA
}

/*
	Функция обработчика прерывания USART по приёму байта
*/
void UART7_IRQHandler(void)
{
  __IO uint8_t received_char;	// переменная хранения принятого значения
  
  /* Если установлен флаг принятого байта */
  if(LL_USART_IsActiveFlag_RXNE(UART7) && LL_USART_IsEnabledIT_RXNE(UART7))
  {
  /* Читаем принятый байт. При этом флаг прерывания автоматически сбрасывается */
	received_char = LL_USART_ReceiveData8(UART7);

	/* Записываем принятый байт в стримбуфер. */
	if (RTUStreamBuffer)
		xStreamBufferSendFromISR( RTUStreamBuffer, (const uint8_t*)&received_char, 1, NULL );
  }
}

/*
	Функция инициализации RTU UART
	xBufferSizeBytes - размер создаваемого для приёма байт стрингбуфера
	Функция возвращает указатель на созданный стримбуфер
*/
StreamBufferHandle_t InitRTUUart(size_t xBufferSizeBytes)
{
	RTUStreamBuffer = xStreamBufferCreate( xBufferSizeBytes, sbiSTREAM_BUFFER_TRIGGER_LEVEL_1 );	// создаём стримбуфер
	InitRTUUartEngine();		// инициализируем контроллер
	InitRTUUartDMA();			// инициализируем DMA
	return RTUStreamBuffer;
}

/*
	Функция передачи данных по UART RTU
	len - длина передаваемого массива в байтах
	data - указатель на передаваемый массив
*/
void RTUUartTx(size_t len, void *data)
{
  while (LL_DMA_IsActiveFlag_TC1(DMA1)) taskYIELD();	// Ожидаем пока не завершится предыдущая передача

  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_1, (uint32_t)data);	// Устанавливаем указатель DMA на передаваемый массив
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_1, len);					// указываем передаваемое кол-во байт

  // Разрешаем UART-запросы к DMA
  LL_USART_EnableDMAReq_TX(UART7);

  // Включаем передающий поток DMA
  LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_1);
  
}

/*
	Функция приёма данных из UART RTU
	len - размер принимаемого массива байт
	data - указатель на приёмный буфер
*/
size_t RTUUartRx(size_t len, void *data)
{
//	xStreamBufferReset( BLEStreamBuffer );
	xStreamBufferSetTriggerLevel( RTUStreamBuffer, len );			// устанавливаем новый порог ожидания в стримбуфере, равный длине принимающего массива
	return xStreamBufferReceive( RTUStreamBuffer, data, len, 500 );	// читаем байты из стримбуфера
}

/*
	Функция проверки наличия данных в стримбуфере
	Возвращает количество байт в буфере
*/

size_t RTUUartPeek(void)
{
	return xStreamBufferBytesAvailable(RTUStreamBuffer);
}

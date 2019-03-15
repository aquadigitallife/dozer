/*
	Модуль для работы с UART RTU
*/
#include "Global.h"

#define sbiSTREAM_BUFFER_TRIGGER_LEVEL_1	( ( BaseType_t ) 1 )	// начальный порог ожидания байт в стримбуфере = 1

#ifdef __cplusplus
extern "C" {
#endif

void DMA2_Stream7_IRQHandler(void) __attribute__((interrupt));	// обработчик прерывания передающего потока DMA
void USART6_IRQHandler(void) __attribute__((interrupt));			// обработчик прерывания контроллера UART по приёму байта

#ifdef __cplusplus
};
#endif

/* дескриптор стримбуфера для передачи данных из обработчика прерывания в задачу */
static StreamBufferHandle_t GSMStreamBuffer = NULL;

static bool tx_on = false;

/*
	Функция инициализации RTU UART
*/
static void InitGSMUartEngine(void)
{
	LL_RCC_ClocksTypeDef rcc_clocks;	// структура для чтения системных частот

  /* Конфигурируем Tx Pin как : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_0_7(GPIOC, LL_GPIO_PIN_6, LL_GPIO_AF_8);
  LL_GPIO_SetPinSpeed(GPIOC, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOC, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_6, LL_GPIO_PULL_UP);
  
  /* Конфигурируем Rx Pin как : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_0_7(GPIOC, LL_GPIO_PIN_7, LL_GPIO_AF_8);
  LL_GPIO_SetPinSpeed(GPIOC, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOC, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_7, LL_GPIO_PULL_UP);

  /* (1) Конфигурируем прерывания от UART */
  NVIC_SetPriority(USART6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);  
  NVIC_EnableIRQ(USART6_IRQn);
  /* (2) Подаём тактовую частоту на контроллер UART ****************/
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6);

  /* Отключаем UART перед конфигурированием */
  /* Закоментировано, поскольку отключен после резета */
  // LL_USART_Disable(USART6);

  /* Настраиваем двунаправленный обмен */
  LL_USART_SetTransferDirection(USART6, LL_USART_DIRECTION_TX_RX);

  /* 8 бит данных, 1 стартовый бит, 1 стоповый бит, без контроля чётности */
  LL_USART_ConfigCharacter(USART6, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);

  /* Без аппаратного контроля потока */
  /* Значение устанавливается после сброса */
  // LL_USART_SetHWFlowCtrl(USART6, LL_USART_HWCONTROL_NONE);

  /* 16-ти кратный оверсемплинг */
  /* Значение устанавливается после сброса */
  // LL_USART_SetOverSampling(USART6, LL_USART_OVERSAMPLING_16);
  
	LL_RCC_GetSystemClocksFreq(&rcc_clocks);

  /* Частота обмена 921600 б/с при частоте APB шины установленной в SystemCoreClock/4 Hz */
  LL_USART_SetBaudRate(USART6, rcc_clocks.PCLK2_Frequency, LL_USART_OVERSAMPLING_16, 115200); 

  /* (4) Включаем USART6 **********************************************************/
  LL_USART_Enable(USART6);

  /* Разрешаем прерывания по приёму байта */
  LL_USART_EnableIT_RXNE(USART6);
}

/*
	Функция инициализации контроллера DMA для USART6
*/
static void InitGSMUartDMA(void)
{
	/* (1) Подаём тактовую частоту на контроллер DMA1 */
//  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1); 

  /* (2) Конфигурируем прерывание от передающего потока DMA */
  NVIC_SetPriority(DMA2_Stream7_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);
  NVIC_EnableIRQ(DMA2_Stream7_IRQn);

  /* (3) Конфигурируем передающий поток DMA */
  LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_7, LL_DMA_CHANNEL_5);				// Включаем поток 1 канал 5
  LL_DMA_ConfigTransfer(DMA2, LL_DMA_STREAM_7, 
                        LL_DMA_DIRECTION_MEMORY_TO_PERIPH | 						// направление память -> периферия 
                        LL_DMA_PRIORITY_LOW               | 						// приоритет низкий 
                        LL_DMA_MODE_NORMAL                | 						// режим NORMAL
                        LL_DMA_PERIPH_NOINCREMENT         | 						// адрес в периферии не инкрементируем
                        LL_DMA_MEMORY_INCREMENT           | 						// адрес в памяти инкрементируем
                        LL_DMA_PDATAALIGN_BYTE            | 						// выравнивание на стороне периферии - побайтное 
                        LL_DMA_MDATAALIGN_BYTE);									// выравнивание на стороне памяти - побайтовое
  LL_DMA_ConfigAddresses(DMA2, LL_DMA_STREAM_7,
                         (uint32_t)NULL,											// указатель в памяти установим позже
                         LL_USART_DMA_GetRegAddr(USART6),							// указатель в периферии USART6->DR
                         LL_DMA_GetDataTransferDirection(DMA2, LL_DMA_STREAM_7));
  LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_7, 0);									// размер транзакции установим позже
  
  LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_7);										// разрешаем прерывание от передающего потока DMA

}

/*
	Функция обработчика прерывания от DMA
*/
void DMA2_Stream7_IRQHandler(void)
{
    LL_DMA_ClearFlag_TC7(DMA2);		// сбрасываем признак окончания передачи по DMA
	tx_on = false;
}

/*
	Функция обработчика прерывания USART по приёму байта
*/
void USART6_IRQHandler(void)
{
  __IO uint8_t received_char;	// переменная хранения принятого значения
  
  /* Если установлен флаг принятого байта */
  if(LL_USART_IsActiveFlag_RXNE(USART6) && LL_USART_IsEnabledIT_RXNE(USART6))
  {
  /* Читаем принятый байт. При этом флаг прерывания автоматически сбрасывается */
	received_char = LL_USART_ReceiveData8(USART6);

	/* Записываем принятый байт в стримбуфер. */
	if (GSMStreamBuffer)
		xStreamBufferSendFromISR( GSMStreamBuffer, (const uint8_t*)&received_char, 1, NULL );
  }
}

/*
	Функция инициализации RTU UART
	xBufferSizeBytes - размер создаваемого для приёма байт стрингбуфера
	Функция возвращает указатель на созданный стримбуфер
*/
StreamBufferHandle_t InitGSMUart(size_t xBufferSizeBytes)
{
	GSMStreamBuffer = xStreamBufferCreate( xBufferSizeBytes, sbiSTREAM_BUFFER_TRIGGER_LEVEL_1 );	// создаём стримбуфер
	InitGSMUartEngine();		// инициализируем контроллер
	InitGSMUartDMA();			// инициализируем DMA
	return GSMStreamBuffer;
}

/*
	Функция передачи данных по UART RTU
	len - длина передаваемого массива в байтах
	data - указатель на передаваемый массив
*/
void GSMUartTx(size_t len, void *data)
{
//  while (tx_on) taskYIELD();	// Ожидаем пока не завершится передача. 
  tx_on = true;
  while (!LL_USART_IsActiveFlag_TC(USART6)) taskYIELD();
  
  LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_7, (uint32_t)data);	// Устанавливаем указатель DMA на передаваемый массив
  LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_7, len);					// указываем передаваемое кол-во байт

  // Разрешаем UART-запросы к DMA
  LL_USART_EnableDMAReq_TX(USART6);

  // Включаем передающий поток DMA
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_7);
  while (tx_on) taskYIELD();	// Ожидаем пока не завершится передача. 

}

/*
	Функция приёма данных из UART RTU
	len - размер принимаемого массива байт
	data - указатель на приёмный буфер
*/
size_t GSMUartRx(size_t len, void *data)
{
	size_t plen = xStreamBufferBytesAvailable(GSMStreamBuffer) + xStreamBufferSpacesAvailable(GSMStreamBuffer);
	if (len == 0) return 0;
//	xStreamBufferReset( GSMStreamBuffer );
//	xStreamBufferSetTriggerLevel( GSMStreamBuffer, len < plen ? len : plen );			// устанавливаем новый порог ожидания в стримбуфере, равный длине принимающего массива
	return xStreamBufferReceive( GSMStreamBuffer, data, len < plen ? len : plen, portMAX_DELAY );	// читаем байты из стримбуфера
}

/*
	Функция проверки наличия данных в стримбуфере
	Возвращает количество байт в буфере
*/

size_t GSMUartPeek(void)
{
	return xStreamBufferBytesAvailable(GSMStreamBuffer);
}

void GSMUartClean(void)
{
	xStreamBufferReset( GSMStreamBuffer );
}
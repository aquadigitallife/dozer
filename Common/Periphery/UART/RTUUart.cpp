#include "Global.h"

#define sbiSTREAM_BUFFER_TRIGGER_LEVEL_1	( ( BaseType_t ) 1 )

#ifdef __cplusplus
extern "C" {
#endif

void DMA1_Stream1_IRQHandler(void) __attribute__((interrupt));
void UART7_IRQHandler(void) __attribute__((interrupt));

#ifdef __cplusplus
};
#endif

/* The stream buffer that is used to send data from an interrupt to the task. */
static StreamBufferHandle_t RTUStreamBuffer = NULL;

static void InitRTUUartEngine(void)
{
  /* Configure Tx Pin as : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOE, LL_GPIO_PIN_8, LL_GPIO_AF_8);
  LL_GPIO_SetPinSpeed(GPIOE, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOE, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOE, LL_GPIO_PIN_8, LL_GPIO_PULL_UP);
  
  /* Configure Rx Pin as : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_0_7(GPIOE, LL_GPIO_PIN_7, LL_GPIO_AF_8);
  LL_GPIO_SetPinSpeed(GPIOE, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOE, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOE, LL_GPIO_PIN_7, LL_GPIO_PULL_UP);

  /* (2) NVIC Configuration for USART interrupts */
  /*  - Set priority for USARTx_IRQn */
  /*  - Enable USARTx_IRQn */
  NVIC_SetPriority(UART7_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);  
  NVIC_EnableIRQ(UART7_IRQn);
  /* (2) Enable UART7 peripheral clock and clock source ****************/
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART7);

  /* (3) Configure UART7 functional parameters ********************************/
  
  /* Disable UART prior modifying configuration registers */
  /* Note: Commented as corresponding to Reset value */
  // LL_USART_Disable(UART7);

  /* TX/RX direction */
  LL_USART_SetTransferDirection(UART7, LL_USART_DIRECTION_TX_RX);

  /* 8 data bit, 1 start bit, 1 stop bit, no parity */
  LL_USART_ConfigCharacter(UART7, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);

  /* No Hardware Flow control */
  /* Reset value is LL_USART_HWCONTROL_NONE */
  // LL_USART_SetHWFlowCtrl(USART2, LL_USART_HWCONTROL_NONE);

  /* Oversampling by 16 */
  /* Reset value is LL_USART_OVERSAMPLING_16 */
  // LL_USART_SetOverSampling(USART2, LL_USART_OVERSAMPLING_16);

  /* Set Baudrate to 115200 using APB frequency set to SystemCoreClock/4 Hz */
  LL_USART_SetBaudRate(UART7, SystemCoreClock/4, LL_USART_OVERSAMPLING_16, 921600); 

  /* (4) Enable UART7 **********************************************************/
  LL_USART_Enable(UART7);

  /* Enable RXNE and Error interrupts */
  LL_USART_EnableIT_RXNE(UART7);
//  LL_USART_EnableIT_ERROR(UART7);

}

static void InitRTUUartDMA(void)
{
	/* (1) Enable the clock of DMA1 */
//  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1); 

  /* (2) Configure NVIC for DMA transfer complete/error interrupts */
  NVIC_SetPriority(DMA1_Stream1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);
  NVIC_EnableIRQ(DMA1_Stream1_IRQn);

  /* (3) Configure the DMA functional parameters for transmission */
  LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_1, LL_DMA_CHANNEL_5);
  LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_1, 
                        LL_DMA_DIRECTION_MEMORY_TO_PERIPH | 
                        LL_DMA_PRIORITY_LOW               | 
                        LL_DMA_MODE_NORMAL                | 
                        LL_DMA_PERIPH_NOINCREMENT         | 
                        LL_DMA_MEMORY_INCREMENT           | 
                        LL_DMA_PDATAALIGN_BYTE            | 
                        LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_STREAM_1,
                         (uint32_t)NULL,
                         LL_USART_DMA_GetRegAddr(UART7),
                         LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_1));
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_1, 0);
  
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_1);

}

void DMA1_Stream1_IRQHandler(void)
{
    LL_DMA_ClearFlag_TC1(DMA1);
}


void UART7_IRQHandler(void)
{
  __IO uint8_t received_char;
  
  /* Check RXNE flag value in SR register */
  if(LL_USART_IsActiveFlag_RXNE(UART7) && LL_USART_IsEnabledIT_RXNE(UART7))
  {
  /* Read Received character. RXNE flag is cleared by reading of DR register */
	received_char = LL_USART_ReceiveData8(UART7);

	/* Send the received character to the stream buffer. */
	if (RTUStreamBuffer)
		xStreamBufferSendFromISR( RTUStreamBuffer, (const uint8_t*)&received_char, 1, NULL );
  }
//  else
//  {
    /* Call Error function */
//    Error_Callback();
//  }
	
}

StreamBufferHandle_t InitRTUUart(size_t xBufferSizeBytes)
{
	RTUStreamBuffer = xStreamBufferCreate( xBufferSizeBytes, sbiSTREAM_BUFFER_TRIGGER_LEVEL_1 );
	InitRTUUartEngine();
	InitRTUUartDMA();
	return RTUStreamBuffer;
}

void RTUUartTx(size_t len, void *data)
{
  while (LL_DMA_IsActiveFlag_TC1(DMA1)) taskYIELD();

  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_1, (uint32_t)data);
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_1, len);

  // Enable DMA TX Request
  LL_USART_EnableDMAReq_TX(UART7);

  // Enable DMA Channel Tx
  LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_1);
  
}

size_t RTUUartRx(size_t len, void *data)
{
//	xStreamBufferReset( BLEStreamBuffer );
	xStreamBufferSetTriggerLevel( RTUStreamBuffer, len );
	return xStreamBufferReceive( RTUStreamBuffer, data, len, 500 );
}

size_t RTUUartPeek(void)
{
	return xStreamBufferBytesAvailable(RTUStreamBuffer);
}

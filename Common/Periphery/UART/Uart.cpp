#include "Global.h"

extern "C" {
void DMA1_Stream3_IRQHandler(void) __attribute__((interrupt));
}

void InitBLEUartEngine(void)
{
  /* Configure Tx Pin as : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOD, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOD, LL_GPIO_PIN_8, LL_GPIO_AF_7);
  LL_GPIO_SetPinSpeed(GPIOD, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOD, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOD, LL_GPIO_PIN_8, LL_GPIO_PULL_UP);
  
  /* Configure Rx Pin as : Alternate function, High Speed, Push pull, Pull up */
  LL_GPIO_SetPinMode(GPIOD, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOD, LL_GPIO_PIN_9, LL_GPIO_AF_7);
  LL_GPIO_SetPinSpeed(GPIOD, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOD, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOD, LL_GPIO_PIN_9, LL_GPIO_PULL_UP);

  /* (2) Enable USART3 peripheral clock and clock source ****************/
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);


  /* (3) Configure USART3 functional parameters ********************************/
  
  /* Disable USART prior modifying configuration registers */
  /* Note: Commented as corresponding to Reset value */
  // LL_USART_Disable(USART2);

  /* TX/RX direction */
  LL_USART_SetTransferDirection(USART3, LL_USART_DIRECTION_TX_RX);

  /* 8 data bit, 1 start bit, 1 stop bit, no parity */
  LL_USART_ConfigCharacter(USART3, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);

  /* No Hardware Flow control */
  /* Reset value is LL_USART_HWCONTROL_NONE */
  // LL_USART_SetHWFlowCtrl(USART2, LL_USART_HWCONTROL_NONE);

  /* Oversampling by 16 */
  /* Reset value is LL_USART_OVERSAMPLING_16 */
  // LL_USART_SetOverSampling(USART2, LL_USART_OVERSAMPLING_16);

  /* Set Baudrate to 115200 using APB frequency set to 500000000 Hz */
  /* Frequency available for USART peripheral can also be calculated through LL RCC macro */
  /* Ex :
      Periphclk = LL_RCC_GetUSARTClockFreq(Instance); or LL_RCC_GetUARTClockFreq(Instance); depending on USART/UART instance
  
      In this example, Peripheral Clock is expected to be equal to 500000000 Hz => equal to SystemCoreClock/2
  */
  LL_USART_SetBaudRate(USART3, SystemCoreClock/4, LL_USART_OVERSAMPLING_16, 115200); 

  /* (4) Enable USART3 **********************************************************/
  LL_USART_Enable(USART3);
}

void InitBLEUartDMA(void)
{
	/* (1) Enable the clock of DMA2 */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1); 

  /* (2) Configure NVIC for DMA transfer complete/error interrupts */
  NVIC_SetPriority(DMA1_Stream3_IRQn, 3);
  NVIC_EnableIRQ(DMA1_Stream3_IRQn);

  /* (3) Configure the DMA functional parameters for transmission */
  LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_3, LL_DMA_CHANNEL_4);
  LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_3, 
                        LL_DMA_DIRECTION_MEMORY_TO_PERIPH | 
                        LL_DMA_PRIORITY_LOW               | 
                        LL_DMA_MODE_NORMAL                | 
                        LL_DMA_PERIPH_NOINCREMENT         | 
                        LL_DMA_MEMORY_INCREMENT           | 
                        LL_DMA_PDATAALIGN_BYTE            | 
                        LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_STREAM_3,
                         (uint32_t)NULL,
                         LL_USART_DMA_GetRegAddr(USART3),
                         LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_3));
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_3, 0);
  
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_3);

}

void DMA1_Stream3_IRQHandler(void)
{
    LL_DMA_ClearFlag_TC3(DMA1);
}


void InitBLEUart(void)
{
	InitBLEUartEngine();
	InitBLEUartDMA();
}

void BLEUartTx(uint32_t len, uint8_t *data)
{
	
  while (LL_DMA_IsActiveFlag_TC3(DMA1)); // Есть-ли во FreeRTOS принудительное переключение контекста?

  WRITE_REG(((DMA_Stream_TypeDef*)((uint32_t)((uint32_t)DMA1 + STREAM_OFFSET_TAB[LL_DMA_STREAM_3])))->M0AR, (uint32_t)data);
  MODIFY_REG(((DMA_Stream_TypeDef*)((uint32_t)((uint32_t)DMA1 + STREAM_OFFSET_TAB[LL_DMA_STREAM_3])))->NDTR, DMA_SxNDT, len);

  /* Enable DMA TX Request */
  LL_USART_EnableDMAReq_TX(USART3);

  /* Enable DMA Channel Tx */
  LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_3);
}



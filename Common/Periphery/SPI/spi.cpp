#include "Global.h"
#include "spi.h"

#ifdef __cplusplus
extern "C" {
#endif
void DMA2_Stream5_IRQHandler(void) __attribute__((interrupt));
void DMA2_Stream6_IRQHandler(void) __attribute__((interrupt));
void EXTI15_10_IRQHandler(void) __attribute__((interrupt));

#ifdef __cplusplus
};
#endif

static TaskHandle_t xHandlingTask = NULL;

/*
void EXTI15_10_IRQHandler(void) {
	if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);

	}
}
*/
void DMA2_Stream6_IRQHandler(void)
{
	if(LL_DMA_IsActiveFlag_TC6(DMA2)) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		LL_DMA_ClearFlag_TC6(DMA2);
  /* Call function Reception complete Callback */
		if (xHandlingTask != NULL) {
			xTaskNotifyFromISR( xHandlingTask, SPI_RD_COMPLETE, eSetBits, &xHigherPriorityTaskWoken );
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
	}
/*
	else if(LL_DMA_IsActiveFlag_TE6(DMA2))
	{
    // Call Error function
		SPI1_TransferError_Callback();
	}
*/
}

void DMA2_Stream5_IRQHandler(void)
{
	if(LL_DMA_IsActiveFlag_TC5(DMA2)) {

		LL_DMA_ClearFlag_TC5(DMA2);
/* Call function Transmission complete Callback */
	}
/*
	else if(LL_DMA_IsActiveFlag_TE5(DMA2))
	{
    // Call Error function
		SPI1_TransferError_Callback();
	}
*/
}


void Configure_DMA(void)
{
  /* DMA2 used for SPI6 Transmission
   * DMA2 used for SPI6 Reception
   */
  /* (1) Enable the clock of DMA2 and DMA2 */
//  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

 /* (2) Configure NVIC for DMA transfer complete/error interrupts */
  NVIC_SetPriority(DMA2_Stream5_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1);
  NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  NVIC_SetPriority(DMA2_Stream6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1);
  NVIC_EnableIRQ(DMA2_Stream6_IRQn);

  /* (3) Configure the DMA2_Stream6  (SPI6RX) functional parameters */
  LL_DMA_ConfigTransfer(DMA2,
                        LL_DMA_STREAM_6,
                        LL_DMA_DIRECTION_PERIPH_TO_MEMORY | LL_DMA_PRIORITY_HIGH | LL_DMA_MODE_NORMAL |
                        LL_DMA_PERIPH_NOINCREMENT | LL_DMA_MEMORY_INCREMENT |
                        LL_DMA_PDATAALIGN_BYTE | LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_ConfigAddresses(DMA2,
                         LL_DMA_STREAM_6,
                         LL_SPI_DMA_GetRegAddr(SPI6), (uint32_t)NULL,
                         LL_DMA_GetDataTransferDirection(DMA2, LL_DMA_STREAM_6));
//  LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, 0);


  LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_6, LL_DMA_CHANNEL_1);

  /* (4) Configure the DMA2_Stream5 (SPI5TX) functional parameters */
  LL_DMA_ConfigTransfer(DMA2,
                        LL_DMA_STREAM_5,
                        LL_DMA_DIRECTION_MEMORY_TO_PERIPH | LL_DMA_PRIORITY_HIGH | LL_DMA_MODE_NORMAL |
                        LL_DMA_PERIPH_NOINCREMENT | LL_DMA_MEMORY_INCREMENT |
                        LL_DMA_PDATAALIGN_BYTE | LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_ConfigAddresses(DMA2, LL_DMA_STREAM_5, (uint32_t)NULL, LL_SPI_DMA_GetRegAddr(SPI6),
                         LL_DMA_GetDataTransferDirection(DMA2, LL_DMA_STREAM_5));
//  LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, 0);

  LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_5, LL_DMA_CHANNEL_1);

  /* (5) Enable DMA interrupts complete/error */
  LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_5);
//  LL_DMA_EnableIT_TE(DMA2, LL_DMA_STREAM_5);
  LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_6);
//  LL_DMA_EnableIT_TE(DMA2, LL_DMA_STREAM_6);
}

void Configure_SPI(void)
{
  /* Configure SCK Pin */
  LL_GPIO_SetPinMode(GPIOG, LL_GPIO_PIN_13, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOG, LL_GPIO_PIN_13, LL_GPIO_AF_5);
  LL_GPIO_SetPinSpeed(GPIOG, LL_GPIO_PIN_13, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinPull(GPIOG, LL_GPIO_PIN_13, LL_GPIO_PULL_UP);

  /* Configure MOSI Pin */
  LL_GPIO_SetPinMode(GPIOG, LL_GPIO_PIN_14, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOG, LL_GPIO_PIN_14, LL_GPIO_AF_5);
  LL_GPIO_SetPinSpeed(GPIOG, LL_GPIO_PIN_14, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinPull(GPIOG, LL_GPIO_PIN_14, LL_GPIO_PULL_DOWN);

  /* Configure MISO Pin */
  LL_GPIO_SetPinMode(GPIOG, LL_GPIO_PIN_12, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOG, LL_GPIO_PIN_12, LL_GPIO_AF_5);
  LL_GPIO_SetPinSpeed(GPIOG, LL_GPIO_PIN_12, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinPull(GPIOG, LL_GPIO_PIN_12, LL_GPIO_PULL_DOWN);

  /* (2) Configure SPI6 functional parameters ********************************/
  /* Enable the peripheral clock of SPI6 */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI6);

  /* Configure SPI6 communication */
  LL_SPI_SetBaudRatePrescaler(SPI6, LL_SPI_BAUDRATEPRESCALER_DIV256);
  LL_SPI_SetTransferDirection(SPI6,LL_SPI_FULL_DUPLEX);
  LL_SPI_SetClockPhase(SPI6, LL_SPI_PHASE_2EDGE);
  LL_SPI_SetClockPolarity(SPI6, LL_SPI_POLARITY_HIGH);
  /* Reset value is LL_SPI_MSB_FIRST */
  //LL_SPI_SetTransferBitOrder(SPI1, LL_SPI_MSB_FIRST);
  LL_SPI_SetDataWidth(SPI6, LL_SPI_DATAWIDTH_8BIT);
  LL_SPI_SetNSSMode(SPI6, LL_SPI_NSS_SOFT);
  LL_SPI_SetMode(SPI6, LL_SPI_MODE_MASTER);

  /* Enable SPI1 */
  LL_SPI_Enable(SPI6);
  
}

void Init_SPI(void)
{
	Configure_DMA();
	Configure_SPI();
}

void start_SPI(void)
{
  LL_SPI_EnableDMAReq_RX(SPI6);
  LL_SPI_EnableDMAReq_TX(SPI6);
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_6);
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_5);
}

void set_spi_handling_task(void)
{
	xHandlingTask = xTaskGetCurrentTaskHandle();
}

void clear_spi_handling_task(void)
{
	xHandlingTask = NULL;
}
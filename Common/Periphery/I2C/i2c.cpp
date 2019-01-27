#include "Global.h"

/* I2C SPEEDCLOCK define to max value: 400 KHz */
#define I2C_SPEEDCLOCK		200000
#define I2C_DUTYCYCLE		LL_I2C_DUTYCYCLE_2

#define I2C_COMPLETE		0x00000001UL
#define I2C_ERROR			0x00000002UL

/**
  * @brief Master Transfer Request Direction
  */
#define I2C_REQUEST_WRITE	0x00
#define I2C_REQUEST_READ	0x01


#ifdef __cplusplus
extern "C" {
#endif
void I2C3_EV_IRQHandler(void) __attribute__((interrupt));
void DMA1_Stream4_IRQHandler(void) __attribute__((interrupt));
void DMA1_Stream2_IRQHandler(void) __attribute__((interrupt));
void I2C3_ER_IRQHandler(void) __attribute__((interrupt));

#ifdef __cplusplus
};
#endif

SemaphoreHandle_t i2c_lock;

__IO uint8_t  ubMasterRequestDirection  = 0;
__IO uint8_t  ubMasterNbDataToReceive   = 1;

uint8_t device_addr;
uint8_t ubDirection = 0;
const void *pdata;

TaskHandle_t xHandlingTask;


/**
  * Brief   This function handles I2C3 (Master) interrupt request.
  * Param   None
  * Retval  None
  */
void I2C3_EV_IRQHandler(void)
{
	/* Check SB flag value in ISR register */
	if(LL_I2C_IsActiveFlag_SB(I2C3)) {
		/* Send Slave address with a 7-Bit SLAVE_OWN_ADDRESS for a ubMasterRequestDirection request */
		LL_I2C_TransmitData8(I2C3, device_addr | ubMasterRequestDirection);
	}
	/* Check ADDR flag value in ISR register */
	else if(LL_I2C_IsActiveFlag_ADDR(I2C3)) {
		// Verify the transfer direction 
		if(LL_I2C_GetTransferDirection(I2C3) == LL_I2C_DIRECTION_READ) {

			if(ubMasterNbDataToReceive == 1) {
				// Prepare the generation of a Non ACKnowledge condition after next received byte 
				LL_I2C_AcknowledgeNextData(I2C3, LL_I2C_NACK);

				// Enable DMA transmission requests 
				LL_I2C_EnableDMAReq_RX(I2C3);
			} else if(ubMasterNbDataToReceive == 2) {
				// Prepare the generation of a Non ACKnowledge condition after next received byte 
				LL_I2C_AcknowledgeNextData(I2C3, LL_I2C_NACK);

				// Enable Pos 
				LL_I2C_EnableBitPOS(I2C3);
			} else {
				// Enable Last DMA bit 
				LL_I2C_EnableLastDMA(I2C3);

				// Enable DMA transmission requests 
				LL_I2C_EnableDMAReq_RX(I2C3);
			}
		} else {
			// Enable DMA transmission requests 
			LL_I2C_EnableDMAReq_TX(I2C3);
		}
		// Clear ADDR flag value in ISR register 
		LL_I2C_ClearFlag_ADDR(I2C3);
	} else if (LL_I2C_IsActiveFlag_BTF(I2C3) && (ubDirection == I2C_REQUEST_WRITE)) {
		if ( ubMasterNbDataToReceive ) {
			ubMasterNbDataToReceive = 0;
			LL_I2C_EnableDMAReq_TX(I2C3);
			LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_4);
		} else {
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			LL_I2C_GenerateStopCondition(I2C3);
			xTaskNotifyFromISR( xHandlingTask, I2C_COMPLETE, eSetBits, &xHigherPriorityTaskWoken );
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
	}
}

/**
  * Brief   This function handles I2C3 (Master) error interrupt request.
  * Param   None
  * Retval  None
  */

void I2C3_ER_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Call Error function
	LL_I2C_ClearFlag_BERR(I2C3);
	LL_I2C_ClearFlag_AF(I2C3);
	LL_I2C_ClearFlag_ARLO(I2C3);
	
//	LL_I2C_ClearFlag_OVR(I2C3);
	
	LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_4);
	LL_DMA_ClearFlag_HT4(DMA1);
    LL_DMA_ClearFlag_TC4(DMA1);
	
	LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_2);
	LL_DMA_ClearFlag_HT2(DMA1);
    LL_DMA_ClearFlag_TC2(DMA1);

	LL_I2C_GenerateStopCondition(I2C3);
	xTaskNotifyFromISR( xHandlingTask, I2C_ERROR, eSetBits, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**
  * @brief  This function handles DMA1_Stream5 interrupt request.
  * @param  None
  * @retval None
  */
void DMA1_Stream4_IRQHandler(void)
{

	if(LL_DMA_IsActiveFlag_TC4(DMA1)) {
		LL_DMA_ClearFlag_HT4(DMA1);
		LL_DMA_ClearFlag_TC4(DMA1);

		LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_4);
		if (ubDirection == I2C_REQUEST_READ) {
			LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_2, (uint32_t)pdata);
			LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_2, ubMasterNbDataToReceive);
			LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_2);

			/* (7) Initiate a ReStart condition to the Slave device *********************/
			/* Master Request direction READ */
			ubMasterRequestDirection = I2C_REQUEST_READ;

			/* Master Generate ReStart condition */
			LL_I2C_GenerateStartCondition(I2C3);

		} else if (ubMasterNbDataToReceive) {
			LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_4, (uint32_t)pdata);
			LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_4, ubMasterNbDataToReceive);
			
//			LL_I2C_EnableDMAReq_TX(I2C3);
//			LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_4);
		}
		/* (6) Prepare acknowledge for Master data reception ************************/
		LL_I2C_AcknowledgeNextData(I2C3, LL_I2C_ACK);
	}
}

/**
  * @brief  This function handles DMA1_Stream2 interrupt request.
  * @param  None
  * @retval None
  */
void DMA1_Stream2_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if(LL_DMA_IsActiveFlag_TC2(DMA1)) {
		LL_DMA_ClearFlag_TC2(DMA1);

		LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_2);
		LL_I2C_GenerateStopCondition(I2C3);
		xTaskNotifyFromISR( xHandlingTask, I2C_COMPLETE, eSetBits, &xHigherPriorityTaskWoken );
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

static void Configure_DMA(void)
{
	/* (2) Configure NVIC for DMA1_Stream5 and DMA1_Stream2 */
	NVIC_SetPriority(DMA1_Stream4_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3);
	NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  
	NVIC_SetPriority(DMA1_Stream2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+4);
	NVIC_EnableIRQ(DMA1_Stream2_IRQn);

	/* (3) Configure the DMA functional parameters for Master Transmit */

	LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_4, LL_DMA_CHANNEL_3);

	LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_4, LL_DMA_DIRECTION_MEMORY_TO_PERIPH | \
												LL_DMA_PRIORITY_HIGH               | \
												LL_DMA_MODE_NORMAL                 | \
												LL_DMA_PERIPH_NOINCREMENT          | \
												LL_DMA_MEMORY_INCREMENT            | \
												LL_DMA_PDATAALIGN_BYTE             | \
												LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_ConfigAddresses(	DMA1,
							LL_DMA_STREAM_4,
							(uint32_t)NULL,
							(uint32_t)LL_I2C_DMA_GetRegAddr(I2C3),
							LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_4));


	/* (4) Configure the DMA functional parameters for Master Receive */
	LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_2, LL_DMA_CHANNEL_3);

	LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_2, LL_DMA_DIRECTION_PERIPH_TO_MEMORY  | \
												LL_DMA_PRIORITY_HIGH				| \
												LL_DMA_MODE_NORMAL					| \
												LL_DMA_PERIPH_NOINCREMENT			| \
												LL_DMA_MEMORY_INCREMENT				| \
												LL_DMA_PDATAALIGN_BYTE				| \
												LL_DMA_MDATAALIGN_BYTE);

	LL_DMA_ConfigAddresses(	DMA1,
							LL_DMA_STREAM_2,
							(uint32_t)LL_I2C_DMA_GetRegAddr(I2C3),
							(uint32_t)NULL,
							LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_2));

	/* (5) Enable DMA1 interrupts complete/error */
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_4);
//	LL_DMA_EnableIT_TE(DMA1, LL_DMA_STREAM_4);
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_2);
//	LL_DMA_EnableIT_TE(DMA1, LL_DMA_STREAM_2);

}


void Configure_I2C_Master(void)
{
	LL_RCC_ClocksTypeDef rcc_clocks;

	/* (1) Enables GPIO clock and configures the I2C3 pins **********************/
	/*    (SCL on PA.8, SDA on PC.9)                     **********************/

	/* Configure SCL Pin as : Alternate function, High Speed, Open drain, Pull up */
	LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_8, LL_GPIO_AF_4);
	LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_8, LL_GPIO_PULL_UP);

	/* Configure SDA Pin as : Alternate function, High Speed, Open drain, Pull up */
	LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetAFPin_8_15(GPIOC, LL_GPIO_PIN_9, LL_GPIO_AF_4);
	LL_GPIO_SetPinSpeed(GPIOC, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetPinOutputType(GPIOC, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_9, LL_GPIO_PULL_UP);

	/* (2) Enable the I2C3 peripheral clock *************************************/

	/* Enable the peripheral clock for I2C3 */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C3);

	/* (3) Configure NVIC for I2C3 **********************************************/

	/* Configure Event IT:
		*  - Set priority for I2C3_EV_IRQn
		*  - Enable I2C3_EV_IRQn
	*/
	NVIC_SetPriority(I2C3_EV_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+5);  
	NVIC_EnableIRQ(I2C3_EV_IRQn);

	/* Configure Error IT:
		*  - Set priority for I2C3_ER_IRQn
		*  - Enable I2C3_ER_IRQn
	*/
	NVIC_SetPriority(I2C3_ER_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+5);  
	NVIC_EnableIRQ(I2C3_ER_IRQn);

	/* (4) Configure I2C3 functional parameters ********************************/
  
	/* Disable I2C3 prior modifying configuration registers */
	LL_I2C_Disable(I2C3);

	/* Retrieve Clock frequencies */
	LL_RCC_GetSystemClocksFreq(&rcc_clocks);

	/* Configure the SCL Clock Speed */
	LL_I2C_ConfigSpeed(I2C3, rcc_clocks.PCLK1_Frequency, I2C_SPEEDCLOCK, LL_I2C_DUTYCYCLE_2);

	/* Configure the Own Address1                   */
	/* Reset Values of :
		*     - OwnAddress1 is 0x00
		*     - OwnAddrSize is LL_I2C_OWNADDRESS1_7BIT
	*/
	//LL_I2C_SetOwnAddress1(I2C3, 0x00, LL_I2C_OWNADDRESS1_7BIT);

	/* Enable Clock stretching */
	/* Reset Value is Clock stretching enabled */
	//LL_I2C_EnableClockStretching(I2C3);


	/* Enable General Call                  */
	/* Reset Value is General Call disabled */
	//LL_I2C_EnableGeneralCall(I2C3);

	/* Configure the 7bits Own Address2     */
	/* Reset Values of :
		*     - OwnAddress2 is 0x00
		*     - Own Address2 is disabled
	*/
	//LL_I2C_SetOwnAddress2(I2C3, 0x00);
	//LL_I2C_DisableOwnAddress2(I2C3);

	/* Enable Peripheral in I2C mode */
	/* Reset Value is I2C mode */
	//LL_I2C_SetMode(I2C3, LL_I2C_MODE_I2C);

	/* (1) Enable I2C3 **********************************************************/
	LL_I2C_Enable(I2C3);

	/* (2) Enable I2C3 transfer event/error interrupts:
		*  - Enable Event interrupts
		*  - Enable Error interrupts
	*/
	LL_I2C_EnableIT_EVT(I2C3);
	LL_I2C_EnableIT_ERR(I2C3);
}

void Init_I2C(void)
{
	i2c_lock = xSemaphoreCreateMutex();
	Configure_I2C_Master();
	Configure_DMA();
}

template <typename T>
BaseType_t i2c(uint8_t dev, T addr, uint32_t len, const void* data)
{
	uint32_t ulNotifiedValue;
	static T reg_addr;
	
	if (xSemaphoreTake( i2c_lock, portMAX_DELAY ) == pdFAIL) return pdFAIL;
	
	reg_addr = addr;
	device_addr = dev & ~I2C_REQUEST_READ;
	ubDirection = dev & I2C_REQUEST_READ;
	pdata = data;
	
	xHandlingTask = xTaskGetCurrentTaskHandle();
	ubMasterNbDataToReceive = len;
	// (1) Configure DMA parameters for Command Code transfer *******************
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_4, (uint32_t)&reg_addr);
	LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_4, sizeof(T));
	// (2) Enable DMA transfer **************************************************
	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_4);
	// (3) Prepare acknowledge for Master data reception ************************
	LL_I2C_AcknowledgeNextData(I2C3, LL_I2C_ACK);

	// (4) Initiate a Start condition to the Slave device ***********************
	// Master Request direction WRITE
	ubMasterRequestDirection = I2C_REQUEST_WRITE;

	// Master Generate Start condition
	LL_I2C_GenerateStartCondition(I2C3);

	// Wait to be notified of a DMA transfer complete interrupt.
	BaseType_t xResult = xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );

	xSemaphoreGive( i2c_lock );

	if( xResult == pdPASS ) {
		switch (ulNotifiedValue) {
			case I2C_COMPLETE:	return pdPASS;
			case I2C_ERROR:
			default:			return pdFAIL;
		}
	}
	
	return xResult;
}

template
BaseType_t i2c<uint8_t>(uint8_t dev, uint8_t addr, uint32_t len, const void* data);
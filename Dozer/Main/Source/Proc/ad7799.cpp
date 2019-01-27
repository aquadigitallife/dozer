#include "Global.h"
#include "spi.h"

#define CR_WEN			0x00
#define CR_WDIS			0x80
#define CR_RD			0x40
#define CR_WR			0x00
#define CR_ADDR_MASK	((uint8_t)0x38)
#define CR_ADDR_CR		((uint8_t)0x00)
#define CR_ADDR_SR		((uint8_t)0x00)
#define CR_ADDR_MR		((uint8_t)(1 << 3))
#define CR_ADDR_CON		((uint8_t)(2 << 3))
#define CR_ADDR_DR		((uint8_t)(3 << 3))
#define CR_ADDR_ID		((uint8_t)(4 << 3))
#define CR_ADDR_IO		((uint8_t)(5 << 3))
#define CR_ADDR_OFFSET	((uint8_t)(6 << 3))
#define CR_ADDR_SCALE	((uint8_t)(7 << 3))
#define CR_CREAD		0x04

#define MR_MODE_CONT	0x00
#define MR_MODE_SINGLE	0x20
#define MR_MODE_IDLE	0x40
#define MR_MODE_PD		0x60
#define MR_MODE_IZERO	0x80
#define MR_MODE_IFSCALE	0xA0
#define MR_MODE_SZERO	0xC0
#define MR_MODE_SFSCALE	0xE0
#define MR_PSW_ON		0x10
#define MR_PSW_OFF		0x00
#define MR_RATE_470HZ	0x01
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
void EXTI15_10_IRQHandler(void) {
	if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12) != RESET)
	{
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);

	}
}
*/

template <typename T>
T ad7799_rd(uint8_t addr)
{
//	BaseType_t xResult;
	uint32_t ulNotifiedValue;
	
	static const T null_val = 0;
	T retval = 0;
	T pretval = 0;
	
	uint8_t cr = CR_WEN | CR_RD | addr;
	
	AD7799_CS_SELECT;
	set_spi_handling_task();
	while (!IS_AD7799_CS_SELECT);

	// (1) Configure DMA parameters for communication register transfer *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&cr);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(cr));

	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&pretval);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(cr));
	// (2) Enable DMA transfer **************************************************
	
	start_SPI();
	// Wait to be notified of a DMA transfer complete interrupt.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );
	
	// (1) Configure DMA parameters for communication register transfer *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&null_val);
	if (sizeof(T) > 2)
		LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, 3);
	else
		LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(T));

	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&pretval);
	if (sizeof(T) > 2)
		LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, 3);
	else
		LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(T));
	// (2) Enable DMA transfer **************************************************
	
	start_SPI();
	// Wait to be notified of a DMA transfer complete interrupt.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );

	AD7799_CS_DESELECT;
	clear_spi_handling_task();
/*	
	for (int i = sizeof(T); i > 0; i--) {
		((uint8_t*)&retval)[sizeof(T) - i] = ((uint8_t*)&pretval)[i - 1];
	}
	if (sizeof(T) > 2) retval = retval >> ((sizeof(T) - 3)*8);
*/
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

void ad7799_wr(uint8_t addr, uint8_t value)
{
//	BaseType_t xResult;
	uint32_t ulNotifiedValue;
	
	uint8_t tmp;
	
	const uint8_t pval = value;
	const uint8_t cr = CR_WEN | CR_WR | addr;
	
	AD7799_CS_SELECT;
	set_spi_handling_task();
	while (!IS_AD7799_CS_SELECT);

	// (1) Configure DMA parameters for communication register transfer *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&cr);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(cr));

	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&tmp);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(tmp));
	// (2) Enable DMA transfer **************************************************
	start_SPI();
	
	// Wait to be notified of a DMA transfer complete interrupt.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );
	
	// (1) Configure DMA parameters for communication register transfer *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&pval);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(pval));

	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&tmp);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(tmp));
	// (2) Enable DMA transfer **************************************************
	start_SPI();
	
	// Wait to be notified of a DMA transfer complete interrupt.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );

	AD7799_CS_DESELECT;
	clear_spi_handling_task();
}
/*
void ad7799_cr(uint8_t on)
{
//	BaseType_t xResult;
	uint32_t ulNotifiedValue;
	
	uint8_t tmp;
	const uint8_t cr = CR_WEN | CR_RD | CR_ADDR_DR | on;
	
	AD7799_CS_SELECT;
	set_spi_handling_task();
	while (!IS_AD7799_CS_SELECT);

	// (1) Configure DMA parameters for communication register transfer *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&cr);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(cr));

	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&tmp);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(cr));
	// (2) Enable DMA transfer **************************************************
	start_SPI();
	
	// Wait to be notified of a DMA transfer complete interrupt.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );

	AD7799_CS_DESELECT;
	clear_spi_handling_task();
	
	
}
*/
double flt1000 = 0.0;
double flt10 = 0.0;
double flt0 = 0.0;
double th = 0.0;
double doze = 0.0;

void AD7799Flt(void *Param)
{
	double val = ad7799_rd<uint32_t>(CR_ADDR_DR);
	while (1)
	{
//		extern double speed;
		if (!IS_SM1_ENABLE) {
			flt1000 += (val - flt1000)/1000.0;
			flt10 += (val - flt10)/10.0;
			while (!IS_ADC_DATA_RDY) taskYIELD();
			val = ad7799_rd<uint32_t>(CR_ADDR_DR);
		} else taskYIELD();
	}
}


void AD7799Proc(void *Param)
{
	TaskHandle_t Flt_TaskHandle;
	
	ad7799_wr(CR_ADDR_MR, (MR_MODE_CONT | MR_PSW_OFF | MR_RATE_16HZ7));
	
	xTaskCreate(AD7799Flt, "", configMINIMAL_STACK_SIZE, 0, TASK_PRI_LED, &Flt_TaskHandle);
	
	while (1) {
		int32_t val = (int32_t)((flt10 - flt0)/41.280);
		if (RTC_Queue != NULL) xQueueSendToBack(AD7799_Queue, &val, 0);
		vTaskDelay(MS_TO_TICK(1000));
	}
}










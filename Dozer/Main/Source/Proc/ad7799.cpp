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




uint8_t ad7799_rd(uint8_t addr)
{
//	BaseType_t xResult;
	uint32_t ulNotifiedValue;
	
	static const uint8_t null_val = 0;
	uint8_t retval = 0;
	
	uint8_t cr = CR_WEN | CR_RD | addr;
	
	AD7799_CS_SELECT;
	set_spi_handling_task();
	while (!IS_AD7799_CS_SELECT);

	// (1) Configure DMA parameters for communication register transfer *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&cr);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(cr));

	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&retval);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(retval));
	// (2) Enable DMA transfer **************************************************
	
	start_SPI();
	// Wait to be notified of a DMA transfer complete interrupt.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );
	
	// (1) Configure DMA parameters for communication register transfer *******************
	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_5, (uint32_t)&null_val);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_5, sizeof(null_val));

	LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_6, (uint32_t)&retval);
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_6, sizeof(retval));
	// (2) Enable DMA transfer **************************************************
	
	start_SPI();
	// Wait to be notified of a DMA transfer complete interrupt.
	xTaskNotifyWait( pdFALSE, 0xffffffffUL, &ulNotifiedValue, portMAX_DELAY );

	AD7799_CS_DESELECT;
	clear_spi_handling_task();
	
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

void AD7799Proc(void *Param)
{
	
	while (1)
	{
		uint8_t val = ad7799_rd(CR_ADDR_ID);
		if (RTC_Queue != NULL) {
			xQueueSendToBack(AD7799_Queue, &val, 0);
		}
		vTaskDelay(MS_TO_TICK(100));
	}
}
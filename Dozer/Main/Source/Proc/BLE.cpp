#include "Global.h"

static	uint8_t outString[20];

void BLEProc(void *Param)
{
	InitBLEUart();

	while (1) {
		uint8_t bytes = BLEUartRx(10, outString);
		if (bytes) BLEUartTx(bytes, outString);
		vTaskDelay(MS_TO_TICK(100));
	}
}
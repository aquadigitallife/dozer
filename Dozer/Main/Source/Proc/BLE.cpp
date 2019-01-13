#include "Global.h"

void BLEProc(void *Param)
{
	static const char outString[] = "Hello, Dozer!\n";
	InitBLEUart();
	while (1) {
		BLEUartTx(14, (uint8_t*)outString);
		vTaskDelay(MS_TO_TICK(1000));
	}
}
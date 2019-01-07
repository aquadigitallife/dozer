#include "Global.h"

void LED_TaskProc(void *Param)
{
	UNUSED(Param);
	
	uint32_t WDG_Time = 0;
	ResetWatchdog(WDG_Time, WDG_TASK_LED, MS_TO_TICK(3000), 1, true);
/*	
	// Включем свю индикацию для теста
	LED_STATE_ON;
	LED_WORK_ON;
	LED_ERR_ON;
	LED_SD_BUSY_ON;
	LED_FLASH_BUSY_ON;
	LED_SM0_ON;
	LED_SM1_ON;
	
	vTaskDelay(MS_TO_TICK(2000));
	
	LED_STATE_OFF;
	LED_WORK_OFF;
	LED_ERR_OFF;
	LED_SD_BUSY_OFF;
	LED_FLASH_BUSY_OFF;
	LED_SM0_OFF;
	LED_SM1_OFF;
*/
	while (1)
	{
		ResetWatchdog(WDG_Time, WDG_TASK_LED, MS_TO_TICK(32), 2, true);
		
		if (Time < (ONE_SEC / 2))
			LED_STATE_ON;
		else
			LED_STATE_OFF;

		vTaskDelay(1);
	}
}

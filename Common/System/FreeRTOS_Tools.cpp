#include "Global.h"

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
extern "C" void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	UNUSED(xTask);
	UNUSED(pcTaskName);

	// Произошло переполнение стека
	volatile uint32_t Dummy = 0;
	(void)(Dummy);

	while (1);
}
#endif

#if (configUSE_IDLE_HOOK > 0)
extern "C" void vApplicationIdleHook()
{
	// Мигаем лампочкой, показывая что мы живы и не зависли
	if (Time < (ONE_SEC >> 1))
		LED_STATE_ON;
	else
		LED_STATE_OFF;
}
#endif

#if (configUSE_TICK_HOOK > 0)
extern "C" void vApplicationTickHook()
{
	Time += ONE_TICK;
	if (Time >= ONE_SEC)
		Time &= FRAC_SEC_MASK;

	AbsTime += ONE_TICK;
}
#endif

#ifdef DEBUG
uint32_t CheckHeap()
{
#if (configSUPPORT_DYNAMIC_ALLOCATION > 0 && configAPPLICATION_ALLOCATED_HEAP > 0)

	// Подсчёт свободного места в куче
	uint32_t *H32 = ((uint32_t*)(ucHeap));
	H32 += sizeof(ucHeap) / sizeof(uint32_t) - 1;

	uint32_t Res = 0;
	while (*H32 == 0xDEADBEEF)
	{
		Res++;
		H32--;
	}

	return Res * sizeof(uint32_t);
#else
	return 0;
#endif
}

void CheckStack(volatile uint32_t* const FreeCount)
{
#ifdef BOOTLOADER
	UNUSED(FreeCount);

#else
	// TODO проверить корректность для всех устройств, сделать ветку для пульта

#if (defined(IPIXEL_LITE) || defined(IPIXEL_TECH))
	if (USB_Task)
		FreeCount[0] = uxTaskGetStackHighWaterMark(USB_Task);

	if (FlashTask)
		FreeCount[1] = uxTaskGetStackHighWaterMark(FlashTask);

	if (ShiftTask)
		FreeCount[2] = uxTaskGetStackHighWaterMark(ShiftTask);

	if (MenuTask)
		FreeCount[3] = uxTaskGetStackHighWaterMark(MenuTask);

	if (RF_Task)
		FreeCount[4] = uxTaskGetStackHighWaterMark(RF_Task);

	if (WDG_Task)
		FreeCount[5] = uxTaskGetStackHighWaterMark(WDG_Task);
#elif defined(IPIXEL)
	if (ShiftTask)
		FreeCount[0] = uxTaskGetStackHighWaterMark(ShiftTask);

	if (FlashTask)
		FreeCount[1] = uxTaskGetStackHighWaterMark(FlashTask);

	if (MenuTask)
		FreeCount[2] = uxTaskGetStackHighWaterMark(MenuTask);

	if (RF_Task)
		FreeCount[3] = uxTaskGetStackHighWaterMark(RF_Task);

	if (GyroTask)
		FreeCount[4] = uxTaskGetStackHighWaterMark(GyroTask);

	if (USB_Task)
		FreeCount[5] = uxTaskGetStackHighWaterMark(USB_Task);

	if (WDG_Task)
		FreeCount[6] = uxTaskGetStackHighWaterMark(WDG_Task);

#elif IREMOTE
	if (ShiftTask)
		FreeCount[0] = uxTaskGetStackHighWaterMark(ShiftTask);

	if (MenuTask)
		FreeCount[1] = uxTaskGetStackHighWaterMark(MenuTask);

	if (RF_Task)
		FreeCount[2] = uxTaskGetStackHighWaterMark(RF_Task);

	if (GyroTask)
		FreeCount[3] = uxTaskGetStackHighWaterMark(GyroTask);

	if (USB_Task)
		FreeCount[4] = uxTaskGetStackHighWaterMark(USB_Task);

	if (WDG_Task)
		FreeCount[5] = uxTaskGetStackHighWaterMark(WDG_Task);

#ifdef DEV_IREMOTE_DMX
	if (DMX_Task)
		FreeCount[6] = uxTaskGetStackHighWaterMark(DMX_Task);
#endif

#else
	UNUSED(FreeCount);
#endif

#endif
}
#endif


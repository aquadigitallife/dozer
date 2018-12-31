#include "Global.h"

void ResetWatchdog(uint32_t &LastResetTime, const uint8_t TaskID, const uint32_t Time, const uint8_t Param, const bool Force /* = false*/)
{
	UNUSED(Param);
	
	if (LastResetTime < AbsTime || Force)
	{
		LastResetTime = AbsTime;

		STR_WDG_CMD WDG_Cmd = {TaskID, Time, 0};
#ifdef DEBUG_WDG
		WDG_Cmd.TaskParam = Param;
#endif
		xQueueSend(WDG_Cmd_Queue, &WDG_Cmd, 1);
	}
}

void WDG_TaskProc(void *Param)
{
	UNUSED(Param);
	
	WDG_Cmd_Queue = xQueueCreate((uint32_t)(WDG_TASK_COUNT * 3), sizeof(STR_WDG_CMD));
	configASSERT(WDG_Cmd_Queue);

	uint32_t TimeToReset[WDG_TASK_COUNT] = {};

#ifdef DEBUG_WDG
	const uint8_t MAX_PARAM = 20;

	uint32_t MinTime[WDG_TASK_COUNT][MAX_PARAM] = {};
	uint32_t TaskParam[WDG_TASK_COUNT] = {};
#endif

	uint8_t pReset = 0;

	STR_WDG_CMD Cmd = {};

	for (uint8_t i = 0; i < WDG_TASK_COUNT; i++)
	{
		TimeToReset[i] = 4; // Даём всем немножко времени для инициализации
#ifdef DEBUG_WDG
		for (uint8_t j = 0; j < MAX_PARAM; j++)
			MinTime[i][j] = portMAX_DELAY;
#endif
	}

#ifdef DEBUG_WDG
	uint32_t DataInQueue = 0;
#endif

	while (1)
	{
#ifdef DEBUG_WDG
		uint32_t cnt = uxQueueMessagesWaiting(WDG_Cmd_Queue);
		if (DataInQueue < cnt)
			DataInQueue = cnt;
#endif

		// Забираем информацию от задач
		while (xQueueReceive(WDG_Cmd_Queue, &Cmd, 0))
		{
			if (Cmd.TaskID >= WDG_TASK_COUNT || Cmd.TaskTime == 0
#ifdef DEBUG_WDG
				|| TaskParam[Cmd.TaskID] >= MAX_PARAM
#endif
				)
			{
#ifdef DEBUG_WDG
				volatile int _ = 0;
				UNUSED(_);
#else
				pReset = 2;
				NVIC_SystemReset();
#endif
			}

#ifdef DEBUG_WDG
			if (MinTime[Cmd.TaskID][TaskParam[Cmd.TaskID]] > TimeToReset[Cmd.TaskID])
				MinTime[Cmd.TaskID][TaskParam[Cmd.TaskID]] = TimeToReset[Cmd.TaskID];

			TaskParam[Cmd.TaskID]   = Cmd.TaskParam;
#endif
			TimeToReset[Cmd.TaskID] = Cmd.TaskTime;
		}

		// Уменьшаем время ожидания до сброса
		for (uint8_t i = 0; i < WDG_TASK_COUNT; i++)
		{
#ifdef DEBUG_WDG
			if (TimeToReset[i])
			{
				if (--TimeToReset[i] == 0)
				{
					volatile int _ = 0;
					UNUSED(_);
				}
			}

			if (TaskParam[i] < MAX_PARAM)
			{
				if (MinTime[i][TaskParam[i]] > TimeToReset[i])
					MinTime[i][TaskParam[i]] = TimeToReset[i];
			}
#else
			if (--TimeToReset[i] == 0)
				pReset = 1; // Счётчик дотикал, задача не отвечает. Сбрасываем
#endif
		}

#ifdef DEBUG_WDG
		IWDG_RESET();
		(void)pReset;
#else
		if (!pReset)
		{
			IWDG_RESET();
		}
		else
		{
			NVIC_SystemReset();
		}
#endif

		vTaskDelay(1);
	}
}

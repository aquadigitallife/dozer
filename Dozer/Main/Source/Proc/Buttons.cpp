/*
	Модуль обработки сигналов кнопок и выдачи команд соответствующим процессам
*/
#include "Global.h"
/* параметры очереди сообщений от кнопок */
#define BUTTON_QUEUE_LENGTH    5	// количество сообщений в очереди
#define BUTTON_ITEM_SIZE       sizeof( uint8_t )	// размер одного элемента очереди

//TaskHandle_t Motor1TaskHandle;	// переменная хранения дескриптора процесса двигателя крыльчатки

/*
	Функция проверки активности процессов крыльчатки и заслонки.
	Возвращает true если процессы не активны
*/
bool isMotorsSuspend(void)
{
	return ((eSuspended == eTaskGetState( Motor0CycleHandle )) 
		 && (motor1_on == 0)/*(eSuspended == eTaskGetState( Motor1TaskHandle ))*/);
}

/*
	Процесс опроса кнопок А, В, С и выдачи команд процессам
	управления двигателями.
	Кнопки A, B выдают сообщения пока нажаты, кнопка С выдаёт
	команду двигателям при изменении своего состояния из выкл. во вкл.
*/
void ButtonsProc(void *Param)
{
	TaskHandle_t Motor0TaskHandle;		// дескриптор процесса управления заслонкой
	
	QueueHandle_t SM0_Queue = xQueueCreate ( BUTTON_QUEUE_LENGTH, BUTTON_ITEM_SIZE );	// создаём очередь сообщений от кнопок

	configASSERT ( SM0_Queue );

	xTaskCreate(Motor0Proc, "" , configMINIMAL_STACK_SIZE + 400, SM0_Queue, TASK_PRI_LED, &Motor0TaskHandle);	// создаём процесс управления заслонкой
	xTaskCreate(Motor1Proc, "" , configMINIMAL_STACK_SIZE + 400, NULL, TASK_PRI_LED, &Motor1TaskHandle);	// создаём процесс управления крыльчаткой
	
	while (1) {
		static uint8_t pre_sm1 = 0;	// переменная предыдущего значения кнопки С.
		vTaskDelay(MS_TO_TICK(10));	// опрос происходит с дискретностью 10 мс.
scan_button:		
		switch MOTOR_BUTTONS {

			case SM0_TEST_A:	// если нажата A
				for (int i = 0;;) {				// подтверждаем нажатие несколько периодов (3) - антидребезговый алгоритм
					vTaskDelay(MS_TO_TICK(10));	// период антидребезга 3*10мс
					if (IS_SM0_TEST_A) i++; else goto scan_button;	// если нажатие не подтвердилось, возвращаемся к опросу
					if ((i > 2) && isMotorsSuspend()) {	// если нажатие подтвердилось и нет процесса рассеивания
						uint8_t mButton = 0x0A;			// формируем сообщение в очереди
						xQueueSendToBack(SM0_Queue, (void*)&mButton, 0);
						break;
					}
				}
				break;

			case SM0_TEST_B:	// если нажата B - всё аналогично A
				for (int i = 0;;) {
					vTaskDelay(MS_TO_TICK(10));
					if (IS_SM0_TEST_B) i++; else goto scan_button;
					if ((i > 2) && isMotorsSuspend()) {
						uint8_t mButton = 0x0B;
						xQueueSendToBack(SM0_Queue, (void*)&mButton, 0);
						break;
					}
				}
				break;

			case SM1_TEST:	// если нажата С
				for (int i = 0;;) {					// подтверждаем нажатие
					vTaskDelay(MS_TO_TICK(10));
					if (IS_SM1_TEST) i++; else goto scan_button;
					if ((i > 2) && (pre_sm1 == 0)) {	// если нажатие подтверждено, и предыдущее состояние = "кнопка не нажата"
						pre_sm1 = 0xFF;					// предыдущее состояние = "кнопка нажата"
						if (motor1_on == 0) {	// если процесс рассеяния выключен, даём команду на запуск и включаем индикацию
							taskENTER_CRITICAL();
							motor1_on = 0xFF; purge_on = 0xFF;
							taskEXIT_CRITICAL();
							LED_SM1_ON;
						} else {	// иначе выключаем процесс и сигнализацию
							LED_SM1_OFF;
							taskENTER_CRITICAL();
							motor1_on = 0; purge_on = 0;
							taskEXIT_CRITICAL();
						}
						break;
					}
				}
				break;

			default: {	// во всех остальных случаях
				uint8_t mButton = 0x0;
				if (isMotorsSuspend()) xQueueSendToBack(SM0_Queue, (void*)&mButton, 0);	// если нет рассеивания, выдаём сообщения об отсутствии нажатий кнопок А и В
				pre_sm1 = 0;	// кнопка С отжата
			}
		}
	}
}

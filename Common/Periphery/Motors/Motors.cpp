/*
Модуль управления двигателями крыльчатки и заслонки.
*/

#include "Global.h"

uint8_t motor1_on = 0;	// флаг-признак включения/выключения процесса рассеивания. 0 - выкл. FFh - вкл.
uint8_t purge_on = 0;	// совместно с motor1_on флаг-признак включения процесса полной выгрузки (рассеивание выключается оператором)

/* Функция задания скорости вращения для двигателя заслонки */
void SetSpeedSM0(uint16_t period)
{
	LL_TIM_OC_SetCompareCH1(TIM9, period/2);
	LL_TIM_SetAutoReload(TIM9, period);
}

/* Инициализация периферии процессора для управления двигателем заслонки */
/*
void InitSM0(uint16_t period)
{
	// Конфигурируем GPIOE.5 как выход таймера - генератора меандра для сигнала STEP контроллера ШД *
	LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_5, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinPull(GPIOE, LL_GPIO_PIN_5, LL_GPIO_PULL_DOWN);
	LL_GPIO_SetPinSpeed(GPIOE, LL_GPIO_PIN_5, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetAFPin_0_7(GPIOE, LL_GPIO_PIN_5, LL_GPIO_AF_3);

	//
	// Инициализируем таймер - генератор меандра *
	//
	// Режим счёта - инкремент *
	LL_TIM_SetCounterMode(TIM9, LL_TIM_COUNTERMODE_UP);
 
	// режим периодической генерации импульса *
	LL_TIM_SetOnePulseMode(TIM9, LL_TIM_ONEPULSEMODE_REPETITIVE);
  
	//  Задаём частоту тактирования таймера равной 10МГц  *
	LL_TIM_SetPrescaler(TIM9, __LL_TIM_CALC_PSC(SystemCoreClock, 10000000));
	//  Задаём коэфф. деления тактовой частоты, определяющий период следования импульсов *
	SetSpeedSM0(period);

	// Настраиваем 1-й канал таймера *
	LL_TIM_OC_SetMode(TIM9,  LL_TIM_CHANNEL_CH1,  LL_TIM_OCMODE_PWM2);

	LL_TIM_OC_ConfigOutput(TIM9, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH | LL_TIM_OCIDLESTATE_LOW);
  
  
	//
	// Финальные приготовления *
	//
	// Включаем 1-й канал *
	LL_TIM_CC_EnableChannel(TIM9, LL_TIM_CHANNEL_CH1);
  
	// Включаем выходы таймера *
	LL_TIM_EnableAllOutputs(TIM9);
  
	// Включаем автозагрузку регистра периода *
	LL_TIM_EnableARRPreload(TIM9);

	// И перезаписываем его *
	LL_TIM_GenerateEvent_UPDATE(TIM9);  
	
//	На всякий случай пробуждаем драйвер ШД
	SM0_WAKEUP;
}
*/

/* Функция включения движения заслонки
	если dir = 0, заслонка движется вниз, иначе вверх
 */
void StartSM0(bool dir)
{
	UNUSED(dir);
	PWR_SM0_EN;
	LED_SM0_ON;
/*
	if (dir) SM0_BACKWARD; else SM0_FORWARD;	// задаём направление вращения на контроллере ШД
	LED_SM0_ON;	// включаем сигнализацию о работе ШД заслонки
	SM0_ENABLE;	// включаем контроллер ШД
	LL_TIM_EnableCounter(TIM9);	// подаём меандр на вход step ШД
*/
}
/*
	Функция остановки движения заслонки
*/
void StopSM0(void)
{
	PWR_SM0_DIS;
	LED_SM0_OFF;
/*
	LL_TIM_DisableCounter(TIM9);	// прекращаем подачу меандра 
	SM0_DISABLE;					// отключаем контроллер ШД
	LED_SM0_OFF;					// гасим сигнализацию о работе ШД.
*/
}

/*
	Процесс ручной регулировки начального положения заслонки.
	Процесс принимает сообщения от кнопок А и В (процесс ButtonsProc)
	и по их значениям включает двигатель заслонки на вращение вверх
	или вниз. Param - указатель на очередь сообщений.
*/
void Motor0Proc(void *Param)
{
	QueueHandle_t SM0_Queue = (QueueHandle_t)Param;		// сохраняем указатель на очередь локально.
	uint8_t mButton;									// переменная для хранения значения нажатой кнопки
	
//	InitSM0(0x400);	// при 0x400 - 22 сек полный ход.	// инициализируем таймер вращения
	
	while (1) {
		if (uxQueueMessagesWaiting( SM0_Queue )) {	// если в очереди есть сообщения
			mButton = 0;
			xQueueReceive(SM0_Queue, &mButton, 0);	// читаем значение нажатой кнопки
			switch (mButton) {
				case 0x0A: StartSM0(1); break;		// если нажата кнопка A, двигаемся вверх
				case 0x0B: StopSM0();/*StartSM0(0);*/ break;		// если нажата кнопка B, двигаемся вниз
//				default: StopSM0();					// если ни одна не нажата, останавливаемся
			}
		}
	}	
}
/*
	Процесс управления заслонкой во время процесса рассеивания.
	Помимо управления заслонкой, процесс контроллирует оставшийся вес корма в кормушке,
	и если он меньше чем P - D + d, то останавливает процесс рассеивания (сбрасывает
	флаг motor1_on). P - вес корма перед началом рассеивания (переменная wgt_start),
	D - заданная доза рассеяния,
	d - эмпирическая поправка на инерционность математического фильтра показаний АЦП тензодатчика
*/
void Motor0Cycle(void *Param)
{
	double wgt_start;			// переменная для хранения веса корма перед рассеиванием

//	InitSM0(0x400);				// инициализируем двигатель заслонки
	
	while (1) {
//start:
		StopSM0();								// останавливаем двигатель
		vTaskSuspend( NULL );					// ожидаем включения крыльчатки (пробуждения от процесса Motor1Proc)
		for (int i = 0; i < 25; i++) {			// после включения крыльчатки ожидаем 25 секунд разгон крыльчатки
			vTaskDelay(MS_TO_TICK(1000));
			if (motor1_on == 0) break;
		}
		if (motor1_on != 0) {					// если к этому времени не выключили процесс кнопкой
			double d = 0;
			wgt_start = get_weight();			// фиксируем текущее показание датчика (P)
			StartSM0(1);						// включаем ротор
			while (motor1_on != 0) {			// пока не выключили рассеивание
				if (((wgt_start - get_doze() + d) > get_weight()) || get_weight() < 30)	// проверяем условие прекращения рассеивания
					if (purge_on == 0) { motor1_on = 0; break; }	// если нет режима опустошения, выключаем рассеивание
				vTaskDelay(MS_TO_TICK(100));
				d += (350 - d)/20;
			}
//			StopSM0();
		}
	}
}

/*-------------------Управление крыльчаткой--------------------*/
/*
	Функция задания скорости вращения двигателя крыльчатки.
	Всё аналогично SetSpeedSM0, только для TIM11
*/
void SetSpeedSM1(uint32_t period)
{
	LL_TIM_OC_SetCompareCH1(TIM11, period/2);
	LL_TIM_SetAutoReload(TIM11, period);
}
/*
	Функция инициализации контроллера ШД. Аналогично InitSM0
*/
void InitSM1(uint32_t period)
{
	/* GPIO TIM11_CH1 configuration */
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_9, LL_GPIO_PULL_DOWN);
	LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_9, LL_GPIO_AF_3);

	/*********************************/
	/* Output waveform configuration */
	/*********************************/
	/* Select counter mode: counting up */
	LL_TIM_SetCounterMode(TIM11, LL_TIM_COUNTERMODE_UP);
 
	/* Set the one pulse mode:  generate repetitive pulses*/
	LL_TIM_SetOnePulseMode(TIM11, LL_TIM_ONEPULSEMODE_REPETITIVE);
  
	/* Set the TIM11 prescaler to get counter clock frequency at 10 MHz           */
	/* In this example TIM11 input clock (TIM11CLK) is set to APB2 clock (PCLK?), */
	/* since APB2 pre-scaler is equal to ?.                                       */
	/*    TIM11CLK = ?PCLK?                                                       */
	/*    PCLK? = HCLK                                                            */
	/*    => TIM11CLK = SystemCoreClock (100 MHz)                                 */  
	LL_TIM_SetPrescaler(TIM11, __LL_TIM_CALC_PSC(SystemCoreClock, 10000000));
  
	SetSpeedSM1(period);

	/* Set output channel 1 in PWM2 mode */
	LL_TIM_OC_SetMode(TIM11,  LL_TIM_CHANNEL_CH1,  LL_TIM_OCMODE_PWM2);

	/* Configure output channel 1 */
	LL_TIM_OC_ConfigOutput(TIM11, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH | LL_TIM_OCIDLESTATE_LOW);
  
	/**************************/
	/* TIM11 interrupts set-up */
	/**************************/
	/* Enable the capture/compare interrupt for channel 1 */
	//  LL_TIM_EnableIT_CC1(TIM11);
  
	/**************************/
	/* Start pulse generation */
	/**************************/
	/* Enable channel 1 */
	LL_TIM_CC_EnableChannel(TIM11, LL_TIM_CHANNEL_CH1);
  
	/* Enable TIM11 outputs */
	LL_TIM_EnableAllOutputs(TIM11);
  
	/* Enable auto-reload register preload */
	LL_TIM_EnableARRPreload(TIM11);

	/* Force update generation */
	LL_TIM_GenerateEvent_UPDATE(TIM11);  
}

/*
	Функция включения двигателя крыльчатки
	аналогично StartSM0
*/
void StartSM1(bool dir)
{
	if (dir) SM1_BACKWARD; else SM1_FORWARD;
	SM1_ENABLE;
	LL_TIM_EnableCounter(TIM11);
//	LED_SM1_ON;		// за исключением включения индикации.
}
/*
	Функция выключения двигателя крыльчатки
	аналогично StopSM1
*/
void StopSM1(void)
{
	LL_TIM_DisableCounter(TIM11);
	SM1_DISABLE;
//	LED_SM1_OFF;	// за исключением управления индикацией
}

double speed = 65535.0;	// переменная для хранения делителя задания скорости вращения крыльчатки

/* константы работы крыльчатки по синусоидальному закону */
#define SPEED_MIN	251.0//1024.0							// максимальное значение делителя (минимальная скорость)
#define SPEED_MID	((SPEED_MAX + SPEED_MIN)/2.0)	// средняя скорость
#define SPEED_MAX	250.0//195.0							// минимальное значение делителя (максимальная скорость)
#define SPEED_AMP	((SPEED_MIN - SPEED_MAX)/2.0)	// амплитуда колебаний скорости

/*
	Процесс рассеивания.
	Процесс запускается если motor1_on == FFh и если: доза не равна 0 или включен режим опустошения
	
*/
void Motor1Proc(void *Param)
{
	/* создаём процесс управления заслонкой */
	xTaskCreate(Motor0Cycle, "" , configMINIMAL_STACK_SIZE + 400, &motor1_on, TASK_PRI_LED, &Motor0CycleHandle);
	
	InitSM1(65535);	// инициализируем контроллер ШД крыльчатки
	while (1) {
		double sin_var, pre_sin_var;						// переменные для генерации чисел по синусоидальному закону
		speed = 65535.0;									// начинаем с минимальной скорости
		StopSM1();											// выключаем движение на минимальной скорости
		while (motor1_on == 0) vTaskSuspend( NULL );		// ожидаем команды на включение
		if (get_doze() == 0.0 && purge_on == 0) continue;	// если доза не задана и нет режима опустошения, возвращаемся к ожиданию
		vTaskResume( Motor0CycleHandle );					// запускаем процесс управления заслонкой
		StartSM1(1); 										// включаем движение крыльчатки на минимальной скорости

		for (;;) {											// цикл наращивания скорости по экспоненте
			double d;										// переменная приращения экспоненты
			SetSpeedSM1((uint32_t)speed);					// устанавливаем новое значение скорости
			vTaskDelay(MS_TO_TICK(10));						// с периодичностью 10 мс
			if (speed > SPEED_MID) d = (speed - SPEED_MAX)/64; else break;	// пока не достигнем средней синусоидальной скорости, вычисляем приращение скорости
			speed = speed - d;								// и увеличиваем её
		}
		
/* Достигли средней синусоидальной. Переходим к работе по синусу
          pre_sin_var = sin(2*pi*vTaskDelay(ms)/T(ms))	T = 12000 ms		*/
		sin_var = 0.0; pre_sin_var = 0.00523596383141958009; // Начальные значения для генератора синуса, определяющие
															 // начальную фазу
	
		while (1) {
			double si;	// вспомогательная переменная
/*			Генерация синуса происходит по следующей итерационной формуле:
            sin_var = K*sin_var - pre_sin_var. K = 2*cos(2*pi*vTaskDelay(ms)/T(ms))		T = 12000 ms - период синуса
			нижние три строчки - реализация данной формулы
*/
			si = sin_var;
			sin_var = 1.99997258449485358538*sin_var - pre_sin_var; 
			pre_sin_var = si;

			SetSpeedSM1((uint32_t)(SPEED_MID + sin_var*SPEED_AMP));	// Задаём новое значение скорости ШД
			vTaskDelay(MS_TO_TICK(10));								// с дискретностью 10 мс.
			if (motor1_on == 0) {	 break;								// если пришла команда на завершение
//				if (sin_var < 0.0009 && sin_var > -0.0009) {LED_ERR_ON; if (pre_sin_var < sin_var) break;} // если скорость равна средней
			}																				// и находится в фазе убывания
		}
		for (;;) {														// то переходим к циклу торможения
			double d;													// приращение убывания скорости
			SetSpeedSM1((uint32_t)speed);								// задаём новое значение скорости
			vTaskDelay(MS_TO_TICK(10));									// с дискретностью 10 мс.
			if (speed >= 512.0) d = speed/64.0; else d = speed/256.0;	// если делитель скорости больше 512, то увеличиваем темп замедления
			speed = speed + d;											// уменьшаем скорость
			if (speed >= 10000.0) { /*speed = 1024.0;*/ break; }		// если скорость близка к минимальной, останавливаем двигатель
		}
	}
}


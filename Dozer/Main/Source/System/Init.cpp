#include "Global.h"

static inline void Init_Power()
{
	// Настройка питания

	// Тактирование для этого модуля включаем здесь
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;

	// Включаем супервизор питания
	PWR->CR |= PWR_CR_PVDE | PWR_CR_PLS_LEV7; // 2.9V
	// Он установит флаг и мы сможем узнать о просадке питания

	// Переключаем регулятор питания в 1 режим
	// (для работы на высокой частоте и при напряжении питания больше 2.4В)
	PWR->CR |= PWR_CR_VOS;
}

static inline void Init_Clocks()
{
	// Базовая настройка тактирования
	InitRCC();

	// TODO Настройка PLL I2S

	// Включаем тактирование периферии
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
	                RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN |
	                RCC_AHB1ENR_GPIOHEN | RCC_AHB1ENR_GPIOFEN | RCC_AHB1ENR_GPIOGEN |
	                RCC_AHB1ENR_DMA1EN  | RCC_AHB1ENR_DMA2EN  |
	                0;

//	RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

//	RCC->APB1ENR |= //RCC_APB1ENR_WWDGEN |
//	                RCC_APB1ENR_SPI2EN | RCC_APB1ENR_SPI3EN |
//	                RCC_APB1ENR_TIM2EN;

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN |
	                RCC_APB2ENR_SPI1EN   |
	                RCC_APB2ENR_TIM9EN   | RCC_APB2ENR_TIM10EN | RCC_APB2ENR_TIM11EN  |
	                RCC_APB2ENR_ADC1EN   | 0;

	// Для отладки
#ifdef DEBUG
	RCC->CFGR &= ~(RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE | RCC_CFGR_MCO2 | RCC_CFGR_MCO2PRE);
	RCC->CFGR |=  (6 << 24) | (3 << 21);
	//RCC->CFGR |=  (0 << 24) | (2 << 21);
#endif
}

static inline void Init_Val()
{
	// Инициализация переменных

	Time = 0;
	AbsTime = 0;

#if (configSUPPORT_DYNAMIC_ALLOCATION > 0 && configAPPLICATION_ALLOCATED_HEAP > 0)
	memset(ucHeap, 0, sizeof(ucHeap));

#ifdef DEBUG
	uint32_t *Heap = (uint32_t*)(ucHeap);
	for (uint32_t i = 0; i < sizeof(ucHeap) / sizeof(uint32_t); i++)
	{
		Heap[i] = 0xDEADBEEF;
	}
#endif // DEBUG
#endif // Alloc heap
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void Init_RTOS()
{
	// Текстовые метки задач
	const char cpTASK[8][configMAX_TASK_NAME_LEN] = {
#ifdef DEBUG // Что бы в прошивке не было этих строк
			"LED",
			"FLASH",
			"BUTTONS",
			"",
			"",
			""
			"WDG"
#endif
	};

	// Создаём задачи
	xTaskCreate(LED_TaskProc  , cpTASK[0] , configMINIMAL_STACK_SIZE      , 0, TASK_PRI_LED  , &LED_TaskHandle);
	xTaskCreate(FlashProc , cpTASK[1] , configMINIMAL_STACK_SIZE + 400, 0, TASK_PRI_FLASH, &FlashTaskHandle);
	xTaskCreate(ButtonsProc, cpTASK[2] , configMINIMAL_STACK_SIZE, 0, TASK_PRI_LED, &ButtonsTaskHandle);
	xTaskCreate(BLEProc, cpTASK[3] , configMINIMAL_STACK_SIZE + 1024, 0, TASK_PRI_LED, &BLETaskHandle);
//	xTaskCreate(MenuTaskProc  , cpTASK[2] , configMINIMAL_STACK_SIZE + 200, 0, TASK_PRI_MENU , &MenuTaskHandle);
//	xTaskCreate(RF_TaskProc   , cpTASK[3] , configMINIMAL_STACK_SIZE + 350, 0, TASK_PRI_RF   , &RF_TaskHandle);
//	xTaskCreate(GyroTaskProc  , cpTASK[4] , configMINIMAL_STACK_SIZE      , 0, TASK_PRI_GYRO , &GyroTaskHandle);
//	xTaskCreate(USB_TaskProc  , cpTASK[5] , configMINIMAL_STACK_SIZE + 500, 0, TASK_PRI_USB  , &USB_TaskHandle);
	xTaskCreate(WDG_TaskProc  , cpTASK[6] , configMINIMAL_STACK_SIZE + 100, 0, TASK_PRI_WDG  , &WDG_TaskHandle);

	configASSERT(LED_TaskHandle);
	configASSERT(WDG_TaskHandle);

	Config_Queue    = xQueueCreate(1, sizeof(STR_CONFIG));

	FlashExist_Sem = xSemaphoreCreateBinary();

	configASSERT(Config_Queue   );

	configASSERT(FlashExist_Sem );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void Init_WatchDogs()
{
#ifndef DISABLE_WDG
	// Настройка сторожевой собаки
	IWDG->KR  = 0x5555; // Разрешаем доступ к регистрам
	IWDG->PR  = 3;      // /32, 4096 мс до сброса (из-за долгого стирания памяти)
	IWDG->RLR = 2500;   // Через счётный регистр устанавливаем 2.5с до сброса.

	IWDG_RESET();
	IWDG->KR = 0xCCCC; // Запускаем
#endif
}

static inline void Init_GPIO()
{
	// Настраиваем руки/ноги
	LL_GPIO_InitTypeDef port = {};
	LL_GPIO_StructInit(&port);

	// Настраиваем все ноги на вход без подтяжки
	port.Pin = LL_GPIO_PIN_ALL;
	port.Mode = LL_GPIO_MODE_ANALOG;
	port.Pull = LL_GPIO_PULL_NO;

	LL_GPIO_Init(GPIOB, &port);
	LL_GPIO_Init(GPIOC, &port);
	LL_GPIO_Init(GPIOD, &port);
	LL_GPIO_Init(GPIOE, &port);
	LL_GPIO_Init(GPIOH, &port);
	LL_GPIO_Init(GPIOF, &port);
	LL_GPIO_Init(GPIOG, &port);

#ifdef ENABLE_SWD
	// У порта А не трогаем 13 и 14 пины, отвечающие за отладку
	port.Pin = LL_GPIO_PIN_ALL & ~(LL_GPIO_PIN_13 | LL_GPIO_PIN_14);
#endif
	LL_GPIO_Init(GPIOA, &port);

	// Гасим лампочки
	LED_STATE_OFF;
	LED_WORK_OFF;
	LED_ERR_OFF;
	LED_SD_BUSY_OFF;
	LED_FLASH_BUSY_OFF;
	LED_SM0_OFF;
	LED_SM1_OFF;

	// Переводим ноги в безопасное состояние
	FLASH_DESELECT;
	FLASH2_DESELECT0;
	FLASH2_DESELECT1;
	
	PWR_SM0_EN;
	SM0_WAKEUP;
	SM0_DISABLE;
	SM0_FORWARD;
	
	SM1_DISABLE;
	SM1_FORWARD;
	
	// Светодиоды индикации
	// PB2  - пульс
	// PF11 - работа
	// PF12 - ошибка работы
	// PG7  - испольование карты памяти
	// PG8  - испольование микросхемы памяти
	// PF9  - активность ШД0
	// PF8  - активность ШД1
	
	// PB2 - пульс
	LL_GPIO_StructInit(&port);
	port.Pin   = LL_GPIO_PIN_2;
	port.Mode  = LL_GPIO_MODE_OUTPUT;
	port.OutputType  = LL_GPIO_OUTPUT_PUSHPULL;
	port.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(GPIOB, &port);
	// DO_CTRL1,2
	port.Pin   = LL_GPIO_PIN_14 | LL_GPIO_PIN_15;
	LL_GPIO_Init(GPIOC, &port);
	// PF0...PF3 - оптовыходы (DO_CTRL3...6)
	// PF11 - работа
	// PF12 - ошибка работы
	port.Pin   = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
	LL_GPIO_Init(GPIOF, &port);
	// PG7 - испольование карты памяти
	// PG8 - испольование микросхемы памяти
	port.Pin   = LL_GPIO_PIN_7 | LL_GPIO_PIN_8;
	LL_GPIO_Init(GPIOG, &port);
	// PF9 - активность ШД0
	// PF8 - активность ШД1
	port.Pin   = LL_GPIO_PIN_4 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
	LL_GPIO_Init(GPIOF, &port);
	//Управление ШД
	port.Pin   = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_6;
	LL_GPIO_Init(GPIOE, &port);

	port.Mode = LL_GPIO_MODE_INPUT;
	port.Pull = LL_GPIO_PULL_UP;
	port.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_15;
	//DI_CTRL1
	LL_GPIO_Init(GPIOA, &port);
	//DI_CTRL2
	port.Pin   = LL_GPIO_PIN_5;
	LL_GPIO_Init(GPIOD, &port);
	//DI_CTRL3
	port.Pin   = LL_GPIO_PIN_7;	// тампер заслонки
	LL_GPIO_Init(GPIOB, &port);
	
#ifdef DEBUG
	// Для отладки
	// PA8 - MCO1
//	GPIO_StructInit(&port);
//	port.Pin       = GPIO_PIN_8;
//	port.Mode      = GPIO_MODE_AF_PP;
//	port.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
//	port.Alternate = GPIO_AF0_MCO;
//	GPIO_Init(GPIOA, &port);
//	// PC9 - MCO2
//	port.Pin       = GPIO_PIN_9;
//	GPIO_Init(GPIOC, &port);
#endif
}

static inline void Init_Timers()
{
	// Настройка таймеров
	
	// Системный таймер иницализируется RTOS
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline void Init_NVIC()
{
	// Настройка разрешений и приоритетов прерываний

	// Настройка групп приоритетов для корректной работы FreeTROS
	// 16 групп без подгрупп
	NVIC_SetPriorityGrouping(3);



	// Разрешаем все прерывания
	__enable_irq();
	__enable_fault_irq();

	// Выводим системные прерывания на свои обработчики
	// (иначе все буду ссылаться на HardFault)
	SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk |
	              SCB_SHCSR_MEMFAULTENA_Msk |
	              SCB_SHCSR_USGFAULTENA_Msk;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Init()
{
	// Настройка питания
	Init_Power();
	// Настаиваем тактовую частоту и включаем тактирование периферии
	Init_Clocks();
	// Инициализируем переменные
	Init_Val();

	Init_GPIO();
	// Настройка задач, очередей и прочего
	Init_RTOS();

	// Настройка периферии
	Init_WatchDogs();
	Init_Timers();

	// Настраиваем и разрешаем прерывания
	Init_NVIC();
}

extern "C" {
	void led_err_on(void);
	void led_err_off(void);
}
void led_err_on(void)
{
	LED_ERR_ON;
}

void led_err_off(void)
{
	LED_ERR_OFF;
}

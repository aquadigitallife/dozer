
// Базовые настройки тактирования
	#define HSE_EXIST                // Есть внешний кварц и тактирование происходить от него
//	#define HSI_OFF                  // Выключть HSI, если HSE запустился

#ifndef HSI_VALUE
	#define HSI_VALUE 16000000UL     // Частота внутренней RC цепочки, Гц
#endif

#ifndef HSE_VALUE
	#define HSE_VALUE 25000000UL     // Частота кварца, Гц
#endif

#ifndef HSE_STARTUP_TIMEOUT
	#define HSE_STARTUP_TIMEOUT 1500UL
#endif

///////////////////////////////////////////////////////////////////////////////
// Определение частот работы ядра и шин периферии

#ifdef STM32F437xx

#define F_HSE             HSE_VALUE
#define F_HSI             HSI_VALUE

// Опорная частота
#define F_REF_HSE         F_HSE
#define F_REF_HSI         F_HSI

#ifdef HSE_EXIST
	#define F_REF         F_REF_HSE
	#define MAIN_PLL_M    25UL  // /M
	#define MAIN_PLL_N    360UL // *N
	#define MAIN_PLL_P    2UL   // /P
	#define MAIN_PLL_Q    8UL   // /Q
#else
	#define F_REF         F_REF_HSI
	#define MAIN_PLL_M    16UL  // /M
	#define MAIN_PLL_N    360UL // *N
	#define MAIN_PLL_P    2UL   // /P
	#define MAIN_PLL_Q    8UL   // /Q
#endif

// Промежуточные частоты ФАПЧ
#define F_MAIN_PLL_M (F_REF        / MAIN_PLL_M)
#define F_MAIN_PLL_N (F_MAIN_PLL_M * MAIN_PLL_N)
#define F_MAIN_PLL_P (F_MAIN_PLL_N / MAIN_PLL_P)
#define F_MAIN_PLL_Q (F_MAIN_PLL_N / MAIN_PLL_Q)

#define F_SYS      (F_MAIN_PLL_P)    // Частота на выходе из PLL
#define F_AHB      (F_SYS / 1)       // Частоты шин периферии
#define F_APB1     (F_AHB / 4)       //
#define F_APB2     (F_AHB / 2)       //
#define F_TIM_APB1 (F_APB1 * 2)      // Частоты тактирования таймеров
#define F_TIM_APB2 (F_APB2 * 2)      //
#define F_SYS48    (F_MAIN_PLL_Q)    // Частота на 48МГц шине для USB

#else
#error Unknow MCU!
#endif

///////////////////////////////////////////////////////////////////////////////
#define ONE_SEC          256UL // Одна секунда
#define FRAC_SEC_MASK    (ONE_SEC-1UL)
// Частота системного таймера
#define F_SYS_TICK       128UL
#define ONE_TICK         (ONE_SEC / F_SYS_TICK) // Один тик

// Перевод миллисекунд в секунды и обратно
#define MS_TO_SEC(X)   ((X) * ONE_SEC / 1000UL)
#define SEC_TO_MS(X)   ((X) * 1000UL / ONE_SEC)
// Перевод тиков в секунды и обратно
#define MS_TO_TICK(X)  ((X) * F_SYS_TICK / 1000UL)
#define TICK_TO_MS(X)  ((X) * 1000UL / F_SYS_TICK)

///////////////////////////////////////////////////////////////////////////////

uint32_t InitRCC();

void Init();

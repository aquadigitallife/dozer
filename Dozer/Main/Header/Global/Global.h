#ifdef GLOBAL_EXPORT
	#define GL_EXP
#else
	#define GL_EXP extern
#endif

#ifdef DEBUG

//#define DISABLE_WDG
//#define DEBUG_WDG
#define ENABLE_SWD
#define ENABLE_SAVE_PARAM

//#define DEBUG_TRACE // Использование разъёма для радиомодуля и некоторых контактов для анализа работы
#endif

#include "stdlib.h"
#include "stdio.h"
#include "stddef.h"
#include "string.h"
#include "math.h"

// Макросы, инициализация ядра и периферии
#include "Macros.h"
#include "Init.h"

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "Proc.h"

// CMSIS
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"

// Периферия
#include "EFlash.h"

// Устройства
#include "S25FL.h"
#include "W25Q.h"

// Общие определения
#include "Common_Const.h"
#include "Common_Types.h"

//#include "Messages.h"

// Общая библиотека проекта
#define USE_CRC16_TABLE
#include "CRC.h"

#include "Global_Const.h"
#include "Global_Types.h"

#include "Global_Var.h"

#include "Settings.h"

// Переход в режим загрузчика
void GoToBootloader();

#ifdef DEBUG
uint32_t CheckHeap();
void CheckStack(volatile uint32_t* const FreeCount);
#endif

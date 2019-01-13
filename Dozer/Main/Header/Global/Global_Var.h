
// Системное
GL_EXP uint16_t Time;    // Время, сбрасывается каждую секунду, секунды, масштаб 2^-8
GL_EXP uint32_t AbsTime; // Время с момена включения, секунды, масштаб 2^-8
GL_EXP uint32_t Time250; // Счётчик 250мкс тиков

GL_EXP uint32_t* BootStatus; // В какой режим нужно войти при включении. Адрес переменной - 0x20000000
GL_EXP volatile STR_FW_HEAD *FW_Info; // Заголовок прошивки
GL_EXP volatile STR_BL_DATA *BL_Info; // Информация о загрузчике

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS
#if (configSUPPORT_DYNAMIC_ALLOCATION > 0 && configAPPLICATION_ALLOCATED_HEAP > 0)
GL_EXP uint8_t ucHeap[configTOTAL_HEAP_SIZE]; // Куча
#endif

// Ручки задач
GL_EXP TaskHandle_t LED_TaskHandle;
GL_EXP TaskHandle_t FlashTaskHandle;
GL_EXP TaskHandle_t ButtonsTaskHandle;
GL_EXP TaskHandle_t Motor0CycleHandle;
GL_EXP TaskHandle_t BLETaskHandle;
GL_EXP TaskHandle_t WDG_TaskHandle;

GL_EXP QueueHandle_t FlashCmd_Queue; // Очередь команд к Flash
GL_EXP QueueHandle_t WDG_Cmd_Queue;  // Очередь команд к сторожевой собаке

GL_EXP QueueHandle_t BatInfo_Queue;  // Очередь с информацией о аккумуляторе
GL_EXP QueueHandle_t Config_Queue;   // Очередь с настройками и основной конфигурацией устройства

GL_EXP SemaphoreHandle_t FlashExist_Sem; // Сигнал о корректной инициализации flash памяти

///////////////////////////////////////////////////////////////////////////////

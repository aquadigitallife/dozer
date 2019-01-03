
///////////////////////////////////////////////////////////////////////////////
// Системное
const uint32_t FW_VER = 0x03000600UL;

// Сброс собаки
#ifndef DISABLE_WDG
#define IWDG_RESET() do {IWDG->KR = 0xAAAA;} while (0)
#else
#define IWDG_RESET()
#endif

///////////////////////////////////////////////////////////////////////////////
// Индикация
#define LED_STATE_ON          do {GPIOB->BSRR = GPIO_BSRR_BS_2;} while (0)
#define LED_STATE_OFF         do {GPIOB->BSRR = GPIO_BSRR_BR_2;} while (0)

#define LED_WORK_ON           do {GPIOF->BSRR = GPIO_BSRR_BS_11;} while (0)
#define LED_WORK_OFF          do {GPIOF->BSRR = GPIO_BSRR_BR_11;} while (0)

#define LED_ERR_ON            do {GPIOF->BSRR = GPIO_BSRR_BS_12;} while (0)
#define LED_ERR_OFF           do {GPIOF->BSRR = GPIO_BSRR_BR_12;} while (0)

#define LED_SD_BUSY_ON        do {GPIOG->BSRR = GPIO_BSRR_BS_7;} while (0)
#define LED_SD_BUSY_OFF       do {GPIOG->BSRR = GPIO_BSRR_BR_7;} while (0)

#define LED_FLASH_BUSY_ON     do {GPIOG->BSRR = GPIO_BSRR_BS_8;} while (0)
#define LED_FLASH_BUSY_OFF    do {GPIOG->BSRR = GPIO_BSRR_BR_8;} while (0)

#define LED_SM0_ON            do {GPIOF->BSRR = GPIO_BSRR_BS_9;} while (0)
#define LED_SM0_OFF           do {GPIOF->BSRR = GPIO_BSRR_BR_9;} while (0)

#define LED_SM1_ON            do {GPIOF->BSRR = GPIO_BSRR_BS_8;} while (0)
#define LED_SM1_OFF           do {GPIOF->BSRR = GPIO_BSRR_BR_8;} while (0)

// SPI памяти
#define FLASH_CS1_UP          do {GPIOD->BSRR = GPIO_BSRR_BS_0;} while (0)
#define FLASH_CS1_DN          do {GPIOD->BSRR = GPIO_BSRR_BR_0;} while (0)

#define FLASH_CS2_UP          do {GPIOD->BSRR = GPIO_BSRR_BS_1;} while (0)
#define FLASH_CS2_DN          do {GPIOD->BSRR = GPIO_BSRR_BR_1;} while (0)

#define FLASH_CS3_UP          do {GPIOD->BSRR = GPIO_BSRR_BS_2;} while (0)
#define FLASH_CS3_DN          do {GPIOD->BSRR = GPIO_BSRR_BR_2;} while (0)

#define FLASH_SELECT          FLASH_CS1_DN
#define FLASH_DESELECT        FLASH_CS1_UP

#define FLASH2_SELECT0        FLASH_CS2_DN
#define FLASH2_DESELECT0      FLASH_CS2_UP

#define FLASH2_SELECT1        FLASH_CS2_DN
#define FLASH2_DESELECT1      FLASH_CS2_UP

// Дискретные входы
#define POWER_GOOD            ((GPIOA->IDR & (1 <<  9)) == 0)

// Кнопки
#define BUT_SM0_TEST_A        ((GPIOA->IDR & (1 <<  6)) == 0)
#define BUT_SM0_TEST_B        ((GPIOA->IDR & (1 <<  4)) == 0)
#define BUT_SM1_TEST          ((GPIOA->IDR & (1 <<  5)) == 0)
#define BUT_RESET             ((GPIOE->IDR & (1 <<  9)) == 0)
#define BUT_TEST              ((GPIOE->IDR & (1 << 10)) == 0)

// ШД
#define SM0_ENABLE            do {GPIOE->BSRR = GPIO_BSRR_BS_6;} while (0)
#define SM0_DISABLE           do {GPIOE->BSRR = GPIO_BSRR_BR_6;} while (0)
#define SM0_FORWARD           do {GPIOE->BSRR = GPIO_BSRR_BR_4;} while (0)
#define SM0_BACKWARD          do {GPIOE->BSRR = GPIO_BSRR_BR_4;} while (0)

#define SM1_ENABLE            do {GPIOE->BSRR = GPIO_BSRR_BS_1;} while (0)
#define SM1_DISABLE           do {GPIOE->BSRR = GPIO_BSRR_BR_1;} while (0)
#define SM1_FORWARD           do {GPIOE->BSRR = GPIO_BSRR_BR_0;} while (0)
#define SM1_BACKWARD          do {GPIOE->BSRR = GPIO_BSRR_BR_0;} while (0)


#ifdef DEBUG_TRACE

#define DBG_TR0_UP                      do { GPIOB->BSRR = GPIO_BSRR_BS3; } while(0)
#define DBG_TR0_DN                      do { GPIOB->BSRR = GPIO_BSRR_BR3; } while(0)

#define DBG_TR1_UP                      do { GPIOB->BSRR = GPIO_BSRR_BS5; } while(0)
#define DBG_TR1_DN                      do { GPIOB->BSRR = GPIO_BSRR_BR5; } while(0)

#define DBG_TR2_UP                      do { GPIOB->BSRR = GPIO_BSRR_BS4; } while(0)
#define DBG_TR2_DN                      do { GPIOB->BSRR = GPIO_BSRR_BR4; } while(0)

#define DBG_TR3_UP                      do { GPIOA->BSRR = GPIO_BSRR_BS15; } while(0)
#define DBG_TR3_DN                      do { GPIOA->BSRR = GPIO_BSRR_BR15; } while(0)

#define DBG_TR4_UP                      do { GPIOB->BSRR = GPIO_BSRR_BS6; } while(0)
#define DBG_TR4_DN                      do { GPIOB->BSRR = GPIO_BSRR_BR6; } while(0)

#define DBG_TR5_UP                      do { GPIOB->BSRR = GPIO_BSRR_BS7; } while(0)
#define DBG_TR5_DN                      do { GPIOB->BSRR = GPIO_BSRR_BR7; } while(0)

#define DBG_TR6_UP                      do { GPIOB->BSRR = GPIO_BSRR_BS8; } while(0)
#define DBG_TR6_DN                      do { GPIOB->BSRR = GPIO_BSRR_BR8; } while(0)

#define DBG_TR7_UP                      do { GPIOB->BSRR = GPIO_BSRR_BS12; } while(0)
#define DBG_TR7_DN                      do { GPIOB->BSRR = GPIO_BSRR_BR12; } while(0)

#else

#define DBG_TR0_UP
#define DBG_TR0_DN

#define DBG_TR1_UP
#define DBG_TR1_DN

#define DBG_TR2_UP
#define DBG_TR2_DN

#define DBG_TR3_UP
#define DBG_TR3_DN

#define DBG_TR4_UP
#define DBG_TR4_DN

#define DBG_TR5_UP
#define DBG_TR5_DN

#define DBG_TR6_UP
#define DBG_TR6_DN

#define DBG_TR7_UP
#define DBG_TR7_DN

#endif

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS
// Приоритеты задач (меньше - ниже)
const uint8_t TASK_PRI_IDLE    = 0;
const uint8_t TASK_PRI_LED     = 0; // Управление индикацией
const uint8_t TASK_PRI_WDG     = configMAX_PRIORITIES-1; // Отслеживание работы задач и сброс сторожевого таймера

// ID задач для задачи сброса сторожевой собаки
const uint8_t WDG_TASK_LED   = 0;
const uint8_t WDG_TASK_COUNT = 1;

///////////////////////////////////////////////////////////////////////////////
// Флешка

// Команды для задачи обслуживания памяти
enum class FLASH_CMD : uint8_t {NOP = 0, READ_PAGE, WRITE_PAGE, VERIFY_PAGE, READ_DATA, SECTOR_ERASE};

const uint32_t FLASH_BUF_HEAD_SIZE = 5; // Объём заголовка (1 байт - команда, 4 - адрес)
const uint32_t SECTOR_SIZE         = 256 * 1024; // размер сектора (256 Кб)
const uint32_t PAGE_SIZE           = 512; // размер страницы (512 б)

///////////////////////////////////////////////////////////////////////////////
// Периферия

// Делители для SPI
const uint32_t SPI_DIV_256 = SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0;
const uint32_t SPI_DIV_128 = SPI_CR1_BR_2 | SPI_CR1_BR_1               ;
const uint32_t SPI_DIV_64  = SPI_CR1_BR_2 |                SPI_CR1_BR_0;
const uint32_t SPI_DIV_32  = SPI_CR1_BR_2                              ;
const uint32_t SPI_DIV_16  =                SPI_CR1_BR_1 | SPI_CR1_BR_0;
const uint32_t SPI_DIV_8   =                SPI_CR1_BR_1               ;
const uint32_t SPI_DIV_4   =                               SPI_CR1_BR_0;
const uint32_t SPI_DIV_2   =                                          0;

///////////////////////////////////////////////////////////////////////////////
// Кнопки
// Состояние кнопки
enum class BUT_STATE : uint8_t {btNOTPRESS = 0, btSHORT, btLONG};
// btNOTPRESS - Кнопка не нажата
// btSHORT    - Короткое нажатие
// btLONG     - Длинное нажатие

// События от кнопки
enum class BUT_EVENT : uint8_t {beNONE = 0, beSHORT, beLONG, beHOLD, beRELEASE, beRELEASE_LONG};
// beNONE         - События нет
// beSHORT        - Произошло короткое нажатие
// beLONG         - Произошло длинное нажатие
// beHOLD         - Кнопку удерживают после длительного нажатия, событие периодическое
// beRELEASE      - Кнопка отпущена после короткого нажатия
// beRELEASE_LONG - Кнопка отпущена после длинного нажатия

///////////////////////////////////////////////////////////////////////////////

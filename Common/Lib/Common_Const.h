
///////////////////////////////////////////////////////////////////////////////
// Общие константы

// Для загрузика и получения данных из основной прошивки
const uint32_t BOOTLOADER_START = 0x08000000; // Начало первого сектора

#if defined(STM32F437xx)
const uint32_t MAIN_FW_START    = 0x08020000; // Начало 5-го сектора
const uint32_t MAIN_FW_SIZE     = (128 * 3) * 1024; // Прошивка лежит с 5-го по 7-ой сектор
#else

#error Unknow MCU!

#endif

const uint32_t BOOT_TO_MAIN      = 0x3C5AF00F;    // Метка, что нужно перейти к основной прошивке
const uint32_t BOOT_TO_LOADER    = ~BOOT_TO_MAIN; // Метка, что нужно перейти к загрузчику

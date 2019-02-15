// Адреса часов реального времени
#define RTC_WRITE	0xD0
#define RTC_READ	0xD1
// мьютекс контроллера I2C
extern SemaphoreHandle_t i2c_lock;
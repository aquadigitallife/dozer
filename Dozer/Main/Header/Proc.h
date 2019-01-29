
void LED_TaskProc(void *Param);
void FlashProc(void *Param);

// Для сторожевой собаки
void WDG_TaskProc(void *Param);
void ResetWatchdog(uint32_t &LastResetTime, const uint8_t TaskID, const uint32_t Time, const uint8_t Param, const bool Force = false);

/*-----------------------Заслонка, диспенсер-------------------*/
void Motor0Proc(void *Param);
void Motor0Cycle(void *Param);
void Motor1Proc(void *Param);
/*-------------------------Опрос кнопок-------------------------*/
void ButtonsProc(void *Param);
/*-----------------------------UART------------------------------*/
StreamBufferHandle_t InitBLEUart(size_t xBufferSizeBytes);
void BLEUartTx(uint32_t len, uint8_t *data);
int32_t BLEUartRx(uint32_t len, uint8_t *data);
int32_t BLEUartPeek(void);
/*------------------------------BLE------------------------------*/
struct ble_date_time
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} __packed;

void rtc_update_ble(struct ble_date_time *arg);
void BLEProc(void *Param);
/*------------------------------I2C-----------------------------*/
void Init_I2C(void);

template <typename T>
BaseType_t i2c(uint8_t dev, T addr, uint32_t len, const void* data);
/*-----------------------------RTC-------------------------------*/
void ble_update_rtc(const struct ble_date_time *arg);
void RTCProc(void *Param);
/*------------------------------SPI------------------------------*/
void start_SPI(void);
void set_spi_handling_task(void);
void clear_spi_handling_task(void);
void Init_SPI(void);
/*-----------------------------AD7799----------------------------*/
void AD7799Proc(void *Param);
/*-----------------------------EEPROM----------------------------*/
#define TENSO_OFFSET_ADDR	0x0000

BaseType_t ee_write(uint16_t addr, uint16_t len, const void *data);
portINLINE void ee_read(uint16_t addr, uint16_t len, const void *data)
{
	i2c<uint16_t>(0xA1, addr, len, data);
}


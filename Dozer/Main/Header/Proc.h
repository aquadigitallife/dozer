
void LED_TaskProc(void *Param);
void FlashProc(void *Param);

// Для сторожевой собаки
void WDG_TaskProc(void *Param);
void ResetWatchdog(uint32_t &LastResetTime, const uint8_t TaskID, const uint32_t Time, const uint8_t Param, const bool Force = false);


//void InitSM1(uint32_t period);
//void StartSM1(bool dir);
//void StopSM1(void);
void Motor0Proc(void *Param);
void Motor0Cycle(void *Param);
void Motor1Proc(void *Param);

void ButtonsProc(void *Param);

StreamBufferHandle_t InitBLEUart(size_t xBufferSizeBytes);
void BLEUartTx(uint32_t len, uint8_t *data);
int32_t BLEUartRx(uint32_t len, uint8_t *data);
int32_t BLEUartPeek(void);

void BLEProc(void *Param);

void Init_I2C(void);
void I2C(uint8_t dev, uint8_t addr, uint32_t len, const void* data);

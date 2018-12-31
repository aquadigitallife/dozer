
void LED_TaskProc(void *Param);


// Для сторожевой собаки
void WDG_TaskProc(void *Param);
void ResetWatchdog(uint32_t &LastResetTime, const uint8_t TaskID, const uint32_t Time, const uint8_t Param, const bool Force = false);

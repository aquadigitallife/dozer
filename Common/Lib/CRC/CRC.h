
#ifdef USE_CRC16
uint16_t CRC16(const void *Data, const size_t Size);
#endif
#ifdef USE_CRC16_TABLE
uint16_t CRC16_Table(const void *Data, const size_t Size);
#endif

#ifdef USE_CRC32
uint32_t CRC32(const void *Data, const size_t Size);
#endif
#ifdef USE_CRC32_TABLE
uint32_t CRC32_Table(const void *Data, const size_t Size);
#endif

#include "CRC_Table.h"

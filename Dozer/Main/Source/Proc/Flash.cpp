#include "Global.h"

using namespace S25FL;

#define SWAP_ADDR(X) (((X)>>24) | ((X)<<24) | (((X)>>8)&0x0000FF00UL) | (((X)<<8)&0x00FF0000UL))

#pragma pack (push, 1)

struct STR_BUF
{
	uint8_t Cmd;
	uint32_t Addr;
	uint8_t Data[S25FL512S::PAGE_SIZE];
};

#pragma pack (pop)

enum class FLASH_TYPE {UNKNOW = 0, S25FL127S, S25FL512S, S70FL01G, W25Q128FV, W25M512JV, COUNT};
static FLASH_TYPE FlashType;
// TODO Подготовится к другим типам памяти, в частности к W25M512JV и S70FL01G

static SemaphoreHandle_t DMA_End; // Семафор окончания приёма/передачи данных

///////////////////////////////////////////////////////////////////////////////

static inline void OneTimeInit()
{
	// Включаем управление от DMA
	SPI1->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;

	// DMA2, stream 0, channel 3 - приём
	// DMA2, stream 2, channel 2 - передача
	// Адрес периферии
	DMA2_Stream0->PAR  = (uint32_t)(&SPI1->DR);
	DMA2_Stream2->PAR  = (uint32_t)(&SPI1->DR);
}

static inline void Start(void* const TX_RX, const uint32_t Count)
{
	// Базовая настройка: очень высокий приоритет, увеличиваем указатель в памяти, разрешаем работу
	const uint32_t DMA_CR_BASE = (3 << DMA_SxCR_PL_Pos) | DMA_SxCR_MINC | DMA_SxCR_EN;
	// Приём: выбираем третий канал, разрешаем прерывание по окончании работы
	const uint32_t DMA_CR_RX = DMA_CR_BASE | (3 << DMA_SxCR_CHSEL_Pos) | DMA_SxCR_TCIE;
	// Передача: выбираем второй канал, направление передачи память->периферия
	const uint32_t DMA_CR_TX = DMA_CR_BASE | (2 << DMA_SxCR_CHSEL_Pos) | DMA_SxCR_DIR_0;

	// Настройка DMA
	// Количество транзакций
	DMA2_Stream0->NDTR = Count;
	DMA2_Stream2->NDTR = Count;

	// Адрес памяти
	DMA2_Stream0->M0AR = (uint32_t)TX_RX;
	DMA2_Stream2->M0AR = (uint32_t)TX_RX;

	// Включаем DMA
	DMA2_Stream0->CR = DMA_CR_RX;
	DMA2_Stream2->CR = DMA_CR_TX;

	// Включаем SPI
	SPI1->CR1 = SPI_DIV_4    | // Делитель
	            SPI_CR1_MSTR | // Мы ведущие
	            SPI_CR1_SSM  | SPI_CR1_SSI | // Управление CS программное
	            SPI_CR1_SPE;
}

static inline void Stop()
{
	SPI1->CR1 = 0;
	SPI1->SR  = 0;

	// Обязательно сбрасываем флаги прерываний, иначе DMA потом не запустится
	DMA2->LIFCR = 0x3DUL | (0x3DUL << 16);

	DMA2_Stream0->CR = 0;
	DMA2_Stream2->CR = 0;
}

static void Proc(void* const TX_RX, const uint32_t Count)
{
	FLASH_SELECT;
	Start(TX_RX, Count);

	// Ждём прерывания от DMA
	xSemaphoreTake(DMA_End, portMAX_DELAY);
	// Ждём, пока SPI закончит работу

	while (SPI1->SR & SPI_SR_BSY);

	FLASH_DESELECT;
	Stop();
}

///////////////////////////////////////////////////////////////////////////////
// Команды одинаковы для всех типов памяти
const uint8_t UN_CMD_READ_ID   = S25FL::CMD_READ_ID;
const uint8_t UN_CMD_READ      = S25FL::CMD_READ;
const uint8_t UN_CMD_WRITE     = S25FL::CMD_PP  ;
const uint8_t UN_CMD_SEC_ERASE = S25FL::CMD_SE  ;
const uint8_t UN_CMD_WR_EN     = S25FL::CMD_WREN;
const uint8_t UN_CMD_WR_DI     = S25FL::CMD_WRDI;


///////////////////////////////////////////////////////////////////////////////
// Работа с регистрами

static uint8_t ReadSR1()
{
	uint8_t Cmd[2] = {CMD_RDSR1};
	Proc(&Cmd, sizeof(Cmd));

	return Cmd[1];
}

static void ClearSR1()
{
	uint8_t Cmd = CMD_CLSR;
	Proc(&Cmd, sizeof(Cmd));
}

static uint8_t ReadCR()
{
	uint8_t Cmd[2] = {CMD_RDCR};
	Proc(&Cmd, sizeof(Cmd));

	return Cmd[1];
}

static uint8_t WriteEnable()
{
	//ReadSR1();

	uint8_t Cmd = CMD_WREN;
	Proc(&Cmd, sizeof(Cmd));

	uint8_t Cnt = 10;

	// Ждём разрешения записи
	while (!(ReadSR1() & REG_SR1_WEL))
	{
		if (--Cnt == 0)
			return 1; // Не дождались

		vTaskDelay(1);
}

	return 0;
}

static void WriteDisable()
{
	uint8_t Cmd = CMD_WRDI;
	Proc(&Cmd, sizeof(Cmd));
}

static uint8_t WriteSR1_CR(const uint8_t SR1, const uint8_t CR)
{
	if (WriteEnable())
	{
		// Не получилось
		return 1;
	}

	uint8_t Cmd[3] = {CMD_WRR, SR1, CR};
	Proc(&Cmd, sizeof(Cmd));

	// Ждём окончания записи
	while (ReadSR1() & REG_SR1_WIP)
		vTaskDelay(1);

	return 0;
}

static uint8_t SectorErase(const uint32_t Addr)
{
	if (WriteEnable())
	{
		// Не получилось
		return 1;
	}

	uint8_t Cmd[5] = {CMD_4SE,
	                  (uint8_t)((Addr >> 24) & 0xFF),
	                  (uint8_t)((Addr >> 16) & 0xFF),
	                  (uint8_t)((Addr >>  8) & 0xFF),
	                  (uint8_t)((Addr      ) & 0xFF)};

	Proc(&Cmd, sizeof(Cmd));

	return 0;
}

static uint32_t ReadID()
{
	// Читаем идентификатор REMS

	uint8_t Cmd[6] = {UN_CMD_READ_ID};

	Proc(&Cmd, sizeof(Cmd));

	const uint16_t ReadID = (((uint16_t)Cmd[4]) << 8) | (Cmd[5]);

	return ReadID;
}

///////////////////////////////////////////////////////////////////////////////

void FlashProc(void *Param)
{
	UNUSED(Param);
	UNUSED(WriteDisable);
	UNUSED(ReadCR);

	// У этой задачи имунитет, т.к. она генерирует очень много событий при своей работе
//	STR_WDG_CMD WDG_Cmd = {WDG_TASK_FLASH, portMAX_DELAY, 1};
//	xQueueSend(WDG_Cmd_Queue, &WDG_Cmd, 3);

	OneTimeInit();

	// Буфер для записи/чтения данных
	STR_BUF Buf = {};

	DMA_End = xSemaphoreCreateBinary();
	configASSERT(DMA_End);

///////////////////////////////////////////////////////////////////////////////
	FlashType = FLASH_TYPE::UNKNOW;

	// Пробуем определить тип подключённой памяти

	// Читаем идентификатор REMS
//	memset(&Buf, 0, sizeof(Buf));
//	Buf.Cmd = UN_CMD_READ_ID;
//	Proc(&Buf, 16);
//
//	uint16_t ReadID = (((uint16_t)Buf.Data[1]) << 8) | (Buf.Data[0]);

	// Читаем идентификатор
	const uint16_t FlashID = ReadID();

	switch (FlashID)
	{
//	case S25FL127S::ASK_READ_ID: FlashType = FLASH_TYPE::S25FL127S; break;
	case S25FL512S::ASK_READ_ID: FlashType = FLASH_TYPE::S25FL512S; break;
//	case W25Q128FV::ASK_READ_ID: FlashType = FLASH_TYPE::W25Q128FV; break;
	}

///////////////////////////////////////////////////////////////////////////////
	// Настройка памяти
	switch (FlashType)
	{
	case FLASH_TYPE::S25FL512S:
	{
		
		// Настраиваем только задержку, т.к. мы всегда работает на частоте ниже 50МГц
		WriteSR1_CR(0, REG_CR1_LC0 | REG_CR1_LC1);
	}


	break;

	default:
		// Нет подключённой памяти или её тип неизвестен
		vTaskSuspend(0);
		break;
	}

	// Выдаём сигнал о работоспособности памяти
	xSemaphoreGive(FlashExist_Sem);

	///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
//	vTaskDelay(1);
//
//	// Читаем идентификатор REMS
//	Buf.Cmd = CMD_READ_ID;
//	Buf.Addr = 0;
//	Proc(&Buf, sizeof(STR_BUF));
//
//	vTaskDelay(1);
//
//	// Читаем идентификатор
//	Buf.Cmd = CMD_RDID;
//	Proc(&Buf, sizeof(STR_BUF));
//
//	vTaskDelay(1);
//
//	// Читаем идентификатор
//	Buf.Cmd = CMD_RES;
//	Proc(&Buf, sizeof(STR_BUF));
//
//	vTaskDelay(1);
//
//	// Читаем электронную подпись
//	Buf.Cmd = CMD_RSFDP;
//	Proc(&Buf, sizeof(STR_BUF));
//
//	vTaskDelay(1);
//
//	// Читаем регистр статуса
//	Buf.Cmd = CMD_RDSR1;
//	Proc(&Buf, sizeof(STR_BUF));
//
//	vTaskDelay(1);
//
//	// Читаем регистр статуса
//	Buf.Cmd = CMD_RDSR2;
//	Proc(&Buf, sizeof(STR_BUF));
//
//	vTaskDelay(1);
//
//	// Читаем конфигурационный регистр
//	Buf.Cmd = CMD_RDCR;
//	Proc(&Buf, sizeof(STR_BUF));
//
//	vTaskDelay(1);
//
//	memset(&Buf, 0, sizeof(Buf));
//
//	WriteEnable();
//	ReadSR1();
//
//	vTaskDelay(1);
//
//	WriteDisable();
//	ReadSR1();
//
//	vTaskDelay(1);
#endif

	while (1)
	{
		STR_FLASH_CMD FlashCmd = {}; // Команда на исполнение
		uint32_t FlashRes = 0; // Результат выполнения

		// Ждём команды
		if (!xQueueReceive(FlashCmd_Queue, &FlashCmd, portMAX_DELAY))
			continue;

		switch (FlashCmd.Cmd)
		{
		case FLASH_CMD::READ_DATA: // Чтение произвольных данных
			{
				// Произвольные данные можем читать только в заранее подготовленный буфер
				if (FlashCmd.DataPtr)
				{
				uint32_t Count = FlashCmd.Size;

				Count += FLASH_BUF_HEAD_SIZE;

				STR_BUF* Data = (STR_BUF*)(FlashCmd.DataPtr);

				Data->Cmd  = CMD_4READ;
				Data->Addr = SWAP_ADDR(FlashCmd.Arg);

				// Работаем
				Proc(Data, Count);
				}
				else
				{
					FlashRes = 1;
				}

				// Сигнализируем о завершении
				if (FlashCmd.ResQueue)
					xQueueSend(FlashCmd.ResQueue, &FlashRes, 0);
			}
			break;

		case FLASH_CMD::VERIFY_PAGE: // Проверка страницы
		case FLASH_CMD::READ_PAGE: // Чтение страницы
			{
				uint32_t Count = S25FL512S::PAGE_SIZE;

				if (FlashCmd.Size < Count)
					Count = FlashCmd.Size;

				Buf.Cmd = CMD_4READ;
				Buf.Addr = SWAP_ADDR(FlashCmd.Arg * S25FL512S::PAGE_SIZE);

				// Работаем
				Proc(&Buf, Count + FLASH_BUF_HEAD_SIZE);

				if (FlashCmd.Cmd == FLASH_CMD::READ_PAGE)
				{
					// Отправляем прочитанные данные
					if (FlashCmd.DataPtr)
					{
						memcpy(FlashCmd.DataPtr, Buf.Data, Count);
					}
					else
					{
						xQueueSend(FlashCmd.DataQueue, Buf.Data, portMAX_DELAY);
					}
				}
				else
				{
					// Забираем эталонные данные
					uint8_t OrigData[S25FL512S::PAGE_SIZE];
					xQueueReceive(FlashCmd.DataQueue, OrigData, portMAX_DELAY);

					int res = memcmp(OrigData, Buf.Data, Count);

					FlashRes = res != 0;

					OrigData[0] = 1;

					if (res == 0)
						OrigData[0] = 0;

					// Отправляем результат проверки
					xQueueSend(FlashCmd.DataQueue, OrigData, portMAX_DELAY);
				}

				// Сигнализируем о завершении
				if (FlashCmd.ResQueue)
					xQueueSend(FlashCmd.ResQueue, &FlashRes, 0);
			}

			break;

		case FLASH_CMD::WRITE_PAGE: // Запись страницы
			{
				uint32_t Count = S25FL512S::PAGE_SIZE;

				if (FlashCmd.Size < Count)
					Count = FlashCmd.Size;

				// Снимаем защиту от записи
				if (WriteEnable() == 0)
				{
					Buf.Cmd = CMD_4PP;
					Buf.Addr = SWAP_ADDR(FlashCmd.Arg * S25FL512S::PAGE_SIZE);

					// Читаем данные для записи
					if (FlashCmd.DataPtr)
					{
						memcpy(Buf.Data, FlashCmd.DataPtr, Count);
					}
					else
					{
						xQueueReceive(FlashCmd.DataQueue, Buf.Data, portMAX_DELAY);
					}

					// Работаем
					Proc(&Buf, Count + FLASH_BUF_HEAD_SIZE);

					// Ждём окончания записи
					while (ReadSR1() & REG_SR1_WIP);

					ClearSR1();
				}
				else
				{
					// Не получилось
					FlashRes = 1;
				}

				// Сигнализируем о завершении
				if (FlashCmd.ResQueue)
					xQueueSend(FlashCmd.ResQueue, &FlashRes, 0);
			}
			break;

		case FLASH_CMD::SECTOR_ERASE: // Стирание сектора
			{
				for (uint32_t i = 0; i < FlashCmd.Size; i++)
				{
					// Работаем
					if (SectorErase((FlashCmd.Arg + i) * S25FL512S::SECTOR_SIZE))
					{
						// Неполучилось стереть сектор
						FlashRes = 1;
						break;
					}

					// Засыпаем на 400 мс, т.к. флешка всё равно не ответит раньше
					vTaskDelay(MS_TO_TICK(400));

					// Ждём окончания записи
					while (ReadSR1() & REG_SR1_WIP)
						vTaskDelay(1);

					ClearSR1();
				}

				// Сигнализируем о завершении
				if (FlashCmd.ResQueue)
					xQueueSend(FlashCmd.ResQueue, &FlashRes, 0);
			}
			break;

		default:
			// Пришла неизвестная команда, ничего не делаем

			if (FlashCmd.ResQueue)
			{
				FlashRes = 0xFFFFFFFFUL;
				xQueueSend(FlashCmd.ResQueue, &FlashRes, 0);
			}
			break;
		}
	}
}

ISR(DMA2_Stream0_IRQHandler)
{
	if (DMA2->LISR & DMA_LISR_TCIF0)
	{
		// Приём окончен

		BaseType_t pHigh = 0;
		xSemaphoreGiveFromISR(DMA_End, &pHigh);
		portYIELD_FROM_ISR(pHigh);
		DMA2->LIFCR = DMA_LIFCR_CTCIF0;
	}
}

#include "Global.h"
#include <stdarg.h>

/*
	Набор функций передачи AT-команд модема SIM5320.
*/

const char at_cmd_ate_off[] = "ATE0\r\n";					// команда отключения выдачи эхо.
const char at_cmd_ate_on[] = "ATE1\r\n";					// команда включения эхо

const char at_cmd_https_state[] = "AT+CHTTPSSTATE\r\n";		// команда запроса состояния соединения https

const char at_cmd_https_start[] = "AT+CHTTPSSTART\r\n";		// команда инициализации https-стека
const char at_cmd_https_stop[] = "AT+CHTTPSSTOP\r\n";		// команда деинициализации https-стека

const char at_cmd_https_open[] = "AT+CHTTPSOPSE=";			// команда открытия https-соединения
const char at_cmd_https_close[] = "AT+CHTTPSCLSE\r\n";		// команда закрытия https-соединения

const char at_cmd_https_send[] = "AT+CHTTPSSEND=";			// команда отбравки данных
const char at_cmd_https_recv[] = "AT+CHTTPSRECV";			// команда приёма данных

const char answ_ok[] = "OK\r\n";							// ответ модема ok
const char answ_error[] = "ERROR\r\n";						// ответ модема error

char answer[256];											// буфер для приёма данных

static FILE *fd = NULL;										// дескриптор UART-а обмена с SIM5320

/*
Процедура обмена AT-командами с модемом.
callback - процедура обработки ответа модема
param - параметр для процедуры callback
format... - AT-команда в printf-стиле
*/
bool at_cmd(void (*callback)(void *), void *param, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	vfprintf(fd, format, args);			// отправляем AT-команду в модем
	va_end(args);

	do {
		fgets(answer, sizeof(answer), fd);	// принимаем ответ модема
	} while (strcmp(answer, "ATE0\r\r\n") == 0
	|| strcmp(answer, "+STIN: 25\r\n") == 0
	|| strcmp(answer, "+CHTTPS: RECV EVENT\r\n") == 0);	// пропускаем эхо от команды ATE0, пропускаем +STIN и RECV EVENT
	
	if (strcmp(answer, answ_ok) == 0) return false;		// если модем ответил OK - возвращаем хорошее завершение
	if (strcmp(answer, "\r\n") != 0) return true;		// иначе модем возвращает данные обрамлённые \r\n...\r\n
	if (callback) callback(param);						// обрабатываем ответ модема (возможно с отправкой данных)
	fgets(answer, sizeof(answer), fd);					// принимаем завершающий ответ
	while (strcmp(answer, "+STIN: 25\r\n") == 0 || strcmp(answer, "+CHTTPS: RECV EVENT\r\n") == 0) {
		fgets(answer, sizeof(answer), fd);
		fgets(answer, sizeof(answer), fd);
	}													// пропускаем асинхронные сообщения +STIN, RECV EVENT
	return (strcmp(answer, answ_ok) != 0);				// если ответ не равен OK - возвращаем плохое завершение
}

/*
Функция ожидания данных.
Если приняты данные, равные str - хорошее завершение
Если приняты данные, равные str2 - плохое
*/
bool at_wait(const char *str, const char *str2)
{
	do {
		fgets(answer, sizeof(answer), fd);
		if (strcmp(answer, str2) == 0) return true;
	} while (strcmp(answer, str) != 0);
	return false;
}

/*
Callback для команды запроса состояния https-соединения
state - по этому указателю функция возвращает код состояния соединения
*/
void https_state_callback(void *state)
{
	fscanf(fd, "+CHTTPSSTATE:%d\r\n", (int*)state);
}

/*
Callback для команды отправки данных
*/
void send_string(void *hdr)
{
	char c;
//	printf("%s", (char*)hdr);
	c = (char)fgetc(fd);				// после отправки команды ожидаем признак
	if (c != '>') return;				// запроса данных
	fputs((char*)hdr, fd);				// отправляем данные
	fgets(answer, sizeof(answer), fd);	// принимаем ответ
}

/*
Callback для команды запроса данных 
возвращает размер принятых данных в переменную по указателю len
*/
void recv_https(void *len)
{
	fscanf(fd, "+CHTTPSRECV: LEN,%d\r\n", (int*)len);
}

/*
Функция производит аппаратный сброс модема, ожидает его инициализацию
(выдачу сообщения PB DONE), и выдачу команды на отключение эхо
*/
FILE *gsm_init(void)
{
//	printf("initialize gsm...");
	fd = fopen("GSM", "r+");
	if (fd == NULL) return NULL;
	
	GSM_RST_ON;
	vTaskDelay(MS_TO_TICK(100));
	GSM_RST_OFF;
	
	do { fgets(answer, sizeof(answer), fd);	} while (strcmp(answer, "PB DONE\r\n") != 0);
	
//	printf("done\r\n");
	
	if (at_cmd(NULL, NULL, "%s", at_cmd_ate_off)) return NULL;
	return fd;
}

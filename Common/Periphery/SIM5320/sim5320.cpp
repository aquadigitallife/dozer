#include "Global.h"
#include <stdarg.h>

const char at_cmd_ate_off[] = "ATE0\r\n";
const char at_cmd_ate_on[] = "ATE1\r\n";

const char at_cmd_https_state[] = "AT+CHTTPSSTATE\r\n";

const char at_cmd_https_start[] = "AT+CHTTPSSTART\r\n";
const char at_cmd_https_stop[] = "AT+CHTTPSSTOP\r\n";

const char at_cmd_https_open[] = "AT+CHTTPSOPSE=";
const char at_cmd_https_close[] = "AT+CHTTPSCLSE\r\n";

const char at_cmd_https_send[] = "AT+CHTTPSSEND=";
const char at_cmd_https_recv[] = "AT+CHTTPSRECV";

const char answ_ok[] = "OK\r\n";
const char answ_error[] = "ERROR\r\n";

char answer[256];

static FILE *fd = NULL;

bool at_cmd(void (*callback)(void *), void *param, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	vfprintf(fd, format, args);
	va_end(args);

	do {
		fgets(answer, sizeof(answer), fd);
	} while (strcmp(answer, "ATE0\r\r\n") == 0
	|| strcmp(answer, "+STIN: 25\r\n") == 0
	|| strcmp(answer, "+CHTTPS: RECV EVENT\r\n") == 0);
	
	if (strcmp(answer, answ_ok) == 0) return false;
	if (strcmp(answer, "\r\n") != 0) return true;
	if (callback) callback(param);
	fgets(answer, sizeof(answer), fd);
	while (strcmp(answer, "+STIN: 25\r\n") == 0 || strcmp(answer, "+CHTTPS: RECV EVENT\r\n") == 0) {
		fgets(answer, sizeof(answer), fd);
		fgets(answer, sizeof(answer), fd);
	}
	return (strcmp(answer, answ_ok) != 0);
}

bool at_wait(const char *str, const char *str2)
{
	do {
		fgets(answer, sizeof(answer), fd);
		if (strcmp(answer, str2) == 0) return true;
	} while (strcmp(answer, str) != 0);
	return false;
}

void https_state_callback(void *state)
{
	fscanf(fd, "+CHTTPSSTATE:%d\r\n", (int*)state);
}

void send_string(void *hdr)
{
	char c;
//	printf("%s", (char*)hdr);
	c = (char)fgetc(fd);
	if (c != '>') return;
	fputs((char*)hdr, fd);
	fgets(answer, sizeof(answer), fd);
}

void recv_https(void *len)
{
	fscanf(fd, "+CHTTPSRECV: LEN,%d\r\n", (int*)len);
}

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

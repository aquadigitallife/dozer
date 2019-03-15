/*
	Модуль связи с сервером по GSM
*/

#include "Global.h"
#include <stdarg.h>

//typedef at_callback 
enum https_state_enum {
	HTTPS_STATE_NONE,
	HTTPS_STATE_ACCQUIRED_HTTPS,
	HTTPS_STATE_OPENING_NETWORK,
	HTTPS_STATE_CLOSING_NETWORK,
	HTTPS_STATE_OPENED_NETWORK,
	HTTPS_STATE_CLOSING_SESSION,
	HTTPS_STATE_OPENING_SESSION,
	HTTPS_STATE_OPENED_SESSION
};

static const char host[] = "api.aquadigitallife.com";
static const int port = 443;

static const char https_host[] = "https://api.aquadigitallife.com/v1";
static const char cmd_auth[] = "/auth/login";
static const char cmd_get_dispancer[] = "/tank/dispanser";
static const char cmd_set_dispancer[] = "/tank/update-dispenser";

static const char at_cmd_ate_off[] = "ATE0\r";
static const char at_cmd_ate_on[] = "ATE1\r";

static const char at_cmd_https_state[] = "AT+CHTTPSSTATE\r";

static const char at_cmd_https_start[] = "AT+CHTTPSSTART\r";
static const char at_cmd_https_stop[] = "AT+CHTTPSSTOP\r";

static const char at_cmd_https_open[] = "AT+CHTTPSOPSE=";
static const char at_cmd_https_close[] = "AT+CHTTPSCLSE\r";

static const char at_cmd_https_send[] = "AT+CHTTPSSEND=";
static const char at_cmd_https_recv[] = "AT+CHTTPSRECV";

static const char answ_ok[] = "OK\r\n";
static const char answ_error[] = "ERROR\r\n";

//static char token[512];
static char answer[256];

static FILE *fd = NULL;

bool at_cmd(void (*callback)(void *), void *param, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	vprintf(format, args);
	vfprintf(fd, format, args);
	va_end(args);

	fgets(answer, sizeof(answer), fd);
	printf("%s", answer);
	if (strcmp(answer, "\r\n") != 0) return true;
	if (callback) callback(param);
	fgets(answer, sizeof(answer), fd);
	printf("%s", answer);
	return (strcmp(answer, answ_ok) != 0);
}

void at_wait(const char *str)
{
	do {
		fgets(answer, sizeof(answer), fd);
		printf("%s", answer);
	} while (strcmp(answer, str) != 0);
}
/*
void https_state_callback(void *state)
{
	fscanf(fd, "+CHTTPSSTATE:%d\r\n", (int*)state);
}
*/

void send_header(void *hdr)
{
	char c = (char)fgetc(fd);
	if (c != '>') return;
	fputs((char*)hdr, fd);
	printf("%s", (char*)hdr);
	fgets(answer, sizeof(answer), fd);
	printf("%s", answer);
}

void recv_https(void *len)
{
	fscanf(fd, "+CHTTPSRECV: LEN,%d\r\n", (int*)len);
}

void gsm_init(void)
{
	fd = fopen("GSM", "r+");
	if (fd == NULL) vTaskDelete(NULL);
	
	GSM_RST_ON;
	vTaskDelay(MS_TO_TICK(100));
	GSM_RST_OFF;
	
	for (int i = 0; i < 3;) {
		fgets(answer, sizeof(answer), fd);
		printf("%s", answer);
		if (strcmp(answer, "+STIN: 25\r\n") == 0) i++;
	}

	while(at_cmd(NULL, NULL, "%s\n", at_cmd_ate_off));
	
}

char *read_token(void)
{
	return NULL;
}

char *read_inn(void)
{
	char *retval;
	char pinn[] = "11111111111";
	retval = (char*)malloc(sizeof(pinn));
	memcpy(retval, pinn, sizeof(pinn));
	return retval;
}

char *read_passwd(void)
{
	char *retval;
	char ppass[] = "demo";
	retval = (char*)malloc(sizeof(ppass));
	memcpy(retval, ppass, sizeof(ppass));
	return retval;
}

char *http_header(const char *addr, char *request)
{
	char req_len[25];
	char *post = (char*)malloc(17 + strlen(https_host) + strlen(addr));
	char *host = (char*)malloc(9 + strlen(https_host) + strlen(addr));
	char *retval;
	
	sprintf(post, "POST %s%s HTTP/1.1\r\n", https_host, addr);
	sprintf(host, "HOST: %s%s\r\n",  https_host, addr);
	sprintf(req_len, "Content-Length: %d\r\n", strlen(request) - 2);
	
	retval = (char*)malloc(strlen(post) + strlen(host) + strlen(req_len) + strlen(request) + 3);
	sprintf(retval, "%s%s%s\r\n%s", post, host, req_len, request);
	free(post); free(host);
	
	return retval;
}

void https_start(void *Param)
{
	char *token = read_token();
	

	gsm_init();
	at_cmd(NULL, NULL, "%s\n", at_cmd_https_start);
	at_cmd(NULL, NULL, "%s\"%s\",%d\r\n", at_cmd_https_open, host, port);

	if (token == NULL) {
		char *header;
		char *inn = read_inn();
		char *passwd = read_passwd();
		int recv = 0;
		
		char *request = (char*)malloc(strlen(inn) + strlen(passwd) + 26);
		
		sprintf(request, "{ INN:\"%s\", Password:\"%s\" }\r\n", inn, passwd);
		free(inn); free(passwd);
		
		header = http_header(cmd_auth, request);
		free(request);
		
		if (at_cmd(send_header, header, "%s%d\r\n", at_cmd_https_send, strlen(header))) printf("error:not ok\r\n");
		free(header);
		
		at_wait("+CHTTPS: RECV EVENT\r\n");
		
		at_cmd(recv_https, &recv, "%s?\r\n", at_cmd_https_recv);
		printf("received %d bytes\r\n", recv);
	}
//	at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);
//	printf("status is: %d\n", status);

	at_cmd(NULL, NULL, "%s\n", at_cmd_https_close);
	at_cmd(NULL, NULL, "%s\n", at_cmd_https_stop);

	do {
		fgets(answer, sizeof(answer), fd);
		printf("%s", answer);
	} while (true);
}
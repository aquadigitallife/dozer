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

static const char at_cmd_ate_off[] = "ATE0\r\n";
static const char at_cmd_ate_on[] = "ATE1\r\n";

static const char at_cmd_https_state[] = "AT+CHTTPSSTATE\r\n";

static const char at_cmd_https_start[] = "AT+CHTTPSSTART\r\n";
static const char at_cmd_https_stop[] = "AT+CHTTPSSTOP\r\n";

static const char at_cmd_https_open[] = "AT+CHTTPSOPSE=";
static const char at_cmd_https_close[] = "AT+CHTTPSCLSE\r\n";

static const char at_cmd_https_send[] = "AT+CHTTPSSEND=";
static const char at_cmd_https_recv[] = "AT+CHTTPSRECV";

static const char answ_ok[] = "OK\r\n";
static const char answ_error[] = "ERROR\r\n";

//static char token[512];
static char answer[256];

static FILE *fd = NULL;

int gsm_get_str(int len, char *ptr)
{
	int retval;
	if (len < 2) return 0;
	for (retval = 0; retval < len-1;) {
		retval += GSMUartRx(1, &ptr[retval]);
		if (retval > 0) if (ptr[retval-1] == '\n') { ptr[retval] = '\0'; break; }
		ptr[retval] = '\0';
	}
	return retval;
}

char gsm_get_char(void)
{
	char c;
	GSMUartRx(1, &c);
	return c;
}

bool at_cmd(void (*callback)(void *), void *param, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	vprintf(format, args);
	vfprintf(fd, format, args);
	va_end(args);

	do {
		fgets(answer, sizeof(answer), fd);
		printf("%s", answer);
	} while (strcmp(answer, "ATE0\r\r\n") == 0 || strcmp(answer, "+STIN: 25\r\n") == 0 || strcmp(answer, "+CHTTPS: RECV EVENT\r\n") == 0);
	if (strcmp(answer, answ_ok) == 0) return false;
	if (strcmp(answer, "\r\n") != 0) return true;
	if (callback) callback(param);
	fgets(answer, sizeof(answer), fd);
	printf("%s", answer);
	while (strcmp(answer, "+STIN: 25\r\n") == 0 || strcmp(answer, "+CHTTPS: RECV EVENT\r\n") == 0) {
		fgets(answer, sizeof(answer), fd);
		printf("%s", answer);
		fgets(answer, sizeof(answer), fd);
		printf("%s", answer);
	}
	return (strcmp(answer, answ_ok) != 0);
}

bool at_wait(const char *str, const char *str2)
{
	do {
		fgets(answer, sizeof(answer), fd);
		printf("%s", answer);
		if (strcmp(answer, str2) == 0) return true;
	} while (strcmp(answer, str) != 0);
	return false;
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
	if (c != '>') { printf("%c", c); return; }
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
		if (strcmp(answer, "PB DONE\r\n") == 0) break;
		if (strcmp(answer, "+STIN: 25\r\n") == 0) i++;
	}

	while(at_cmd(NULL, NULL, "%s", at_cmd_ate_off));
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

char *http_header(const char *addr, const char *request)
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

char *aqual_request(const char *addr, const char *request)
{
		int recv = 0;
		char *header = http_header(addr, request);

		at_cmd(send_header, header, "%s%d\r\n", at_cmd_https_send, strlen(header));
		free(header);
		
		if (at_wait("+CHTTPS: RECV EVENT\r\n", "+CHTTPSNOTIFY: PEER CLOSED\r\n")) return NULL;

		do {
			if (at_cmd(recv_https, &recv, "%s?\r\n", at_cmd_https_recv)) return NULL;
			printf("received %d bytes\r\n", recv);
		} while (recv < 17);
		header = NULL;
		header = (char*)malloc(recv+3);
		if (header != NULL) {
			int len;
			for (int i = 0; i < recv; i += len) {
				if (i > 160) break;
				vTaskDelay(MS_TO_TICK(1000));
				if (at_cmd(NULL, NULL, "%s=32\r\n", at_cmd_https_recv)) {
					free(header);
					return NULL;
				}
				
				len = 0;
				do {
					fscanf(fd, "\r\n+CHTTPSRECV: DATA,%d\r\n", &len);
				} while (len == 0);
				
				printf("len= %d i= %d\r\n", len, i);
/*	
				for (int n = 0; n < len+2; n++) {
					header[i + n] = fgetc(fd);
				}
				header[i + len+2] = '\0';
				printf("%s", &header[i]);
*/
				fread(&header[i], len, 1, fd);
				header[i+len] = '\0';
				printf("%s\r\n", &header[i]);
				fgets(answer, sizeof(answer), fd);
				printf("%s", answer);

				fgets(answer, sizeof(answer), fd);
				printf("%s", answer);
				if (at_wait("+CHTTPS: RECV EVENT\r\n", "+CHTTPSNOTIFY: PEER CLOSED\r\n")) return NULL;
			}
		}
		return header;
}

void https_start(void *Param)
{
	char *token = read_token();
	

	gsm_init();
	at_cmd(NULL, NULL, "%s", at_cmd_https_start);
	at_cmd(NULL, NULL, "%s\"%s\",%d\r\n", at_cmd_https_open, host, port);

	if (token == NULL) {
		char *header;
		char *inn = read_inn();
		char *passwd = read_passwd();
//		int recv = 0;
		
		char *request = (char*)malloc(strlen(inn) + strlen(passwd) + 26);
		
		sprintf(request, "{ INN:\"%s\", Password:\"%s\" }\r\n", inn, passwd);
		free(inn); free(passwd);

		header = aqual_request(cmd_auth, request);
		if (header) {
			printf("%s\r\n", header);
			free(header);
		}
/*		header = http_header(cmd_auth, request);
		free(request);
		
		if (at_cmd(send_header, header, "%s%d\r\n", at_cmd_https_send, strlen(header))) {printf("error:not ok\r\n"); goto finish;}
		free(header);
		
		at_wait("+CHTTPS: RECV EVENT\r\n");
		
		at_cmd(recv_https, &recv, "%s?\r\n", at_cmd_https_recv);
		printf("received %d bytes\r\n", recv);
*/
	}
//	at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);
//	printf("status is: %d\n", status);

	at_cmd(NULL, NULL, "%s", at_cmd_https_close);
	at_cmd(NULL, NULL, "%s", at_cmd_https_stop);

	do {
		fgets(answer, sizeof(answer), fd);
		printf("%s", answer);
	} while (true);
}
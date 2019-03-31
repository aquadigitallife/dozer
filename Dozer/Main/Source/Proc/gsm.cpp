/*
	Модуль связи с сервером по GSM
*/

#include "Global.h"
#include <stdarg.h>

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

#define ON_ERROR(...) do { \
printf(__VA_ARGS__); \
goto network_close; \
} while(0)

static const char https[] = "https://";
static const char https_host[] = "api.aquadigitallife.com";
static const int port = 443;

static const char cmd_auth[] = "/v1/auth/login";
static const char cmd_get_dispancer[] = "/v1/tank/dispanser";
static const char cmd_set_dispancer[] = "/v1/tank/update-dispenser";

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

static char answer[256];

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

void send_header(void *hdr)
{
	char c = (char)fgetc(fd);
	if (c != '>') return;
	fputs((char*)hdr, fd);
	fgets(answer, sizeof(answer), fd);
}

void recv_https(void *len)
{
	fscanf(fd, "+CHTTPSRECV: LEN,%d\r\n", (int*)len);
}

bool gsm_init(void)
{
	printf("initialize gsm...");
	fd = fopen("GSM", "r+");
	if (fd == NULL) return true;
	
	GSM_RST_ON;
	vTaskDelay(MS_TO_TICK(100));
	GSM_RST_OFF;
	
	do { fgets(answer, sizeof(answer), fd);	} while (strcmp(answer, "PB DONE\r\n") != 0);
	
	printf("done\r\n");
	
	return at_cmd(NULL, NULL, "%s", at_cmd_ate_off);
}

char *read_token(void)
{
	return NULL;
}

char *read_inn(void)
{
	char *retval;
	static const char pinn[] = "11111111111";
	retval = (char*)malloc(sizeof(pinn));
	if (retval == NULL) return (char*)pinn;
	memcpy(retval, pinn, sizeof(pinn));
	return retval;
}

char *read_passwd(void)
{
	char *retval;
	static const char ppass[] = "demo";
	retval = (char*)malloc(sizeof(ppass));
	if (retval == NULL) return (char*)ppass;
	memcpy(retval, ppass, sizeof(ppass));
	return retval;
}

char *add_http_header(const char *addr, const char *request, const char *token)
{
	int hdr_len;
	char req_len[25];
	char *retval;

	char *post = (char*)malloc(17 + strlen(https) + strlen(https_host) + strlen(addr));
	char *host = (char*)malloc(9 + strlen(https) + strlen(https_host) + strlen(addr));
	if (post == NULL || host == NULL) return NULL;
	
	sprintf(post, "POST %s%s%s HTTP/1.1\r\n", https, https_host, addr);
	sprintf(host, "HOST: %s%s%s\r\n", https, https_host, addr);
	sprintf(req_len, "Content-Length: %d\r\n", strlen(request) - 2);
	
	hdr_len = strlen(post) + strlen(host) + strlen(req_len) + strlen(request) + 3;
	if (token != NULL) hdr_len += strlen(token);
	
	retval = (char*)malloc(hdr_len);
	if (retval == NULL) return NULL;
	
	if (token == NULL) sprintf(retval, "%s%s%s\r\n%s", post, host, req_len, request);
	else sprintf(retval, "%s%s%s%s\r\n%s", post, host, token, req_len, request);
	free(post); free(host);

	return retval;
}

char *https_request(const char *addr, const char *request, const char *token)
{
	int recv = 0;
	char *data = add_http_header(addr, request, token);

	if (data == NULL) return NULL;
	
	printf("sending request...");
	if (at_cmd(send_header, data, "%s%d\r\n", at_cmd_https_send, strlen(data))) {free(data); return NULL;}
	free(data);
	printf("done\r\nwaiting response...");
	if (at_wait("+CHTTPS: RECV EVENT\r\n", "+CHTTPSNOTIFY: PEER CLOSED\r\n")) {
		printf("peer closed\r\n");
		return NULL;
	}

	do {
		if (at_cmd(recv_https, &recv, "%s?\r\n", at_cmd_https_recv)) return NULL;
	} while (recv < 17);

	printf("done\r\nwill be receive %d bytes\r\n", recv);

	data = (char*)malloc(recv+3);
	if (data != NULL) {
		int len;
		printf("buffer allocated\r\n");
		for (int i = 0; i < recv; i += len) {
			if (at_cmd(NULL, NULL, "%s=32\r\n", at_cmd_https_recv)) {
				free(data);
				return NULL;
			}
				
			len = 0;
			do {
				fgets(answer, sizeof(answer), fd);
				fgets(answer, sizeof(answer), fd);
				sscanf(answer, "+CHTTPSRECV: DATA,%d\r\n", &len);
			} while (len == 0);
//			printf("read %d bytes\r\n", len);
			fread(&data[i], len+2, 1, fd);
			data[i+len+2] = '\0';

			fgets(answer, sizeof(answer), fd);
		}
		printf("data received\r\n");
	}
	return data;
}

char *get_token(void)
{
	char *httpjson, *retptr;
	char *inn = read_inn();
	char *passwd = read_passwd();

	char *request = (char*)malloc(strlen(inn) + strlen(passwd) + 26);

	if (request == NULL) return NULL;
	sprintf(request, "{ INN:\"%s\", Password:\"%s\" }\r\n", inn, passwd);
	free(inn); free(passwd);

	httpjson = https_request(cmd_auth, request, NULL);
	free(request);
	if (httpjson == NULL) return NULL;
	request = strstr(httpjson, "_Autorize") + strlen("_Autorize\":\"");
	*strchr(request, '\"') = '\0';
	retptr = (char*)malloc(strlen(request)+15);
	if (retptr == NULL) return NULL;
	sprintf(retptr, "_Authorize: %s\r\n", request);
	free(httpjson);
	return retptr;
}

void https_start(void *Param)
{
	https_state_enum status;
	static const char *tankId[28] = {
		 "595"
		,"610"
		,"611"
		,"612"
		,"613"
		,"614"
		,"615"
		,"616"
		,"619"
		,"620"
		,"622"
		,"624"
		,"627"
		,"628"
		,"629"
		,"631"
		,"632"
		,"633"
		,"742"
		,"743"
		,"745"
		,"772"
		,"773"
		,"913"
		,"1994"
		,"1995"
		,"1996"
		,"2004"
	};
	char *token, *request, *response;
	int hcode;

	gsm_init();
	printf("opening network...");
	if (at_cmd(NULL, NULL, "%s", at_cmd_https_start)) ON_ERROR("error!\r\n");
	if (at_cmd(NULL, NULL, "%s\"%s\",%d\r\n", at_cmd_https_open, https_host, port)) ON_ERROR("error!\r\n");
	printf("done\r\n");

	token = read_token();
	if (token == NULL) {
		token = get_token();
		if (token == NULL) ON_ERROR("no_memory!\r\n");
	}
	
	for (int i = 0; i < 28; i++) {
	request = (char*)malloc(13 + strlen(tankId[i]));
	if (request == NULL) ON_ERROR("no_memory!\r\n");
	sprintf(request, "{ nTankId: %s }\r\n", tankId[i]);
	response = https_request(cmd_get_dispancer, request, token);
	free(request);
	if (response == NULL) ON_ERROR("error!\r\n");
	
	sscanf(response, "HTTP/1.1 %d OK", &hcode);
	if (hcode != 200) {
		free(response);
		ON_ERROR("http ret: %d\r\n", hcode);
	}
	printf("%s", strstr(response, "{"));
	free(response);
	}

network_close:
	printf("close network...");
	at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);

	if (status == HTTPS_STATE_OPENED_SESSION) at_cmd(NULL, NULL, "%s", at_cmd_https_close);
	at_cmd(NULL, NULL, "%s", at_cmd_https_stop);
	printf("done\r\n");
	vTaskDelete(NULL);
}
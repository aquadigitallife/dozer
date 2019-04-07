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

//static const char ctoken[] = "A_uthorize: ZXlKMGVYQWlPaUpLVjFRaUxDSmhiR2NpT2lKSVV6STFOaUo5LmV5SkpaQ0k2SWpSaU9USmhZbVJtTFdRME9UUXROR1U1TmkwNVpqZzVMV000WVRNeE1EVmhNemt4WkNJc0lsVnpaWEpPWVcxbElqb2lZWHBoZEd0aU1qSkFaMjFoYVd3dVkyOXRJaXdpVUdodmJtVWlPaUlyT1RrMk56QTNPVGszTmpReElpd2lSVzFoYVd3aU9pSmhlbUYwYTJJeU1rQm5iV0ZwYkM1amIyMGlMQ0pTYjJ4bElqb2lRV1J0YVc0aUxDSkdkV3hzVG1GdFpTSTZJa3RpSUVGNllYUWlMQ0p1Um1GeWJVbGtJam94TENKdVJtbHphRWxrSWpveE9Td2lWR2x0WlNJNk1UVTFORFUxTVRZMk9YMC5vSnhtYm9EOWs4cXhLcEhFQUhlRWpuQ1BWSTNVaTFQbnVyS3NLUjBKRFpR";

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
/*	char *retptr;
	uint16_t taddr;
	if (pdFAIL == ee_read(TOKENADDR_ADDR, sizeof(taddr), &taddr)) return NULL;
	retptr = (char*)malloc(EEPROM_SIZE - taddr + 1);
	if (retptr == NULL) return NULL;
	if (pdFAIL == ee_read(taddr, EEPROM_SIZE - taddr, retptr)) return NULL;
	retptr[EEPROM_SIZE - taddr] = 0;
*/
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

BaseType_t get_tank_token(int nTankId, cJSON **tank, char **token)
{
	char *request, *response;
	cJSON *jtoken, *jitem;
	char *inn = read_inn();
	char *passwd = read_passwd();
	cJSON *json = cJSON_CreateObject();
	
	*tank = NULL; *token = NULL;
	
	if (json == NULL) return pdFAIL;
	cJSON_AddItemReferenceToObject(json, "INN", cJSON_CreateStringReference(inn));
	cJSON_AddItemReferenceToObject(json, "Password", cJSON_CreateStringReference(passwd));
	request = cJSON_PrintUnformatted(json);
	free(inn); free(passwd);
	cJSON_Delete(json);
	if (request == NULL) return pdFAIL;
	
	response = https_request(cmd_auth, request, NULL);
	free(request);
	if (response == NULL) return pdFAIL;
	
	json = cJSON_Parse(strstr(response, "{"));
	free(response);
	if (json == NULL) return pdFAIL;
	
	jtoken = cJSON_GetObjectItemCaseSensitive(json, "_Autorize");
	if (jtoken == NULL) goto error;
	if (jtoken->valuestring == NULL) goto error;
	if (strlen(jtoken->valuestring) == 0) goto error;
	
	*token = (char*)malloc(strlen(jtoken->valuestring) + 15);
	sprintf(*token, "_Authorize: %s\r\n", jtoken->valuestring);
	
	jtoken = cJSON_GetObjectItemCaseSensitive(json, "Tanks");
	if (jtoken == NULL) goto good;
	
	cJSON_ArrayForEach(jitem, jtoken)
	{
		cJSON *ch = cJSON_GetObjectItemCaseSensitive(jitem, "nTankId");
		if (ch == NULL) continue; 
		if (ch->valueint == nTankId) {
			*tank = cJSON_Duplicate(jitem, 1);
			break;
		}
	}
good:
	cJSON_Delete(json);
	return pdPASS;
error:
	cJSON_Delete(json);
	return pdFAIL;
}

void https_start(void *Param)
{
	https_state_enum status;
/*
	static const int tankId[28] = {
		 595
		,610
		,611
		,612
		,613
		,614
		,615
		,616
		,619
		,620
		,622
		,624
		,627
		,628
		,629
		,631
		,632
		,633
		,742
		,743
		,745
		,772
		,773
		,913
		,1994
		,1995
		,1996
		,2004
	};
*/
	char *token;
	char *request, *response;
	int hcode;
	cJSON *json, *tank;
	cJSON *jobj = NULL;

	gsm_init();
	printf("opening network...");
	if (at_cmd(NULL, NULL, "%s", at_cmd_https_start)) ON_ERROR("error!\r\n");
	if (at_cmd(NULL, NULL, "%s\"%s\",%d\r\n", at_cmd_https_open, https_host, port)) ON_ERROR("error!\r\n");
	printf("done\r\n");

	if (pdFAIL == get_tank_token(610, &tank, &token)) ON_ERROR("error get_tank_token\r\n");

	if (tank != NULL) {
		for (const cJSON *item = tank->child; item != NULL; item = item->next) {
			printf("%s ", item->string);
			switch (item->type) {
				case cJSON_False:
					printf("false\r\n");
					break;
				case cJSON_True:
					printf("true\r\n");
					break;
				case cJSON_Number:
					printf("%.3f\r\n", item->valuedouble);
					break;
				case cJSON_String:
					printf("%s\r\n", item->valuestring);
					break;
				default:
					printf("???\r\n");
			}
		}
		cJSON_Delete(tank);
	} else printf("tank is null\r\n");
//	for (int i = 0; i < 28; i++) {
/*---------------------get dispancer--------------------------------------*/	
	printf("\r\n\r\n");

	json = cJSON_CreateObject();
	if (json == NULL) ON_ERROR("no memory!\r\n");
	if (cJSON_AddNumberToObject(json, "nTankId", 610) == NULL) ON_ERROR("no memory!\r\n");
	request = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	if (request == NULL) ON_ERROR("no memory!\r\n");

	response = https_request(cmd_get_dispancer, request, token);
	free(request);
	if (response == NULL) ON_ERROR("error!\r\n");
	
	sscanf(response, "HTTP/1.1 %d OK", &hcode);
	if (hcode != 200) {
		free(response);
		ON_ERROR("http ret: %d\r\n", hcode);
	}
	printf("\r\n");
//	printf("%s", strstr(response, "{"));
	json = cJSON_Parse(strstr(response, "{"));
	free(response);

	if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            printf("Error before: %s\n", error_ptr);
        }
	}
	jobj = cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(json, "dispanser"), 1);
	cJSON_Delete(json);
/*------------------------------------------------------------------------*/	
	if (jobj != NULL) {
		for (const cJSON *item = jobj->child; item != NULL; item = item->next) {
			printf("%s ", item->string);
			switch (item->type) {
				case cJSON_False:
					printf("false\r\n");
					break;
				case cJSON_True:
					printf("true\r\n");
					break;
				case cJSON_Number:
					printf("%.3f\r\n", item->valuedouble);
					break;
				case cJSON_String:
					printf("%s\r\n", item->valuestring);
					break;
				case cJSON_Array:
				{
					const cJSON *sitem;
					printf("array:\r\n");
					cJSON_ArrayForEach(sitem, item)
					{
						for (const cJSON *ch = sitem->child; ch != NULL; ch = ch->next) {
							printf("    %s ", ch->string);
							switch (ch->type) {
								case cJSON_False:
									printf("false");
									break;
								case cJSON_True:
									printf("true");
									break;
								case cJSON_Number:
									printf("%.3f", ch->valuedouble);
									break;
								case cJSON_String:
									printf("%s", ch->valuestring);
									break;
								default:
									printf("???");
							}
						}
						printf("\r\n");
					}
					break;
				}
				default:
					printf("???\r\n");
			}
		}
	} else printf("dispancer is null\r\n");
	cJSON_Delete(jobj);
//	}

network_close:
	printf("close network...");
	at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);

	if (status == HTTPS_STATE_OPENED_SESSION) at_cmd(NULL, NULL, "%s", at_cmd_https_close);
	at_cmd(NULL, NULL, "%s", at_cmd_https_stop);
	printf("done\r\n");
	vTaskDelete(NULL);
}
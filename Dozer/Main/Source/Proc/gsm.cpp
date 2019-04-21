/*
	Модуль связи с сервером по GSM
*/

#include "Global.h"

#define ON_ERROR(...) do { \
goto network_close; \
} while(0)

//printf(__VA_ARGS__);

static const char https[] = "https://";
static const char https_host[] = "api.aquadigitallife.com";
static const int port = 443;

static const char cmd_auth[] = "/v1/auth/login";
static const char cmd_get_dispancer[] = "/v1/tank/dispanser";
static const char cmd_set_dispancer[] = "/v1/tank/update-dispenser";

static FILE *fd;

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
	
//	printf("sending request...");
	if (at_cmd(send_string, data, "%s%d\r\n", at_cmd_https_send, strlen(data))) {free(data); return NULL;}
	free(data);
//	printf("done\r\nwaiting response...");
	if (at_wait("+CHTTPS: RECV EVENT\r\n", "+CHTTPSNOTIFY: PEER CLOSED\r\n")) {
//		printf("peer closed\r\n");
		return NULL;
	}

	do {
		if (at_cmd(recv_https, &recv, "%s?\r\n", at_cmd_https_recv)) return NULL;
	} while (recv < 17);

//	printf("done\r\nwill be receive %d bytes\r\n", recv);

	data = (char*)malloc(recv+3);
	if (data != NULL) {
		int len;
//		printf("buffer allocated\r\n");
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
//		printf("data received\r\n");
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

	fd = gsm_init();
//	printf("opening network...");
	if (at_cmd(NULL, NULL, "%s", at_cmd_https_start)) ON_ERROR("error!\r\n");
	if (at_cmd(NULL, NULL, "%s\"%s\",%d\r\n", at_cmd_https_open, https_host, port)) ON_ERROR("error!\r\n");
//	printf("done\r\n");

	if (pdFAIL == get_tank_token(610, &tank, &token)) ON_ERROR("error get_tank_token\r\n");
/*
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
*/
/*---------------------get dispancer--------------------------------------*/	
//	printf("\r\n\r\n");

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
//	printf("\r\n");
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
/*	if (jobj != NULL) {
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
*/
	cJSON_Delete(jobj);

network_close:
//	printf("close network...");
	at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);

	if (status == HTTPS_STATE_OPENED_SESSION) at_cmd(NULL, NULL, "%s", at_cmd_https_close);
	at_cmd(NULL, NULL, "%s", at_cmd_https_stop);
//	printf("done\r\n");
	vTaskDelete(NULL);
}
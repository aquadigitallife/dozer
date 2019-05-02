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
static int mode = INTERVAL_MODE;
static bool tank_changed = false;

/*
char *read_token(void)
{
	char *retptr;
	uint16_t taddr;
	if (pdFAIL == ee_read(TOKENADDR_ADDR, sizeof(taddr), &taddr)) return NULL;
	retptr = (char*)malloc(EEPROM_SIZE - taddr + 1);
	if (retptr == NULL) return NULL;
	if (pdFAIL == ee_read(taddr, EEPROM_SIZE - taddr, retptr)) return NULL;
	retptr[EEPROM_SIZE - taddr] = 0;

	return NULL;
}
*/

void tank_change(void)
{
	tank_changed = true;
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

char *read_tank_num(void)
{
	char *cTankNum;
	uint8_t numlen;
	ee_read(TANK_NUM_ADDR, 1, &numlen);
	if (numlen < 51) {
		cTankNum = (char*)malloc(numlen+1);
		if (cTankNum == NULL) return NULL;
		ee_read(TANK_NUM_ADDR+1, numlen, cTankNum);
		cTankNum[numlen] = 0;
	} else cTankNum = NULL;
	return cTankNum;
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

BaseType_t get_tank_token(cJSON **tank, char **token)
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
	if (jtoken == NULL) goto error;
	
	request = read_tank_num();
	if (request == NULL) goto error;
	printf("tank number: %s\r\n", request);
	
	cJSON_ArrayForEach(jitem, jtoken)
	{
		cJSON *ch = cJSON_GetObjectItemCaseSensitive(jitem, "cCodeTank");
		if (ch == NULL) continue; 
		if (0 == strcmp(request, ch->valuestring)) {
			*tank = cJSON_Duplicate(jitem, 1);
			free(request);
			goto good;
		}
	}
	free(request);
error:
	cJSON_Delete(json);
	return pdFAIL;
good:
	cJSON_Delete(json);
	return pdPASS;
}

BaseType_t get_next_action_from_shedule(cJSON *jobj, struct dozer_action *result)
{
	int tg;
	struct dozer_action cur, prev;
	struct date_time now;
	double daily;
	
	if (jobj == NULL) return pdFAIL;
	
	cJSON *json = cJSON_GetObjectItemCaseSensitive(jobj, "nDailyDose");
	if (json == NULL) daily = 0.0;
	else daily = json->valuedouble;

	json = cJSON_GetObjectItemCaseSensitive(jobj, "cSchedule");
	if (json == NULL) return pdFAIL;

	get_sys_date(&now);

	cur.day = now.day;
	cur.actual = 0;

	prev.day = now.day;

	for (int j = 0; j < 2; j++) {
		int i;
		const cJSON *sitem, *ch;
//		printf("iter %d\r\n", j);
		prev.hours = 24;
		prev.minutes = 0;
		i = 0; tg = 0;
		cJSON_ArrayForEach(sitem, json)
		{
			ch = cJSON_GetObjectItemCaseSensitive(sitem, "nTime");
			if (ch == NULL) continue;
			cur.hours = ch->valueint;
			if (cur.hours == 24) cur.hours = 0;
			ch = cJSON_GetObjectItemCaseSensitive(sitem, "nMinutes");
			if (ch == NULL) continue;
			cur.minutes = ch->valueint;

			ch = cJSON_GetObjectItemCaseSensitive(sitem, "nPersentage");
			if (ch == NULL) cur.doze = 0;
			else cur.doze = daily*ch->valuedouble/100.0;

			i++;
			if (true == is_action_trigged(&cur, &now)) {
//				printf("action triggered: %d\r\n", i);
				if (0 < action_compare(&prev, &cur)) {
					memcpy(&prev, &cur, sizeof(struct dozer_action));
					tg = i;
				}
			}
		}
		if (tg != 0) {
//			printf("action gt %d %d:%d:%d\r\n", tg, prev.day, prev.hours, prev.minutes);
			prev.actual = tg;
			memcpy(result, &prev, sizeof(struct dozer_action));
			return pdPASS;
		}
		cur.day = day_inc(cur.day);
		prev.day = day_inc(prev.day);
	}
	return pdFAIL;
}

BaseType_t get_next_action_from_interval(cJSON *jobj, struct dozer_action *prev, struct dozer_action *result)
{
	uint8_t hours, minutes;
	double daily;
	struct dozer_action cur;
	struct date_time now;

	cJSON *json = cJSON_GetObjectItemCaseSensitive(jobj, "nScheduleH");
	if (json == NULL) return pdFAIL;
	hours = json->valueint;
	json = cJSON_GetObjectItemCaseSensitive(jobj, "nScheduleM");
	if (json == NULL) return pdFAIL;
	minutes = json->valueint;
	
	if (hours == 0 && minutes == 0) return pdFAIL;
	
	json = cJSON_GetObjectItemCaseSensitive(jobj, "nDailyDose");
	if (json == NULL) daily = 0.0;
	else daily = json->valuedouble;
	
	
	if (mode == INTERVAL_MODE)
		memcpy(&cur, prev, sizeof(struct dozer_action));
	else {
		get_sys_date(&now);
		cur.actual = 0;
		cur.day = now.day;
		cur.hours = now.hours;
		cur.minutes = now.minutes;
		mode = INTERVAL_MODE;
	}
	
	cur.doze = daily*(double)get_minutes(hours, minutes)/1440.0;

	if (cur.actual == 0) {
		
		get_sys_date(&now);

		while (false == is_action_trigged(&cur, &now)) {
			add_minutes(&cur, get_minutes(hours, minutes));
			get_sys_date(&now);
		}
		cur.actual = 0xFF;
	}
//	printf("action period %02d:%02d:%02d\r\n", cur.day, cur.hours, cur.minutes);
	memcpy(result, &cur, sizeof(struct dozer_action));
	return pdPASS;
}

void https_start(void *Param)
{
	https_state_enum status;

//	static const int tankId[28] = { 595, 610, 611, 612, 613, 614, 615, 616, 619, 620, 622, 624, 627, 628, 629, 631, 632, 633, 742, 743, 745, 772, 773, 913, 1994, 1995, 1996, 2004 };

	char *token = NULL;
	char *request, *response;
	cJSON *json;
	cJSON *tank = NULL;
	cJSON *jobj = NULL;

	fd = gsm_init();
	printf("opening network...");
	if (at_cmd(NULL, NULL, "%s", at_cmd_https_start)) ON_ERROR("error!\r\n");
	printf("done\r\n");
	
	while (true) {
		int tankId;
		printf("connect to server...");
		if (at_cmd(NULL, NULL, "%s\"%s\",%d\r\n", at_cmd_https_open, https_host, port)) ON_ERROR("error!\r\n");
		printf("done\r\n");
		
		while (pdFAIL == get_tank_token(&tank, &token)) {
			vTaskDelay(MS_TO_TICK(60000));
			at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);
			if (status != HTTPS_STATE_OPENED_SESSION) ON_ERROR("peer closed\r\n");
			printf("repeat get_tank_token\r\n");
		}
//		show_json_obj(tank);
		jobj = cJSON_GetObjectItemCaseSensitive(tank, "nTankId");
		if (jobj == NULL) {printf("no tank ID found!\r\n"); tankId = 610;}
		tankId = jobj->valueint;
		printf("tank ID: %d\r\n", tankId);
		
		tank_changed = false;

		while(!tank_changed) {
			int hcode;
/*---------------------get dispancer--------------------------------------*/	
//			printf("\r\n\r\n");

			json = cJSON_CreateObject();
		
			if (json == NULL) ON_ERROR("no memory!\r\n");
			if (cJSON_AddNumberToObject(json, "nTankId", tankId) == NULL) {cJSON_Delete(json); ON_ERROR("no memory!\r\n");}

			request = cJSON_PrintUnformatted(json);
			cJSON_Delete(json);
		
			if (request == NULL) ON_ERROR("no memory!\r\n");

			response = https_request(cmd_get_dispancer, request, token);
			free(request);
		
			if (response == NULL) ON_ERROR("error get_dispancer\r\n");
	
			sscanf(response, "HTTP/1.1 %d OK", &hcode);
			if (hcode != 200) {
				free(response);
				ON_ERROR("http ret: %d\r\n", hcode);
			}
//			printf("\r\n");
//			printf("%s", strstr(response, "{"));
			json = cJSON_Parse(strstr(response, "{"));

			if (json == NULL) {
				const char *error_ptr = cJSON_GetErrorPtr();
				if (error_ptr != NULL) printf("error before: %s\n", error_ptr);
				free(response);
				ON_ERROR("json parse error\r\n");
			}
			free(response);

			jobj = cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(json, "dispanser"), 1);
			cJSON_Delete(json);
			
//			show_json_obj(jobj);
/*----------------------------Update action------------------------------*/	
				
			if (pdFAIL == get_next_action_from_interval(jobj, &next_action, &next_action)) {	// работа по расписанию
				struct dozer_action action;
				mode = SHEDULE_MODE;
				if (pdPASS == get_next_action_from_shedule(jobj, &action)) {
					if (0 != action_compare(&next_action, &action))
						memcpy(&next_action, &action, sizeof(struct dozer_action));
					else if (next_action.actual != 0)
						memcpy(&next_action, &action, sizeof(struct dozer_action));
				}
			}

//			printf("current time: %d:%d:%d\r\n", now.day, now.hours, now.minutes);
//			printf("next action: %d:%d:%d doze: %.3f\r\n", next_action.day, next_action.hours, next_action.minutes, next_action.doze);
/*------------------------------------------------------------------------*/
			json = cJSON_GetObjectItemCaseSensitive(jobj, "nVolume");
			cJSON_SetNumberHelper(json, get_weight());
			
			request = cJSON_PrintUnformatted(jobj);
			
			response = https_request(cmd_set_dispancer, request, token);
			free(request);

			if (response != NULL) {
				sscanf(response, "HTTP/1.1 %d OK", &hcode);
				if (hcode != 200) printf("server error: %d\r\n", hcode);
				free(response);
			} else printf("no response\r\n");

			vTaskDelay(MS_TO_TICK(60000));
			cJSON_Delete(jobj);

			at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);
			if (status != HTTPS_STATE_OPENED_SESSION) ON_ERROR("peer closed\r\n");
		}
network_close:
		if (tank != NULL) { free(tank); tank = NULL; } if (token != NULL) { free(token); token = NULL; }
//		printf("close network...");
		at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);

		if (status == HTTPS_STATE_OPENED_SESSION) at_cmd(NULL, NULL, "%s", at_cmd_https_close);
	}
	at_cmd(NULL, NULL, "%s", at_cmd_https_stop);
//	printf("done\r\n");
	vTaskDelete(NULL);
}
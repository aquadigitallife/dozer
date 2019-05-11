/*
	Модуль связи с сервером по GSM
*/

#include "Global.h"

#define ON_ERROR(...) do { \
goto network_close; \
} while(0)

//printf(__VA_ARGS__);

static const char https[] = "https://";
static const char https_host[] = "api.aquadigitallife.com";		// адрес сервера. В будущем сделать программируемым
static const int port = 443;									// порт https. В будущем сделать программируемым

static const char cmd_auth[] = "/v1/auth/login";					// команда авторизации и получения информации о бассейнах
static const char cmd_get_dispancer[] = "/v1/tank/dispanser";		// команда получения информации о параметрах кормушки
static const char cmd_set_dispancer[] = "/v1/tank/update-dispenser";// команда установки параметров кормушки

static FILE *fd;						// дескриптор файла UART обмена с SIM5320
static uint32_t mode = UNDEFINED_MODE;	// режим работы кормушки (ручной, через интервал, по расписанию)
static bool tank_changed = false;		// признак изменения параметров соединения с сервером

/*
Функция сигнализации об изменении параметров связи с сервером
*/
void tank_change(void)
{
	tank_changed = true;
}

/*
Функция чтения строки из eeprom
addr - адрес хранения строки в eeprom
len - максимально возможная длина строки
если длина строки, считанная из eeprom, больше len
(например при отсутствии записи в eeprom считанная длина равна 0xFF)
функция возвращает NULL.
При успешном завершении, функция возвращает указатель на строку в "куче",
которая после использования должна быть освобождена функцией free
*/
char *read_eeprom_string(uint16_t addr, size_t len)
{
	char *str;
	uint8_t plen;
	ee_read(addr, 1, &plen);
	if (plen < (len+1)) {
		str = (char*)malloc(plen+1);
		if (str == NULL) return NULL;
		ee_read(addr+1, plen, str);
		str[plen] = 0;
	} else str = NULL;
	return str;
}

/*
Чтение строки ИНН
При неудаче, в eeprom записывается ИНН по умолчанию
(в будущем нужно записывать и пароль по умолчанию)
*/
char *read_inn(void)
{
	static const uint8array pinn = { 11 ,"11111111111" };
	char *inn = read_eeprom_string(INN_ADDR, INN_MAX_LEN);
	if (inn == NULL) {
		ee_write(INN_ADDR, pinn.len+1, &pinn);
		inn = (char*)malloc(pinn.len+1);
		if (inn == NULL) return NULL;
		memcpy(inn, &pinn.data, pinn.len);
		inn[pinn.len] = 0;
		printf("default inn written\r\n");
	}
	return inn;
}

/*
Чтение строки пароля
При неудаче, в eeprom записывается пароль по умолчанию
(в будущем нужно записывать и ИНН по умолчанию)
*/
char *read_passwd(void)
{
	static const uint8array ppass = { 4 ,"demo" };
	char *psw = read_eeprom_string(PSW_ADDR, PSW_MAX_LEN);
	if (psw == NULL) {
		ee_write(PSW_ADDR, ppass.len+1, &ppass);
		psw = (char*)malloc(ppass.len+1);
		if (psw == NULL) return NULL;
		memcpy(psw, &ppass.data, ppass.len);
		psw[ppass.len] = 0;
		printf("default passwd written\r\n");
	}
	return psw;
}

/*
Чтение строки номера бассейна
*/
portINLINE char *read_tank_num(void)
{
	return read_eeprom_string(TANK_NUM_ADDR, TANK_NUM_MAX_LEN);
}

/*
Функция добавляет к данным запроса http-заголовок метода POST
Возвращает строку запроса с заголовком.
После использования необходимо освободить память функцией free
*/
char *add_http_header(const char *addr, const char *request, const char *token)
{
	int hdr_len;
	char req_len[25];
	char *retval, *post, *host;

	post = (char*)malloc(17 + strlen(https) + strlen(https_host) + strlen(addr));	// выделяем память под первую строку заголовка
	if (post == NULL) return NULL;
	host = (char*)malloc(9 + strlen(https) + strlen(https_host) + strlen(addr));	// выделяем память под вторую строку заголовка
	if (host == NULL) { free(post); return NULL; }
	
	sprintf(post, "POST %s%s%s HTTP/1.1\r\n", https, https_host, addr);	// формируем первую строку заголовка
	sprintf(host, "HOST: %s%s%s\r\n", https, https_host, addr);			// формируем вторую строку заголовка
	sprintf(req_len, "Content-Length: %d\r\n", strlen(request) - 2);	// формируем третью строку заголовка
	
	hdr_len = strlen(post) + strlen(host) + strlen(req_len) + strlen(request) + 3; // вычисляем общую длину заголовка
	if (token != NULL) hdr_len += strlen(token); // если к заголовку необходимо добавить токен, к общей длине добавляем длину токена
	
	retval = (char*)malloc(hdr_len);	// выделяем память под заголовок
	if (retval == NULL) { free(post); free(host); return NULL; }
	
	if (token == NULL) sprintf(retval, "%s%s%s\r\n%s", post, host, req_len, request);	// если токена нет, формируем заголовок без токена
	else sprintf(retval, "%s%s%s%s\r\n%s", post, host, token, req_len, request);	// иначе заголовок с токеном
	free(post); free(host);

	return retval;
}

/*
Функия отправляет запрос серверу.
Возвращает ответ сервера. После использования
необходимо освободить динамически выделенную память функцией free
*/
char *https_request(const char *addr, const char *request, const char *token)
{
	int recv = 0;
	char *data = add_http_header(addr, request, token);	// получаем запрос с заголовком

	if (data == NULL) return NULL;
	
//	printf("sending request...");
	if (at_cmd(send_string, data, "%s%d\r\n", at_cmd_https_send, strlen(data))) {free(data); return NULL;}	// отсылаем запрос серверу
	free(data);
//	printf("done\r\nwaiting response...");
	if (at_wait("+CHTTPS: RECV EVENT\r\n", "+CHTTPSNOTIFY: PEER CLOSED\r\n")) {	// ожидаем ответа от сервера
//		printf("peer closed\r\n");
		return NULL;
	}

	do {
		if (at_cmd(recv_https, &recv, "%s?\r\n", at_cmd_https_recv)) return NULL;
	} while (recv < 17);	// ожидаем пока не придёт весь ответ (минимальная длина ответа = 17 байт)

//	printf("done\r\nwill be receive %d bytes\r\n", recv);

	data = (char*)malloc(recv+3);	// выделяем память под ответ сервера
	if (data != NULL) {
		int len;
//		printf("buffer allocated\r\n");
		for (int i = 0; i < recv; i += len) {		// запрашиваем ответ у модема порциями по 32 байта
			if (at_cmd(NULL, NULL, "%s=32\r\n", at_cmd_https_recv)) {
				free(data);
				return NULL;
			}
				
			len = 0;
			do {
				fgets(answer, sizeof(answer), fd);
				fgets(answer, sizeof(answer), fd);
				sscanf(answer, "+CHTTPSRECV: DATA,%d\r\n", &len);
			} while (len == 0);		// получаем ответ модема сколько байт он готов отдать
//			printf("read %d bytes\r\n", len);
			fread(&data[i], len+2, 1, fd);	// читаем данные в data
			data[i+len+2] = '\0';

			fgets(answer, sizeof(answer), fd);
		}
//		printf("data received\r\n");
	}
	return data;
}

/*
Функция авторизуется на сервере, получает токен, список бассейнов,
ищет в списке бассейнов по номеру tank_num свой бассейн, сохраняет информацию
о своём бассейне в отдельный объект JSON
*/
BaseType_t get_tank_token(const char *tank_num, cJSON **tank, char **token)
{
	char *request, *response;
	cJSON *jtoken, *jitem;
	char *inn = read_inn();					// читаем ИНН
	char *passwd = read_passwd();			// читаем пароль
	cJSON *json = cJSON_CreateObject();		// создаём JSON-объект для формирования запроса
	
	*tank = NULL; *token = NULL;
	
	if (json == NULL) return pdFAIL;
	// формируем json-строку запроса
	cJSON_AddItemReferenceToObject(json, "INN", cJSON_CreateStringReference(inn));
	cJSON_AddItemReferenceToObject(json, "Password", cJSON_CreateStringReference(passwd));
	request = cJSON_PrintUnformatted(json);
	if (inn) free(inn);
	if (passwd) free(passwd);
	cJSON_Delete(json);
	if (request == NULL) return pdFAIL;
	
	response = https_request(cmd_auth, request, NULL);	// посылаем запрос
	free(request);
	if (response == NULL) return pdFAIL;
	
	json = cJSON_Parse(strstr(response, "{"));	// парсим ответ, создаётся JSON-объект с данными о токене и списке бассейнов
	free(response);
	if (json == NULL) return pdFAIL;
	
	jtoken = cJSON_GetObjectItemCaseSensitive(json, "_Autorize");	// находим информацию о токене
	if (jtoken == NULL) goto error;
	if (jtoken->valuestring == NULL) goto error;
	if (strlen(jtoken->valuestring) == 0) goto error;
	
	*token = (char*)malloc(strlen(jtoken->valuestring) + 15);		// выделяем память под хранение токена
	sprintf(*token, "_Authorize: %s\r\n", jtoken->valuestring);		// формируем строку http-заголовка с токеном
	
	jtoken = cJSON_GetObjectItemCaseSensitive(json, "Tanks");		// находим информацию о бассейнах
	if (jtoken == NULL) goto error;
	
//	request = read_tank_num();
	if (tank_num == NULL) goto error;
	printf("tank number: %s\r\n", tank_num);						// отладочный вывод
	
	cJSON_ArrayForEach(jitem, jtoken)								// ищем в списке свой бассейн
	{
		cJSON *ch = cJSON_GetObjectItemCaseSensitive(jitem, "cCodeTank");
		if (ch == NULL) continue; 
		if (0 == strcmp(tank_num, ch->valuestring)) {
			*tank = cJSON_Duplicate(jitem, 1);						// копируем данные о своём бассейне в отдельную область памяти
			goto good;
		}
	}
error:
	cJSON_Delete(json);
	return pdFAIL;
good:
	cJSON_Delete(json);
	return pdPASS;
}

/*
Функция ищет в JSON-объекте dispencer jobj подобъект расписания,
в нём ищет ближайшее к текущему времени, событие
и формирует по нему структуру по указателю result
*/
BaseType_t get_next_action_from_shedule(cJSON *jobj, struct dozer_action *result)
{
	int tg;							// признак того что запись в расписании найдена
	struct dozer_action cur, prev;	// две структуры для поиска ближайшего к текущему времени события расписания
	struct date_time now;			// структура хранения текущего времени
	double daily;					// суточная доза
	
	if (jobj == NULL) return pdFAIL;
	// считываем величину суточной дозы
	cJSON *json = cJSON_GetObjectItemCaseSensitive(jobj, "nDailyDose");
	if (json == NULL) daily = 0.0;
	else daily = json->valuedouble*1000.0;	// переводим килограммы в граммы

	json = cJSON_GetObjectItemCaseSensitive(jobj, "cSchedule");	// находим расписание
	if (json == NULL) return pdFAIL;

	get_sys_date(&now);	// читаем текущее время

	cur.day = now.day;
	cur.actual = 0;

	prev.day = now.day;

	for (int j = 0; j < 2; j++) {	// цикл из двух итераций, сегодняшний день и до текущего времени завтрашнего дня
		int i;
		const cJSON *sitem, *ch;
//		printf("iter %d\r\n", j);
		prev.hours = 24;	// стартовое значение времени события - максимально "невозможное"
		prev.minutes = 0;
		i = 0; tg = 0;
		cJSON_ArrayForEach(sitem, json)		// проходим по всему расписанию
		{
			// читаем время очередной записи в расписании
			ch = cJSON_GetObjectItemCaseSensitive(sitem, "nTime");
			if (ch == NULL) continue;
			cur.hours = ch->valueint;
			if (cur.hours == 24) cur.hours = 0;
			ch = cJSON_GetObjectItemCaseSensitive(sitem, "nMinutes");
			if (ch == NULL) continue;
			cur.minutes = ch->valueint;
			// читаем процент от суточной дозы, которую нужно выдать в это время
			ch = cJSON_GetObjectItemCaseSensitive(sitem, "nPersentage");
			if (ch == NULL) cur.doze = 0;
			else cur.doze = daily*ch->valuedouble/100.0;	// вычисляем дозу расписания

			i++;
			if (true == is_action_trigged(&cur, &now)) {	// если время расписания больше текущего времени
//				printf("action triggered: %d\r\n", i);
				if (0 < action_compare(&prev, &cur)) {		// если текущее время ближе к текущему времени чем предыдущая запись расписания
					memcpy(&prev, &cur, sizeof(struct dozer_action));	// назначаем ближайшей к текущему времени данную запись
					tg = i;
				}
			}
		}
		if (tg != 0) {	// если ближайшая к текущему времени запись найдена формируем result и выходим
//			printf("action gt %d %d:%d:%d\r\n", tg, prev.day, prev.hours, prev.minutes);
			prev.actual = tg;
			memcpy(result, &prev, sizeof(struct dozer_action));
			return pdPASS;
		}
		cur.day = day_inc(cur.day);	// иначе ищем в следующем дне
		prev.day = day_inc(prev.day);
	}
	return pdFAIL;	// ничего не нашли или расписания нет.
}

/*
Функция читает из JSON-объекта dispencer величину интервала выдачи, суточную дозу
и вычисляет время события.
*/
BaseType_t get_next_action_from_interval(cJSON *jobj, struct dozer_action *prev, struct dozer_action *result)
{
	uint8_t hours, minutes;		// сюда считываем часы и минуты интервала
	double daily;				// суточная доза
	struct dozer_action cur;	// структура для вычисления времени события
	struct date_time now;		// структура хранения текущего времени

	// Читаем интервал из json-объекта jobj
	cJSON *json = cJSON_GetObjectItemCaseSensitive(jobj, "nScheduleH");
	if (json == NULL) return pdFAIL;
	hours = json->valueint;
	json = cJSON_GetObjectItemCaseSensitive(jobj, "nScheduleM");
	if (json == NULL) return pdFAIL;
	minutes = json->valueint;
	
	if (hours == 0 && minutes == 0) return pdFAIL;
	// Читаем суточную дозу
	json = cJSON_GetObjectItemCaseSensitive(jobj, "nDailyDose");
	if (json == NULL) daily = 0.0;
	else daily = json->valuedouble*1000.0;	// переводим её в граммы
	
	
	if (mode == INTERVAL_MODE)
		memcpy(&cur, prev, sizeof(struct dozer_action));	// если уже в режиме выдачи по интервалу, инициализируем структуру вычисления последним событием
	else {							// иначе инициализируем структуру вычисления текущим временем
		get_sys_date(&now);
		cur.actual = 0;
		cur.day = now.day;
		cur.hours = now.hours;
		cur.minutes = now.minutes;
		mode = INTERVAL_MODE;
	}
	
	cur.doze = daily*(double)get_minutes(hours, minutes)/1440.0;	//вычисляем дозу выдачи

	if (cur.actual == 0) {	// если последнее событие уже произошло
		
		get_sys_date(&now);

		while (false == is_action_trigged(&cur, &now)) {	// добавляем ко времени последнего события
			add_minutes(&cur, get_minutes(hours, minutes));	// время интервала до тех пор, пока время
			get_sys_date(&now);								// события не станет больше текущего
		}
		cur.actual = 0xFF;									// помечаем событие как актуальное
		printf("next period %02d:%02d:%02d doze: %.3f\r\n", cur.day, cur.hours, cur.minutes, cur.doze);
	}
	memcpy(result, &cur, sizeof(struct dozer_action));		// заполняем структуру результата
	return pdPASS;
}

/*
Функция проверяет, действует ручной режим или нет.
*/
bool is_action_manual(cJSON *jobj)
{
	cJSON *json = cJSON_GetObjectItemCaseSensitive(jobj, "bManual");
	if (json == NULL) return false;
	if (json->type == cJSON_True) return true;
	return false;
}
/*
Процесс обмена информацией с сервером
*/
void httpsProc(void *Param)
{
	https_state_enum status;	// состояние https-соединения

//	static const int tankId[28] = { 595, 610, 611, 612, 613, 614, 615, 616, 619, 620, 622, 624, 627, 628, 629, 631, 632, 633, 742, 743, 745, 772, 773, 913, 1994, 1995, 1996, 2004 };

	char *token = NULL;			// указатель на строку токена
	char *request, *response;	// указатели на строки https-запроса и ответа
	cJSON *json;				// JSON-объекты разного назначения
	cJSON *tank = NULL;			// объект хранения информации о бассейне
	cJSON *jobj = NULL;			// JSON-объекты разного назначения

	fd = gsm_init();			// Инициализируем GSM-модем
	printf("opening network...");
	if (at_cmd(NULL, NULL, "%s", at_cmd_https_start)) ON_ERROR("error!\r\n");	// инициализируем https-стек
	printf("done\r\n");
	
	while (true) {				// внешний цикл обмена с gsm-сервером. Выход из цикла отсутствует
		int tankId;				// идентификатор бассейна
		char *tankNum;			// строка номера бассейна
		struct date_time now;	// текущие дата и время
		printf("connect to server...");
		if (at_cmd(NULL, NULL, "%s\"%s\",%d\r\n", at_cmd_https_open, https_host, port)) ON_ERROR("error!\r\n");	// соединяемся с сервером
		printf("done\r\n");
		
		while (true) {			// цикл повторяется, пока пользователь не введёт правильные ИНН, пароль и номер бассейна
			tankNum = read_tank_num();												// читаем номер бассейна из eeprom
			if (pdPASS == get_tank_token(tankNum, &tank, &token)) break;			// авторизуемся на сервере, получаем токен и информацию о бассейне. Если успешно - выходим из цикла
			if (tankNum) free(tankNum);
			vTaskDelay(MS_TO_TICK(30000));											// ждём 30 секунд
			at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);		// проверяем статус соединения
			if (status != HTTPS_STATE_OPENED_SESSION) ON_ERROR("peer closed\r\n");	// если соединение оборвано, переподключаемся
			printf("repeat get_tank_token\r\n");									// иначе пытаемся повторно авторизоваться
		}
		
		for (int i = 0; i < 20; i++) {							// ждём готовности BLE
			if (is_ble_ready()) break;
			vTaskDelay(MS_TO_TICK(500));
		}

		if (is_ble_ready()) {									// если BLE готово...
			xSemaphoreTake( ble_write_lock, portMAX_DELAY );
			if (tankNum) {
				gecko_cmd_gatt_server_write_attribute_value(gattdb_pool_number, 0x0000, strlen(tankNum), (const uint8_t*)tankNum);	// ...показываем смартфону запрограммированный номер басейна
				free(tankNum);
			}
			tankNum = read_inn();
			if (tankNum)
				gecko_cmd_gatt_server_write_attribute_value(gattdb_inn, 0x0000, strlen(tankNum), (const uint8_t*)tankNum);			// ... и ИНН

			xSemaphoreGive( ble_write_lock );
		}
		if (tankNum) free(tankNum);
//		show_json_obj(tank);

		// читаем идентификатор бассейна
		jobj = cJSON_GetObjectItemCaseSensitive(tank, "nTankId");
		if (jobj == NULL) ON_ERROR("no tank ID found!\r\n");
		tankId = jobj->valueint;
		printf("tank ID: %d\r\n", tankId);
		
		get_sys_date(&now);					// читаем текущую дату и время
		// инициализируем структуру ближайшего события
		next_action.actual = 0;
		next_action.day = now.day;
		next_action.hours = now.hours;
		next_action.minutes = now.minutes;
		
		tank_changed = false;				// инициализируем флаг смены коммуникационных параметров

		while(!tank_changed) {				// пока не поменяются коммуникационные параметры, общаемся с сервером
			int hcode;						// код возврата сервера
/*---------------------Получаем объект dispancer--------------------------*/	
//			printf("\r\n\r\n");
			// Формируем запрос к серверу о параметрах кормушки
			json = cJSON_CreateObject();
			if (json == NULL) ON_ERROR("no memory!\r\n");
			if (cJSON_AddNumberToObject(json, "nTankId", tankId) == NULL) {cJSON_Delete(json); ON_ERROR("no memory!\r\n");}
			request = cJSON_PrintUnformatted(json);
			cJSON_Delete(json);
			if (request == NULL) ON_ERROR("no memory!\r\n");

			// посылаем запрос, принимаем ответ
			response = https_request(cmd_get_dispancer, request, token);
			free(request);
			if (response == NULL) ON_ERROR("error get_dispancer\r\n");

			// читаем код возврата сервера
			sscanf(response, "HTTP/1.1 %d OK", &hcode);
			if (hcode != 200) {		// только с кодом 200 всё хорошо
				free(response);
				ON_ERROR("server error1: %d\r\n", hcode);
			}
//			printf("\r\n");
//			printf("%s", strstr(response, "{"));
			json = cJSON_Parse(strstr(response, "{"));	// создаём JSON-объект кормушки

			if (json == NULL) {							// что-то пошло не так. Выясняем, сообщаем
				const char *error_ptr = cJSON_GetErrorPtr();
				if (error_ptr != NULL) printf("error before: %s\n", error_ptr);
				free(response);
				ON_ERROR("json parse error\r\n");
			}
			free(response);

			jobj = cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(json, "dispanser"), 1);	// выделяем объект dispanser
			cJSON_Delete(json);																// остальное удаляем
			
//			show_json_obj(jobj);
/*--------------------Обновляем структуру next_action---------------------*/	
			if (is_action_manual(jobj)) {				// если действует ручной режим
				if (mode != MANUAL_MODE) {				// а до этого запроса был другой режим
					mode = MANUAL_MODE;					// переключаемся в ручной режим
					taskENTER_CRITICAL();
					next_action.actual = 0;				// ближайшее событие не актуально
					motor1_on = 0xFF; purge_on = 0xFF;	// включаем выдачу
					taskEXIT_CRITICAL();
					printf("dispancer on\r\n");
				}
			} else {									// если ручной режим выключен
				if (mode == MANUAL_MODE) {				// а до этого запроса был выключен
					mode = UNDEFINED_MODE;				// переходим в неопределённый режим
					taskENTER_CRITICAL();
					motor1_on = 0; purge_on = 0;		// прекращаем выдачу
					taskEXIT_CRITICAL();
					printf("dispancer off\r\n");
				}
				if (pdFAIL == get_next_action_from_interval(jobj, &next_action, &next_action)) {	// если информации о периоде выдачи нет, 
					struct dozer_action action;
					mode = SHEDULE_MODE;															// то работа по расписанию
					if (pdPASS == get_next_action_from_shedule(jobj, &action)) {					// читаем расписание, выбираем ближайшее к текущему времени событие
						if (0 != action_compare(&next_action, &action)) {							// если выбранное событие не совпадает с ближайшим
							memcpy(&next_action, &action, sizeof(struct dozer_action));				// обновляем next_action
							printf("next shedule %02d:%02d:%02d doze: %.3f\r\n", action.day, action.hours, action.minutes, action.doze);
						} else if (next_action.actual != 0)											// даже если совпадает, но актуально
							memcpy(&next_action, &action, sizeof(struct dozer_action));				// то тоже обновляем (вдруг доза изменилась?)
					}
				}
			}
//			printf("current time: %d:%d:%d\r\n", now.day, now.hours, now.minutes);
//			printf("next action: %d:%d:%d doze: %.3f\r\n", next_action.day, next_action.hours, next_action.minutes, next_action.doze);
/*----------------------Сообщаем серверу об остатке корма-----------------------------*/
			json = cJSON_GetObjectItemCaseSensitive(jobj, "nVolume");
			cJSON_SetNumberHelper(json, get_weight()/1000.0);
			request = cJSON_PrintUnformatted(jobj);
			cJSON_Delete(jobj);
			response = https_request(cmd_set_dispancer, request, token);
			free(request);

			// проверяем, всё-ли прошло нормально
			if (response != NULL) {
				sscanf(response, "HTTP/1.1 %d OK", &hcode);
				if (hcode != 200) printf("server error2: %d\r\n", hcode);
				free(response);
			} else printf("no response\r\n");


			vTaskDelay(MS_TO_TICK(30000));											// пауза 30 секунд

			at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);		// если соединение разорвано
			if (status != HTTPS_STATE_OPENED_SESSION) ON_ERROR("peer closed\r\n");	// восстанавливаем
		}
network_close:
		if (tank != NULL) { free(tank); tank = NULL; } if (token != NULL) { free(token); token = NULL; }	// освобождаем память
//		printf("close network...");
		at_cmd(https_state_callback, &status, "%s\n", at_cmd_https_state);			// проверяем статус соединения

		if (status == HTTPS_STATE_OPENED_SESSION) at_cmd(NULL, NULL, "%s", at_cmd_https_close);	// если соединение есть, разрываем
	}
	at_cmd(NULL, NULL, "%s", at_cmd_https_stop);
//	printf("done\r\n");
	vTaskDelete(NULL);
}
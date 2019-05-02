/*
	Модуль работы с bluetooth LE
*/

#include "Global.h"

/* BG stack headers */
#include "gecko_bglib.h"
#include "gatt_db.h"

#include "i2c.h"

BGLIB_DEFINE();		// объявление структур для работы bglib

// App booted flag
static bool appBooted = false;	// признак готовности модуля BLE

int appHandleEvents(struct gecko_cmd_packet *evt);	// функция обработки сообщений от BLE
/*
	Процесс обработки сообщений от BLE и сообщений других модулей (RTC, AD7799),
	предназначенных для BLE
*/
void BLEProc(void *Param)
{
	struct gecko_cmd_packet* evt;	// структура хранения сообщений от BLE

  /* Инициализация BGLIB */
	BGLIB_INITIALIZE_NONBLOCK(BLEUartTx, BLEUartRx, BLEUartPeek);

	InitBLEUart(BGLIB_MSG_MAX_PAYLOAD+4);	// Инициализация UART BLE
	

	gecko_cmd_system_reset(0);	// сброс BLE модуля
	while (1) {

		evt = gecko_peek_event();	// проверяем на наличие сообщений от BLE
		if (NULL != evt) {	// если сообщение принято
			if (appHandleEvents(evt) == -1) continue;	// обрабатываем сообщение. В случае кода возврата -1 - признак неготовности модуля BLE
														// возвращаемся на ожидание сообщения
		}
		taskYIELD();
		if (appBooted == false) continue;				// возможно избыточность
		
		if (RTC_Queue != NULL) {		// проверка готовности очереди RTC
			struct date_time rtc;
			if (pdPASS == xQueueReceive(RTC_Queue, &rtc, 0)) {	// если пришло текущее время от RTC
			/* записываем время в БД BLE */
				gecko_cmd_gatt_server_write_attribute_value(gattdb_date_time, 0x0000, sizeof(struct date_time), (const uint8_t*)&rtc);
			/* Уведомляем об этом смартфон */
				gecko_cmd_gatt_server_send_characteristic_notification(0xFF, gattdb_date_time, sizeof(struct date_time), (const uint8_t*)&rtc);
			}
		}
		taskYIELD();
		if (AD7799_Queue != NULL) {	// проверка готовности очереди тензодатчика
			uint8_t ad_id[6] = {0,0,0,0,0,0};
			if (pdPASS == xQueueReceive(AD7799_Queue, &ad_id, 0)) {	// если пришло сообщение о текущем весе корма
			/* записываем в БД BLE */
				gecko_cmd_gatt_server_write_attribute_value(gattdb_op_time, 0x0000, sizeof(ad_id), (const uint8_t*)&ad_id);
			/* Уведомляем об этом смартфон */
				gecko_cmd_gatt_server_send_characteristic_notification(0xFF, gattdb_op_time, sizeof(ad_id), (const uint8_t*)&ad_id);
			}
		}
		taskYIELD();
	}
}

/***********************************************************************************************//**
 *  Обработка сообщений от BLE
 *  evt - указатель на сообщение
 **************************************************************************************************/
#define date_ptr ((const struct date_time*)evt->data.evt_gatt_server_attribute_value.value.data)
#define date_len (evt->data.evt_gatt_server_attribute_value.value.len)
#define date_offset (evt->data.evt_gatt_server_attribute_value.offset)
int appHandleEvents(struct gecko_cmd_packet *evt)
{
  // Пока BLE не готов, сообщени не обрабатываются
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
    vTaskDelay(MS_TO_TICK(50));	//usleep(50000);
    return -1;
  }

  /* Обработка сообщений */
  switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:	// сообщение о готовности BLE

      appBooted = true;

	  /* сигнализируем о присутствии с интервалом 100 мс по всем каналам */
      gecko_cmd_le_gap_set_adv_parameters(160, 160, 7);

      /* Разрешаем соединения */
      gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

      break;

    case gecko_evt_le_connection_closed_id:	// соединение закрыто со стороны пользователя
      // Разрешаем новые соединения
      gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

      break;
	  
	case gecko_evt_gatt_server_attribute_value_id:	// сообщение об изменении параметра со смартфона
		switch (evt->data.evt_gatt_server_attribute_value.attribute) {	// селектор изменяемого атрибута 
			case gattdb_date_time:	// изменение даты/времени
				switch (evt->data.evt_gatt_server_attribute_value.att_opcode) {	// селектор типа действия с атрибутом
					case gatt_write_request:	// запрос на запись
					/* обновляем дату и время в RTC */
						printf("change date: %04d:%02d:%02d:%02d:%02d:%02d\r\n", date_ptr->year, date_ptr->month, date_ptr->day, date_ptr->hours, date_ptr->minutes, date_ptr->seconds);
						ble_update_rtc((const struct date_time*)evt->data.evt_gatt_server_attribute_value.value.data);
						
					break;
				}
			break;
			
			case gattdb_current_doze:	// изменение дозы
				if (evt->data.evt_gatt_server_attribute_value.att_opcode == gatt_write_request) {	// запрос на запись
					uint8_t att_store[6];	// массив для приёма 6-и байтного числа дозы в граммах
					memcpy(att_store,
					evt->data.evt_gatt_server_attribute_value.value.data,
					evt->data.evt_gatt_server_attribute_value.value.len);	// копируем величину дозы из сообщения
					set_doze(*(int32_t*)(att_store));	// преобразуем в double и сохраняем
					
				}
			break;
			
			case gattdb_dispancer:	// команда на начало процесса рассеивания
				if (evt->data.evt_gatt_server_attribute_value.att_opcode == gatt_write_request) {	// запрос на запись
					extern uint8_t motor1_on, purge_on;
					extern TaskHandle_t Motor1TaskHandle;
					if (evt->data.evt_gatt_server_attribute_value.value.data[0] != 0) {	// если аттрибут не равен нулю
						motor1_on = 0xFF; vTaskResume( Motor1TaskHandle ); LED_SM1_ON;	// запускаем процесс
					}
					else {motor1_on = 0; purge_on = 0; LED_SM1_OFF;}	// иначе останавливаем процесс
				}
			break;
			
			case gattdb_calibrate:	// команда на сброс и сохранение начального смещения АЦП тензодатчика
				if (evt->data.evt_gatt_server_attribute_value.att_opcode == gatt_write_request) {	// запрос на запись
					set_ad7799_zero();	// при любом значении аттрибута сбрасываем в ноль и записываем смещение
				}
		}
	break;
		
    default:
      break;
  }
  return 0;
}

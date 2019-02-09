#include "Global.h"

#include "infrastructure.h"

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"
#include "gatt_db.h"

#include "i2c.h"

BGLIB_DEFINE();

// App booted flag
static bool appBooted = false;

int appHandleEvents(struct gecko_cmd_packet *evt);

void BLEProc(void *Param)
{
	struct gecko_cmd_packet* evt;

  /* Initialize BGLIB with our output function for sending messages. */
	BGLIB_INITIALIZE_NONBLOCK(BLEUartTx, BLEUartRx, BLEUartPeek);

	InitBLEUart(BGLIB_MSG_MAX_PAYLOAD+4);
	

  /* Reset NCP to ensure it gets into a defined state.
   * Once the chip successfully boots, gecko_evt_system_boot_id event should be received. */
	gecko_cmd_system_reset(0);
	while (1) {
		/* Check for stack event. */
		evt = gecko_peek_event();
		if (NULL != evt) {
		// Run application and event handler.
			if (appHandleEvents(evt) == -1) continue;
		}
		taskYIELD();
		if (appBooted == false) continue;
		
		if (RTC_Queue != NULL) {
//			static uint8_t flag = 0;
			struct ble_date_time rtc;
			if (pdPASS == xQueueReceive(RTC_Queue, &rtc, 0)) {
				gecko_cmd_gatt_server_write_attribute_value(gattdb_date_time, 0x0000, sizeof(struct ble_date_time), (const uint8_t*)&rtc);
				gecko_cmd_gatt_server_send_characteristic_notification(0xFF, gattdb_date_time, sizeof(struct ble_date_time), (const uint8_t*)&rtc);
			}
		}
		taskYIELD();
		if (AD7799_Queue != NULL) {
			uint8_t ad_id[6] = {0,0,0,0,0,0};
			if (pdPASS == xQueueReceive(AD7799_Queue, &ad_id, 0)) {
				gecko_cmd_gatt_server_write_attribute_value(gattdb_op_time, 0x0000, sizeof(ad_id), (const uint8_t*)&ad_id);
				gecko_cmd_gatt_server_send_characteristic_notification(0xFF, gattdb_op_time, sizeof(ad_id), (const uint8_t*)&ad_id);
			}
		}
		taskYIELD();
//		gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
	}
}

/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
int appHandleEvents(struct gecko_cmd_packet *evt)
{
  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
//#if defined(DEBUG)
//    printf("Event: 0x%04x\n", BGLIB_MSG_ID(evt->header));
//#endif
    vTaskDelay(MS_TO_TICK(50));	//usleep(50000);
    return -1;
  }

  /* Handle events */
  switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:

      appBooted = true;
//      printf("System booted. Starting advertising... \n");

      /* Set advertising parameters. 100ms advertisement interval. All channels used.
       * The first two parameters are minimum and maximum advertising interval, both in
       * units of (milliseconds * 1.6). The third parameter '7' sets advertising on all channels. */
      gecko_cmd_le_gap_set_adv_parameters(160, 160, 7);

      /* Start general advertising and enable connections. */
      gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

//      printf("Device is being advertised.\n");
      break;

    case gecko_evt_le_connection_closed_id:
      // Restart general advertising and re-enable connections after disconnection.
      gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

      break;
	  
	case gecko_evt_gatt_server_attribute_value_id:
		switch (evt->data.evt_gatt_server_attribute_value.attribute) {
			case gattdb_date_time:
				switch (evt->data.evt_gatt_server_attribute_value.att_opcode) {
					case gatt_write_request:
						ble_update_rtc((const struct ble_date_time*)evt->data.evt_gatt_server_attribute_value.value.data);
					break;
				}
			break;
			
			case gattdb_current_doze:
				if (evt->data.evt_gatt_server_attribute_value.att_opcode == gatt_write_request) {
					uint8_t att_store[6];
					memcpy(att_store,
					evt->data.evt_gatt_server_attribute_value.value.data,
					evt->data.evt_gatt_server_attribute_value.value.len);
					set_doze(*(int32_t*)(att_store));
					
				}
			break;
			
			case gattdb_dispancer:
				if (evt->data.evt_gatt_server_attribute_value.att_opcode == gatt_write_request) {
					extern uint8_t motor1_on, purge_on;
					extern TaskHandle_t Motor1TaskHandle;
//					extern double doze, th, flt10;
					if (evt->data.evt_gatt_server_attribute_value.value.data[0] != 0) {
//						if (doze > 0.0) {
//							th = flt10 - doze;
//							if (th < 0.0) th = 0.0;
							motor1_on = 0xFF; vTaskResume( Motor1TaskHandle ); LED_SM1_ON;
//						}
					}
					else {motor1_on = 0; purge_on = 0; LED_SM1_OFF;}
				}
			break;
			
			case gattdb_calibrate:
				if (evt->data.evt_gatt_server_attribute_value.att_opcode == gatt_write_request) {
					set_ad7799_zero();
				}
		}
	break;
		
    default:
      break;
  }
  return 0;
}

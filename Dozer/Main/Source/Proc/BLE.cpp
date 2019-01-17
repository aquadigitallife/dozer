#include "Global.h"

#include "infrastructure.h"

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

/* Own header */
#include "app.h"

PACKSTRUCT(struct st_date_time
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
});

BGLIB_DEFINE();

uint8_t dozer_name[] = "Hello, Dozer!";

void BLEProc(void *Param)
{
	uint32_t flag = 0;
	uint8_t flag2 = 0;
	struct gecko_cmd_packet* evt;

  /* Initialize BGLIB with our output function for sending messages. */
	BGLIB_INITIALIZE_NONBLOCK(BLEUartTx, BLEUartRx, BLEUartPeek);

	InitBLEUart(BGLIB_MSG_MAX_PAYLOAD+4);

  /* Reset NCP to ensure it gets into a defined state.
   * Once the chip successfully boots, gecko_evt_system_boot_id event should be received. */
	gecko_cmd_system_reset(0);

	while (1) {
		static struct st_date_time date_time = {2019, 01, 12, 14, 31, 0};
		/* Check for stack event. */
		evt = gecko_peek_event();
		// Run application and event handler.
		appHandleEvents(evt);

		
		if (flag > Time) {
			flag2 ^= 0xFF;
			if (flag2) LED_WORK_ON; else LED_WORK_OFF;
			date_time.seconds++;
			if (date_time.seconds > 59) date_time.seconds = 0;
			if (gecko_cmd_gatt_server_write_attribute_value(17, 0x0000, sizeof(struct st_date_time), (const uint8_t*)&date_time)->result != 0) LED_ERR_ON;
			else LED_ERR_OFF;
			vTaskDelay(MS_TO_TICK(50));
			gecko_cmd_gatt_server_send_characteristic_notification(0xFF, 17, sizeof(struct st_date_time), (const uint8_t*)&date_time);
		}
		flag = Time;
	}
}


// App booted flag
static bool appBooted = false;

/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
void appHandleEvents(struct gecko_cmd_packet *evt)
{


  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
//#if defined(DEBUG)
//    printf("Event: 0x%04x\n", BGLIB_MSG_ID(evt->header));
//#endif
    vTaskDelay(MS_TO_TICK(50));	//usleep(50000);
    return;
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

      /* Restart general advertising and re-enable connections after disconnection. */
      gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

      break;

    default:
      break;
  }
}

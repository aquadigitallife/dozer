#include "Global.h"

#define BUTTON_QUEUE_LENGTH    5
#define BUTTON_ITEM_SIZE       sizeof( uint8_t )

static TaskHandle_t Motor1TaskHandle;


bool isMotorsSuspend(void)
{
	return ((eSuspended == eTaskGetState( Motor0CycleHandle )) 
		 && (eSuspended == eTaskGetState( Motor1TaskHandle )));
}

void ButtonsProc(void *Param)
{
	uint8_t motor1_on = 0;
	TaskHandle_t Motor0TaskHandle;
	
	QueueHandle_t SM0_Queue = xQueueCreate ( BUTTON_QUEUE_LENGTH, BUTTON_ITEM_SIZE );

	configASSERT ( SM0_Queue );

	xTaskCreate(Motor0Proc, "" , configMINIMAL_STACK_SIZE + 400, SM0_Queue, TASK_PRI_LED, &Motor0TaskHandle);
	xTaskCreate(Motor1Proc, "" , configMINIMAL_STACK_SIZE + 400, &motor1_on, TASK_PRI_LED, &Motor1TaskHandle);
	
	while (1) {
		static uint8_t pre_sm1 = 0;
		vTaskDelay(MS_TO_TICK(10));
scan_button:		
		switch MOTOR_BUTTONS {

			case SM0_TEST_A:
				for (int i = 0;;) {
					vTaskDelay(MS_TO_TICK(10));
					if (IS_SM0_TEST_A) i++; else goto scan_button;
					if ((i > 2) && isMotorsSuspend()) {
						uint8_t mButton = 0x0A;
						xQueueSendToBack(SM0_Queue, (void*)&mButton, 0);
						break;
					}
				}
				break;

			case SM0_TEST_B:
				for (int i = 0;;) {
					vTaskDelay(MS_TO_TICK(10));
					if (IS_SM0_TEST_B) i++; else goto scan_button;
					if ((i > 2) && isMotorsSuspend()) {
						uint8_t mButton = 0x0B;
						xQueueSendToBack(SM0_Queue, (void*)&mButton, 0);
						break;
					}
				}
				break;

			case SM1_TEST:
				for (int i = 0;;) {
					vTaskDelay(MS_TO_TICK(10));
					if (IS_SM1_TEST) i++; else goto scan_button;
					if ((i > 2) && (pre_sm1 == 0)) {
						pre_sm1 = 0xFF;
						if (motor1_on == 0) { motor1_on = 0xFF; vTaskResume( Motor1TaskHandle ); LED_SM1_ON; }
						else { LED_SM1_OFF;	motor1_on = 0; }
						break;
					}
				}
				break;

			default: {
				uint8_t mButton = 0x0;
				if (isMotorsSuspend()) xQueueSendToBack(SM0_Queue, (void*)&mButton, 0);
				pre_sm1 = 0;
			}
		}
/*		
		switch (TEST_BUTTONS) {
			case TEST_RESET:
				break;
			case TEST_TEST:
				for (int i = 0;;) {
					vTaskDelay(MS_TO_TICK(10));
					if (IS_TEST) i++; else goto scan_button;
					if ()
				}
				break;
			default:
		}
*/
	}
}

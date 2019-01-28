#include "Global.h"

void SetSpeedSM0(uint16_t period)
{
	LL_TIM_OC_SetCompareCH1(TIM9, period/2);
	LL_TIM_SetAutoReload(TIM9, period);
}

void InitSM0(uint16_t period)
{
	/* GPIO TIM11_CH1 configuration */
	LL_GPIO_SetPinMode(GPIOE, LL_GPIO_PIN_5, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinPull(GPIOE, LL_GPIO_PIN_5, LL_GPIO_PULL_DOWN);
	LL_GPIO_SetPinSpeed(GPIOE, LL_GPIO_PIN_5, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetAFPin_0_7(GPIOE, LL_GPIO_PIN_5, LL_GPIO_AF_3);

	/*********************************/
	/* Output waveform configuration */
	/*********************************/
	/* Select counter mode: counting up */
	LL_TIM_SetCounterMode(TIM9, LL_TIM_COUNTERMODE_UP);
 
	/* Set the one pulse mode:  generate repetitive pulses*/
	LL_TIM_SetOnePulseMode(TIM9, LL_TIM_ONEPULSEMODE_REPETITIVE);
  
	/* Set the TIM9 prescaler to get counter clock frequency at 10 MHz           */
	/* In this example TIM11 input clock (TIM11CLK) is set to APB2 clock (PCLK?), */
	/* since APB2 pre-scaler is equal to ?.                                       */
	/*    TIM11CLK = ?PCLK?                                                       */
	/*    PCLK? = HCLK                                                            */
	/*    => TIM11CLK = SystemCoreClock (100 MHz)                                 */  
	LL_TIM_SetPrescaler(TIM9, __LL_TIM_CALC_PSC(SystemCoreClock, 10000000));
  
	SetSpeedSM0(period);

	/* Set output channel 1 in PWM2 mode */
	LL_TIM_OC_SetMode(TIM9,  LL_TIM_CHANNEL_CH1,  LL_TIM_OCMODE_PWM2);

	/* Configure output channel 1 */
	LL_TIM_OC_ConfigOutput(TIM9, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH | LL_TIM_OCIDLESTATE_LOW);
  
	/**************************/
	/* TIM9 interrupts set-up */
	/**************************/
	/* Enable the capture/compare interrupt for channel 1 */
	//  LL_TIM_EnableIT_CC1(TIM9);
  
	/**************************/
	/* Start pulse generation */
	/**************************/
	/* Enable channel 1 */
	LL_TIM_CC_EnableChannel(TIM9, LL_TIM_CHANNEL_CH1);
  
	/* Enable TIM11 outputs */
	LL_TIM_EnableAllOutputs(TIM9);
  
	/* Enable auto-reload register preload */
	LL_TIM_EnableARRPreload(TIM9);

	/* Force update generation */
	LL_TIM_GenerateEvent_UPDATE(TIM9);  
	
//	PWR_SM0_EN;
	SM0_WAKEUP;
}

void StartSM0(bool dir)
{
	if (dir) SM0_BACKWARD; else SM0_FORWARD;
	LED_SM0_ON;
	SM0_ENABLE;
	LL_TIM_EnableCounter(TIM9);
}

void StopSM0(void)
{
	LL_TIM_DisableCounter(TIM9);
	SM0_DISABLE;
	LED_SM0_OFF;
}

void Motor0Proc(void *Param)
{
	QueueHandle_t SM0_Queue = (QueueHandle_t)Param;
	uint8_t mButton;
	
	InitSM0(0x400);	// при 0x400 - 22 сек полный ход.
	
	while (1) {
		if (uxQueueMessagesWaiting( SM0_Queue )) {
			mButton = 0;
			xQueueReceive(SM0_Queue, &mButton, 0);
			switch (mButton) {
				case 0x0A: StartSM0(1); break;
				case 0x0B: StartSM0(0); break;
				default: StopSM0();
			}
		}
	}	
//	vTaskDelete(NULL);
}

void Motor0Cycle(void *Param)
{
	InitSM0(0x400);
	
	while (1) {
		StopSM0();
		vTaskSuspend( NULL );
		StartSM0(1);
		vTaskDelay(MS_TO_TICK(2000));
		StopSM0();
		vTaskDelay(MS_TO_TICK(3000));
		StartSM0(0);
		vTaskDelay(MS_TO_TICK(2000));
	}
}

/*-------------------Управление крыльчаткой--------------------*/

void SetSpeedSM1(uint32_t period)
{
	LL_TIM_OC_SetCompareCH1(TIM11, period/2);
	LL_TIM_SetAutoReload(TIM11, period);
}

void InitSM1(uint32_t period)
{
	/* GPIO TIM11_CH1 configuration */
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_9, LL_GPIO_PULL_DOWN);
	LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_9, LL_GPIO_AF_3);

	/*********************************/
	/* Output waveform configuration */
	/*********************************/
	/* Select counter mode: counting up */
	LL_TIM_SetCounterMode(TIM11, LL_TIM_COUNTERMODE_UP);
 
	/* Set the one pulse mode:  generate repetitive pulses*/
	LL_TIM_SetOnePulseMode(TIM11, LL_TIM_ONEPULSEMODE_REPETITIVE);
  
	/* Set the TIM11 prescaler to get counter clock frequency at 10 MHz           */
	/* In this example TIM11 input clock (TIM11CLK) is set to APB2 clock (PCLK?), */
	/* since APB2 pre-scaler is equal to ?.                                       */
	/*    TIM11CLK = ?PCLK?                                                       */
	/*    PCLK? = HCLK                                                            */
	/*    => TIM11CLK = SystemCoreClock (100 MHz)                                 */  
	LL_TIM_SetPrescaler(TIM11, __LL_TIM_CALC_PSC(SystemCoreClock, 10000000));
  
	SetSpeedSM1(period);

	/* Set output channel 1 in PWM2 mode */
	LL_TIM_OC_SetMode(TIM11,  LL_TIM_CHANNEL_CH1,  LL_TIM_OCMODE_PWM2);

	/* Configure output channel 1 */
	LL_TIM_OC_ConfigOutput(TIM11, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH | LL_TIM_OCIDLESTATE_LOW);
  
	/**************************/
	/* TIM11 interrupts set-up */
	/**************************/
	/* Enable the capture/compare interrupt for channel 1 */
	//  LL_TIM_EnableIT_CC1(TIM11);
  
	/**************************/
	/* Start pulse generation */
	/**************************/
	/* Enable channel 1 */
	LL_TIM_CC_EnableChannel(TIM11, LL_TIM_CHANNEL_CH1);
  
	/* Enable TIM11 outputs */
	LL_TIM_EnableAllOutputs(TIM11);
  
	/* Enable auto-reload register preload */
	LL_TIM_EnableARRPreload(TIM11);

	/* Force update generation */
	LL_TIM_GenerateEvent_UPDATE(TIM11);  
}

void StartSM1(bool dir)
{
	if (dir) SM1_BACKWARD; else SM1_FORWARD;
	SM1_ENABLE;
	LL_TIM_EnableCounter(TIM11);
//	LED_SM1_ON;
}

void StopSM1(void)
{
	LL_TIM_DisableCounter(TIM11);
	SM1_DISABLE;
//	LED_SM1_OFF;
}

uint8_t motor1_on = 0;
double speed = 65535.0;

void Motor1Proc(void *Param)
{
	extern double flt10, th;
	xTaskCreate(Motor0Cycle, "" , configMINIMAL_STACK_SIZE + 400, NULL, TASK_PRI_LED, &Motor0CycleHandle);
	
	InitSM1(65535);
	while (1) {
restart:
		speed = 65535.0;
		StopSM1();
		while (motor1_on == 0) vTaskSuspend( NULL );
		if (flt10 <= th) continue;
		StartSM1(0); 
		for (;;) {
			vTaskResume( Motor0CycleHandle );
			for (;;) {
				double d;
				SetSpeedSM1((uint32_t)speed);
				vTaskDelay(MS_TO_TICK(10));
				if (speed > 256.0) d = (speed - 128.0)/64; else d = (speed - 128.0)/128.0;
				if (d < 0.5) break;
				speed = speed - d;
			}

			vTaskDelay(MS_TO_TICK(2000));
			
			for (;;) {
				double d;
				SetSpeedSM1((uint32_t)speed);
				vTaskDelay(MS_TO_TICK(10));
				if (speed >= 512.0) d = speed/64.0; else d = speed/256.0;
				speed = speed + d;
				if (speed >= 10000.0) break;
			}
			StopSM1();
			speed = 65535.0;
			for (int i = 0; i < 10; i++) {
				if (motor1_on == 0) goto restart;
				vTaskDelay(MS_TO_TICK(1000));
			}
			if (flt10 <= th) break;
			SetSpeedSM1((uint32_t)speed);
			StartSM1(0); 
//			if (motor1_on == 0) break;
		}
	}

//	vTaskDelete(NULL);
}




void MotorCycleProc(void *Param)
{
}
#include "Global.h"

void InitSM1(uint32_t delay_us, uint32_t pulse_us)
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
  
	/* Set the capture/compare register to get a pulse delay of 50 us */
	LL_TIM_OC_SetCompareCH1(TIM11, __LL_TIM_CALC_DELAY(SystemCoreClock, LL_TIM_GetPrescaler(TIM11), delay_us));
  
	/* Set the autoreload register to get a pulse length of 50 us */
	LL_TIM_SetAutoReload(TIM11, __LL_TIM_CALC_PULSE(SystemCoreClock, LL_TIM_GetPrescaler(TIM11), delay_us, pulse_us));

	/* Set output channel 1 in PWM2 mode */
	LL_TIM_OC_SetMode(TIM11,  LL_TIM_CHANNEL_CH1,  LL_TIM_OCMODE_PWM2);

	/* Configure output channel 1 */
	LL_TIM_OC_ConfigOutput(TIM11, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH | LL_TIM_OCIDLESTATE_LOW);
  
	/**************************/
	/* TIM3 interrupts set-up */
	/**************************/
	/* Enable the capture/compare interrupt for channel 1 */
	//  LL_TIM_EnableIT_CC1(TIM11);
  
	/**************************/
	/* Start pulse generation */
	/**************************/
	/* Enable channel 1 */
	LL_TIM_CC_EnableChannel(TIM11, LL_TIM_CHANNEL_CH1);
  
	/* Enable TIM3 outputs */
	LL_TIM_EnableAllOutputs(TIM11);
  
	/* Enable auto-reload register preload */
	LL_TIM_EnableARRPreload(TIM11);

	/* Force update generation */
	LL_TIM_GenerateEvent_UPDATE(TIM11);  

	StartSM1(0);
}

void StartSM1(bool dir)
{
	if (dir) SM1_BACKWARD; else SM1_FORWARD;
	SM1_ENABLE;
	LL_TIM_EnableCounter(TIM11);
}

void StopSM1(void)
{
	LL_TIM_DisableCounter(TIM11);
	SM1_DISABLE;
}
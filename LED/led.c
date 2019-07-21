#include "led.h"

void MX_LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct =
	{ 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, LED1_Pin | LED2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : PAPin PAPin */
	GPIO_InitStruct.Pin = LED1_Pin | LED2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void led1_flash(uint16_t time_gap)
{
	static uint32_t tick = 0;
	if(HAL_GetTick() - tick >= time_gap)
	{
		tick = HAL_GetTick();
		HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
	}
}

void led2_flash(uint16_t time_gap)
{
	static uint32_t tick = 0;
	if(HAL_GetTick() - tick >= time_gap)
	{
		tick = HAL_GetTick();
		HAL_GPIO_TogglePin(LED2_GPIO_Port,LED2_Pin);
	}
}
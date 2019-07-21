#include "button.h"

key_press_t key =
{ .status = NONE, .key_event = NO_EVENT};

/** Configure pins as 
 * Analog
 * Input
 * Output
 * EVENT_OUT
 * EXTI
 */
void MX_GPIO_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStruct =
	{ 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin : PtPin */
	GPIO_InitStruct.Pin = KEY_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(KEY_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == KEY_Pin)
	{
		// 首次按下，记录这次按下的时间
		if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
		{
			key.press_point = HAL_GetTick();
			key.status = PRESS_DOWN;
		}
		// 按下后松开，判断此次按下的时间，产生不同的按键事件
		else
		{
			if (key.status == PRESS_DOWN)
			{
				if (HAL_GetTick() - key.press_point >= SHORT_PRESS_TIME)	// 大于短按时间
				{
					if(HAL_GetTick() - key.press_point >= LONG_PRESS_TIME)
					{
						key.key_event = LONG_PRESS;
					}
					else
					{
						key.key_event = SHORT_PRESS;
					}
					key.status = FINISH;
				}
			}
		}
	}
}

key_event_e get_key_event(void)
{
	if(key.status == FINISH)
	{
		key.status = NO_EVENT;
		return key.key_event;
	}
	else
	{
		return NO_EVENT;
	}
}

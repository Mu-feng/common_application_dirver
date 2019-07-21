#ifndef __led_H
#define __led_H

#ifdef __cplusplus
 extern "C" {
#endif

#define LED1(x)		HAL_GPIO_WritePin(GPIOA,LED1_Pin,(x)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define LED2(x)		HAL_GPIO_WritePin(GPIOA,LED2_Pin,(x)?GPIO_PIN_SET:GPIO_PIN_RESET)


void led1_flash(uint16_t time_gap);
void led2_flash(uint16_t time_gap);

#ifdef __cplusplus
}
#endif
#endif

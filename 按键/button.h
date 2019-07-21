#ifndef __button_H
#define __button_H

#ifdef __cplusplus
 extern "C" {
#endif

#define SHORT_PRESS_TIME	50
#define LONG_PRESS_TIME		500

typedef enum
{
	NO_EVENT = 0,
	SHORT_PRESS,
	LONG_PRESS,
}key_event_e;

typedef enum
{
	NONE = 0,
	PRESS_DOWN,
	FINISH,
}key_status_e;

typedef struct
{
	uint32_t press_point;
	key_status_e status;
	key_event_e key_event;
}key_press_t;

void MX_GPIO_Init(void);

key_event_e get_key_event(void);

#ifdef __cplusplus
}
#endif
#endif

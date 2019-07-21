/**
  ******************************************************************************
  * File Name          : USART.h
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __wifi_usart_H
#define __wifi_usart_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#define WIFI_RECV_SIZE  512
#define SOCKET_OFFSET   5
#define BUFF_OFFSET     9

#define HEAD1   0xA5
#define HEAD2   0x5A

typedef enum
{
    // Android to RTK
    APPKEY = 1U,        
    APPSECRET,
    DEVICE_ID,
    DEVICE_TYPE,
    LOCATION_MODE_SWITCH,    // 模式切换
    
    // RTK to Android
    CALIBRATION_STATUS = 0x11,
    POSITION_MSG,
    SATELLITE_NUM,
    BATTERY_MSG,
    
    // Developer Option
    NOVATEL_CMD_MODE = 20,
    NOVATEL_CMD,
    NOVATEL_RETURN,
    ALTITUDE_TYPE,
}wifi_data_type_e;

#pragma pack(1)
typedef struct
{
    uint8_t socket;
    uint8_t head[2];
    wifi_data_type_e type;
    uint8_t length;
    uint8_t buff[WIFI_RECV_SIZE];
    uint8_t crc;
    bool finish;
}wifi_recv_format_packet_t;

typedef struct
{
    uint8_t buff[WIFI_RECV_SIZE];
    volatile uint16_t index_start;
    volatile uint16_t index_stop;
    volatile bool status;
}wifi_recv_buff_t;

extern UART_HandleTypeDef wifi_uart;

void MX_UART5_Init(void);

bool wifi_init(void);
bool wifi_send_data(uint8_t socket, uint8_t* buff,uint16_t length);
bool wifi_connect(uint8_t *cmd, uint8_t *passwd);
bool wifi_recv_data(uint8_t* socket,uint8_t* buff);
wifi_recv_format_packet_t* get_recv_format_packet(void);
int16_t find_str(uint8_t *str_cheked, uint8_t *str_mark, uint16_t length);
bool is_available(void);
void handle_wifi_cmd(void);
void wifi_send_format_packet(wifi_recv_format_packet_t* wifi_recv_format_packet_p);
void send_msg_to_android(void);
bool get_cmd_mode(void);
void send_novatel_return(uint8_t buff[],uint8_t length);

#ifdef __cplusplus
}
#endif

#endif /*__ usart_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>
#include "radio.h"
#include "qx_service.h"
#include "flash.h"
#include "ip5328p.h"
#include "gnss.h"
#include "wifi_usart.h"

static uint16_t get_raw_recv(uint8_t *raw_data);
static bool respond_check(uint8_t *str);
static void wifi_uart_send(uint8_t *buff, uint16_t length);
static uint16_t get_recv_length(uint8_t *recv_buff, uint8_t* buff, uint16_t recv_buff_len);

const char* SSID = "MMC_RTK";
const char* PASSWD = "mmc12345678";
UART_HandleTypeDef wifi_uart;
wifi_recv_buff_t wifi_recv =
{
    .index_start = 0, /* 接收缓存起点 */
    .index_stop = 0,
    .status = 0,
};

wifi_recv_format_packet_t format_packet =
{
    .head = {HEAD1, HEAD2},
    .finish = false,
};

bool wifi_init_finish = false;

/**
  * @brief  UART5 init function
  * @param  None
  * @retval None
  */
void MX_UART5_Init(void)
{

    wifi_uart.Instance = UART5;
    wifi_uart.Init.BaudRate = 115200;
    wifi_uart.Init.WordLength = UART_WORDLENGTH_8B;
    wifi_uart.Init.StopBits = UART_STOPBITS_1;
    wifi_uart.Init.Parity = UART_PARITY_NONE;
    wifi_uart.Init.Mode = UART_MODE_TX_RX;
    wifi_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    wifi_uart.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&wifi_uart) != HAL_OK)
    {
        Error_Handler();
    }
    __HAL_UART_ENABLE_IT(&wifi_uart, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&wifi_uart, UART_IT_IDLE);
}

void UART5_IRQHandler(void)
{
    if (__HAL_UART_GET_IT_SOURCE(&wifi_uart, UART_IT_RXNE) && __HAL_UART_GET_FLAG(&wifi_uart, UART_FLAG_RXNE))
    {
        wifi_recv.buff[wifi_recv.index_stop] = wifi_uart.Instance->DR;
        wifi_recv.index_stop++;
        if (wifi_recv.index_stop >= WIFI_RECV_SIZE)
        {
            wifi_recv.index_stop = 0;
        }
    }
    if (__HAL_UART_GET_IT_SOURCE(&wifi_uart, UART_IT_IDLE) && __HAL_UART_GET_FLAG(&wifi_uart, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&wifi_uart);
        wifi_recv.status = 1;
        if(wifi_init_finish == true)
        {
            if(wifi_recv_data(&format_packet.socket, format_packet.buff) == true)
            {
                format_packet.finish = true;
            }
            else
            {
                format_packet.finish = false;
            }
        }
    }
}

/**
  * @brief wifi: 接收数据，返回socket号，返回接收的数据
  * @param socket: 接收数据的socket
  * @param buff: 接收的数据
  * @retval true: 表示接收到数据
  */
bool wifi_recv_data(uint8_t* socket, uint8_t* buff)
{
    uint8_t temp[WIFI_RECV_SIZE];  /* 恐怖的地方，让我出BUG */
    uint16_t length;
    uint16_t buff_len;
    if(wifi_recv.status == 1)
    {
        length = get_raw_recv(temp);

        /** 获取数据帧起点——“+IPD”
          * 数据格式"<?>+IPD,<socket>,<channel>,<buff>"  总长度为length
          * <?>为 未知长度的数据
          * socket为一个字节,相对偏移量为 5,
          */
        int16_t ipd_offset = find_str(temp, (uint8_t*)"+IPD,", length);
        if(ipd_offset >= 0)
        {
            /* 获取socket序号 */
            *socket = temp[ipd_offset + SOCKET_OFFSET]  - '0';
            buff_len = length - ipd_offset - SOCKET_OFFSET;
            get_recv_length(&temp[ipd_offset + SOCKET_OFFSET], buff, buff_len);
            return true;
        }
    }
    return false;
}

bool save_qxwz_config(uint8_t type,uint8_t msg[],uint8_t len)
{
    qx_config_t config;

    /* 读数据 */
    HAL_Read_Flash(QX_ACCOUNT_INFO_ADDR, (uint8_t *)&config, QX_ACCOUNT_INFO_LEN);

    switch(type)
    {
    case APPKEY:
        if(len < sizeof(config.appkey))
        {
            msg[len] = '\0';
        }
        memcpy(config.appkey, msg, len + 1);
        break;

    case APPSECRET:
        if(len < sizeof(config.appsecret))
        {
            msg[len] = '\0';
        }
        memcpy(config.appsecret, msg, len + 1);
        break;

    case DEVICE_ID:
        if(len < sizeof(config.device_id))
        {
            msg[len] = '\0';
        }
        memcpy(config.device_id, msg, len + 1);
        break;

    case DEVICE_TYPE:
        if(len < sizeof(config.device_type))
        {
            msg[len] = '\0';
        }
        memcpy(config.device_type, msg, len + 1);
        break;

    default:
        return false;

    }

    /* 擦除数据 */
    HAL_Erase_Flash(INFO_ADDR);

    /* 写入数据 */
    write_flash(QX_ACCOUNT_INFO_ADDR, (const uint8_t *)&config, sizeof(qx_config_t));

    return true;
}

static qx_config_t config;

void sync_qx_config(void)
{
    /* 读取数据 */
    HAL_Read_Flash(QX_ACCOUNT_INFO_ADDR, (uint8_t *)&config, QX_ACCOUNT_INFO_LEN);

    /* 获取千寻配置变量指针，用于修改千寻配置 */
    qxwz_usr_config_t* qxwz_usr_config = get_qxwz_usr_config();

    /* 以下为配置运行时千寻的参数 */
    qxwz_usr_config->appkey = (char*)config.appkey;
    qxwz_usr_config->appsecret = (char*)config.appsecret;
    qxwz_usr_config->device_ID = (char*)config.device_id;
    qxwz_usr_config->device_Type = (char*)config.device_type;
}

/**
  * @brief 从接收缓存中获取此次接收到的原生数据
  * @param temp: 原生数据
  * @retval true,表示有'OK’，false,表示没有
  */
static uint16_t get_raw_recv(uint8_t *raw_data)
{
    uint16_t length;
    memcpy(raw_data, &(wifi_recv.buff[wifi_recv.index_start]),
           wifi_recv.index_stop - wifi_recv.index_start);
    length = wifi_recv.index_stop - wifi_recv.index_start;
    wifi_recv.status = 0;
    wifi_recv.index_stop = 0;
    return length;
}

/**
  * @brief  查找字符串中是否有 某个子字符串，如果有返回其索引，如果没有，返回-1
  * @param  str_checked: 待查找的字符串
  * @param  str_mark: 查找的子字符串
  * @retval -1 表示没有找到，其他数值，子字符串在待检测字符串中的位置
  */
int16_t find_str(uint8_t *str_cheked, uint8_t *str_mark, uint16_t length)
{
    uint16_t n2 = strlen((char*)str_mark);
    int16_t index = 0;
    for(; index < length; index++)
    {
        if (memcmp(&(str_cheked[index]), str_mark, n2) == 0)
        {
            break;
        }
    }
    if(index < length)
    {
        return index;
    }
    else
    {
        return -1;
    }
}

/**
  * @brief  从接收缓存中获取数据长度，数据内容
  * @param  recv_buff: 数据缓存
  * @param  buff: 保存接收到的数据内容
  * @param  recv_buff_len: 此次处理的数据缓存长度
  * @retval 数据缓存中的数据长度
  */
static uint16_t get_recv_length(uint8_t *recv_buff, uint8_t* buff, uint16_t recv_buff_len)
{
    uint16_t length = 0;
    uint8_t index;
    index = find_str(recv_buff, (uint8_t*)",", recv_buff_len); /* index = 1 */
    index++;                                                   /* index = 2 */
    uint8_t i;
    for(i = 0; recv_buff[index + i] != ':'; i++)
    {
        length = length * 10 + recv_buff[index + i] - '0';
    }
    index += i;
    memcpy(buff, &recv_buff[index + 1],recv_buff_len - index);
    buff[recv_buff_len - index] = '\0';
    return length;
}

/**
  * @brief  发送AT指令到esp8266
  * @param  -cmd: 命令内容
  * @param  -length: 命令长度
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_send_cmd(uint8_t cmd[], uint16_t length)
{
    bool status = false;
    for(uint8_t n = 0; n < 3; n++)
    {
        wifi_uart_send(cmd, length);
        uint8_t i;
        for(i = 0 ; i < 200; i++) /* 循环200次，每次等待25ms，一共5s */
        {
            osDelay(25);
            if (wifi_recv.status == 1)
            {
                /* wifi_recv.status = 0; */
                if (respond_check((uint8_t*)"OK") == true)
                {
                    status = true;
                    break;
                }
            }
        }
        if(i < 200)
        {
            break;
        }
    }
    return status;
}

/**
  * @brief  wifi串口发送字符串函数
  * @param  buff: 待发送的字符串（数组）
  * @param  length: 待发送的字符串（数组）长度
  * @retval true,发送成功
  */
static void wifi_uart_send(uint8_t *buff, uint16_t length)
{
    for(uint16_t i = 0; i < length; i++)
    {
        wifi_uart.Instance->DR = buff[i];
        while(__HAL_UART_GET_FLAG(&wifi_uart, UART_FLAG_TXE) == RESET);
    }
}

/**
  * @brief  重启模块
  * @param  None
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_reset(void)
{
    uint8_t cmd[] = "AT+RST\r\n";
    return wifi_send_cmd(cmd, strlen((char*)cmd));
}

/**
  * @brief  重置模块参数
  * @param  None
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_restore(void)
{
    uint8_t cmd[] = "AT+RESTORE\r\n";
    return wifi_send_cmd(cmd, strlen((char*)cmd));
}

/**
  * @brief  关闭回显
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_set_ate(uint8_t mode)
{
    uint8_t cmd[] = "ATE0\r\n";
    cmd[3] = mode + '0';
    return wifi_send_cmd(cmd, strlen((char*)cmd));
}

/**
  * @brief  设置工作模式
  * @param  mode: 1:station模式,
  *               2:softAP模式
  *               3:softAP+station模式
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_set_mode(uint8_t mode)
{
    uint8_t cmd[] = "AT+CWMODE_DEF=0\r\n";
    cmd[14] = mode + '0';
    return wifi_send_cmd(cmd, strlen((char*)cmd));
}

/**
  * @brief  配置wifi ap的ip地址以及网关
  * @param  None
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_set_ap_ip(void)
{
    uint8_t cmd[] = "AT+CIPSTA_DEF=\"192.168.4.1\",\"192.168.4.1\",\"255.255.255.255\"\r\n";
    return wifi_send_cmd(cmd, strlen((char*)cmd));
}

/**
  * @brief  配置wifi ap的参数
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_config_ap(uint8_t* ssid, uint8_t* passwd)
{
    uint8_t cmd[64];
    sprintf((char*)cmd, "AT+CWSAP_DEF=\"%s\",\"%s\",1,4\r\n", ssid, passwd);
    return wifi_send_cmd(cmd, strlen((char*)cmd));
}

/**
  * @brief  设置多连接模式
  * @param  mode: 1:透传模式,
  *               0:退出透传模式
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_set_pt(uint8_t mode)
{
    uint8_t cmd[] = "AT+CIPMODE=0\r\n";
    cmd[11] = mode + '0';
    return wifi_send_cmd(cmd, strlen((char*)cmd));
}

/**
  * @brief  设置多连接模式
  * @param  mode: 1:设置成多连接
  *               0:设置成单连接
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_set_mux(uint8_t mode)
{
    uint8_t cmd[] = "AT+CIPMUX=1\r\n";
    cmd[10] = mode + '0';
    return wifi_send_cmd(cmd, strlen((char*)cmd)); /* 设置多连接 */
}

/**
  * @brief  创建TCP服务
  * @retval true,表示正常响应，false,表示没有正常响应
  */
static bool wifi_set_tcp_server(void)
{
    uint8_t cmd[] = "AT+CIPSERVER=1,8090\r\n";
    return wifi_send_cmd(cmd, strlen((char*)cmd));
}

/**
  * @brief  连接到指定wifi
  * @retval true,表示正常响应，false,表示没有正常响应
  */
bool wifi_connect(uint8_t *cmd, uint8_t *passwd)
{
    uint8_t temp[64];
    sprintf((char*) temp, "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", (const char*) cmd,
            (const char*) passwd);
    return wifi_send_cmd(temp, (uint8_t)strlen((char*)temp));
}

/**
  * @brief  esp8266初始化配置
  * @retval true,表示正常响应，false,表示没有正常响应
  */
bool wifi_init(void)
{
    if(wifi_reset() != true)
    {
        return false;
    }
    osDelay(200);
    if(wifi_set_ate(0) != true)
    {
        return false;
    }
    osDelay(200);
    if(wifi_set_mode(3) != true) /* 设置ap+stastion模式 */
    {
        return false;
    }
    osDelay(200);
    if(wifi_config_ap((uint8_t*) SSID, (uint8_t*) PASSWD) != true) /* 配置ap信息 */
    {
        return false;
    }
    osDelay(200);
    if(wifi_set_ap_ip() != true) /* 设置AP的ip以及网关 */
    {
        return false;
    }
    osDelay(200);

    /* 透传模式在重启后默认关闭，所以设置多连接之前无需设置透传 */
    if(wifi_set_mux(1) != true) /* 设置多连接 */
    {
        return false;
    }

    osDelay(200);
    if(wifi_set_tcp_server() != true) /* 创建TCP服务器 */
    {
        return false;
    }
    wifi_init_finish = true;
    return true;
}

/**
  * @brief  通过wifi模块发送数据到对应socket
  * @param  socket:发送目标socket
  * @param  buff:待发送的数据
  * @param  length:发送数据的长度
  * @retval true,表示有'OK’，false,表示没有
  */
bool wifi_send_data(uint8_t socket, uint8_t* buff,uint16_t length)
{
    bool status = false;
    uint8_t cmd[64];
    sprintf((char*)cmd, "AT+CIPSEND=%d,%d\r\n", socket, length);
    for(uint8_t i = 0; i < 3; i++)
    {
        /* 准备发送数据 */
        wifi_init_finish = false;
        wifi_uart_send(cmd, strlen((char*)cmd));
        uint8_t j = 0;
        for(; j < 200; j++)
        {
            osDelay(25);
            if (wifi_recv.status == 1)
            {
                /* 检测是否接收到 '>' 以发送数据 */
                if (respond_check((uint8_t*)">") == true)
                {
                    status = true;
                    wifi_uart_send(buff, length);
                    break;
                }
            }
        }
        wifi_init_finish = true;
        if(j < 200)
        {
            break;
        }
    }
    return status;
}

/**
  * @brief  检查返回的数据中是否有 ‘OK’
  * @retval true,表示有'OK’，false,表示没有
  */
static bool respond_check(uint8_t *str)
{
    uint8_t temp[WIFI_RECV_SIZE];
    uint16_t length;

    length = get_raw_recv(temp);

    int16_t offset = find_str(temp, str, length);

    if ((offset >= 0)) /* 有OK返回 */
    {
        return true;
    }
    return false;
}

/**
  * @brief  获取wifi接收数据的外部函数接口
  * @retval 数据结构体指针
  */
wifi_recv_format_packet_t* get_recv_format_packet(void)
{
    return &format_packet;
}

/**
  * @brief  用来判断接收到的数据是否符合固定的格式，符合表示可用
  * @retval true:数据可用
  */
bool is_available(void)
{
    bool status = false;
    if((format_packet.buff[0] == HEAD1) && (format_packet.buff[1] == HEAD2))
    {
        format_packet.type = format_packet.buff[2];
        format_packet.length = format_packet.buff[3];
        if(format_packet.length < 0xFF)
        {
            /* wifi接收校验 */
            if(format_packet.buff[format_packet.length + 4] == crc_check(0,&format_packet.buff[2],format_packet.length + 2))
            {
                status = true;

                /* 处理前，head1、head2、type和length都在buff里面,之后，format_packet.buff只剩下数据 */
                memcpy(&format_packet.buff[0],&format_packet.buff[4],format_packet.length);
                format_packet.buff[format_packet.length] = 0; /* 添加结束符 */
            }
        }
    }
    return status;
}

/**
  * @brief  对不同类型的数据进行处理
  * @retval none
  */
void handle_wifi_cmd(void)
{
    float * buff;

    switch(format_packet.type)
    {
    case APPKEY:
        /* 写入APPKEY */
        save_qxwz_config(format_packet.type,format_packet.buff,strlen((char*)format_packet.buff));
        break;

    case APPSECRET:
        /* 写入APPSECRET */
        save_qxwz_config(format_packet.type,format_packet.buff,strlen((char*)format_packet.buff));
        break;

    case DEVICE_ID:
        /* 写入Device_ID */
        save_qxwz_config(format_packet.type,format_packet.buff,strlen((char*)format_packet.buff));
        break;

    case DEVICE_TYPE:
        /* 写入Device_Type */
        save_qxwz_config(format_packet.type,format_packet.buff,strlen((char*)format_packet.buff));
        break;

    case LOCATION_MODE_SWITCH:
        /**
          * 更改RTK校准模式，千寻模式或Novatel模式
          * 更改的时候选择把flash更新到qx配置变量
          */
        if(format_packet.buff[0] == 0x02) /* 切换为千寻模式 */
        {
            sync_qx_config();
            switch_base_mode(QX_CALIBRATION);
        }
        switch_base_mode(NOVATEL_CALIBRATION);
        break;

    case NOVATEL_CMD_MODE:
        if(format_packet.buff[0] == 1)
        {
            /* enter cmd mode */
            enter_cmd_mode(true);
        }
        else
        {
            /* exit cmd mode */
            enter_cmd_mode(false);
        }
        break;

    case NOVATEL_CMD:
        /**
          * 接收Novatel指令
          * 调用Novatel串口，发送,命令内容在 (format_packet.buff),以‘\0’结束
          * TODO
          */

        break;

    case ALTITUDE_TYPE:
        /* 设置高度类型 */
        buff = (float *)format_packet.buff;
        set_fix_position(*buff, *(buff+1), *(buff+2));
        break;

    default:
        break;

    }
}

/**
  * @brief  打包wifi通信数据，并发送
  * @param  format_packet_p：要发送的数据包地址
  * @retval none
  */
void wifi_send_format_packet(wifi_recv_format_packet_t* format_packet_p)
{
    uint8_t buff[32];
    buff[0] = HEAD1;
    buff[1] = HEAD2;
    buff[2] = format_packet_p->type;
    buff[3] = format_packet_p->length;
    memcpy(&buff[4], format_packet_p->buff, format_packet_p->length);
    buff[4 + format_packet_p->length] = crc_check(0, &buff[2], buff[3] + 2);
    wifi_send_data(format_packet_p->socket, buff, format_packet_p->length + 5);
}

void send_msg_to_android(void)
{
    static uint8_t i = 0;
    wifi_recv_format_packet_t *recv_format_packet = get_recv_format_packet();

    i++;

    switch(i)
    {
    case 1:
        /* TODO 发送校准状态 */
        recv_format_packet->type = CALIBRATION_STATUS;
        recv_format_packet->length = 1;
        recv_format_packet->buff[0] = get_fix_status();
        wifi_send_format_packet(recv_format_packet);
        break;

    case 2:
        /* TODO 发送位置信息 */
        recv_format_packet->type = POSITION_MSG;
        recv_format_packet->length = 12;
        memcpy(&recv_format_packet->buff[0], get_position(), recv_format_packet->length);
        wifi_send_format_packet(recv_format_packet);
        break;

    case 3:
        /* TODO 发送卫星搜星数量 */
        recv_format_packet->type = SATELLITE_NUM;
        recv_format_packet->length = 1;
        recv_format_packet->buff[0] = get_num_of_satellites();
        wifi_send_format_packet(recv_format_packet);
        break;

    case 4:
        /* TODO 发送电量信息 */
        recv_format_packet->type = BATTERY_MSG;
        recv_format_packet->length = 4;
        uint16_t battery_msg[2];
        battery_msg[0] = get_battery_level();
        battery_msg[1] = 0;
        memcpy(&recv_format_packet->buff[0], (uint8_t*)&battery_msg, recv_format_packet->length);
        wifi_send_format_packet(recv_format_packet);
        break;

    default:
        i = 0;
        break;

    }
}

/**
  * @brief  发送数据到Android
  * @param  buff: 数据缓存
  * @param  length: 数据长度
  */
void send_novatel_return(uint8_t buff[],uint8_t length)
{

}

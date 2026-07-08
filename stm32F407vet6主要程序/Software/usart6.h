#ifndef __USART6_H
#define __USART6_H

#include "My.h"

#define USART6_MAX_LEN   400

#define SOIL_VOICE      0x02
#define AIR_VOICE       0x03
#define HUMI_VOICE      0x05
#define OBS_VOICE       0x06

/* 语音模块接收数据 */
extern uint8_t USART6DMA_RX_BUF[USART6_MAX_LEN];
extern uint8_t USART6_rx_len;

/* 语音模块接口 */
void USART6DMA_Init(uint32_t __baud);
void USART6DMA_SendData(uint8_t *data, uint16_t size);
void ASR_VOICE(uint8_t voice_choose);
void Check_vioce(void);
void USART6_DataProc(uint8_t *USART6_data, uint16_t Size);

#endif

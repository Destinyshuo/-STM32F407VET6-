#ifndef __USART4_H
#define __USART4_H

#include "My.h"

#define UART4_MAX_LEN           400

#define UART4_FRAME_HEAD1       0xa3
#define UART4_FRAME_HEAD2_LOC   0xb1
#define UART4_FRAME_HEAD2_STA   0xb2
#define UART4_FRAME_HEAD2_TYP   0xb3
#define UART4_FRAME_HEAD2_LED   0xb4
#define UART4_FRAME_TAIL1       0xc3

#define UART4_MODE_SHADE        0x01
#define UART4_MODE_HALF_SHADE   0x02
#define UART4_MODE_SUN          0x03

/* 视觉模块接收数据 */
extern uint8_t UART4DMA_RX_BUF[UART4_MAX_LEN];
extern uint8_t UART4_rx_len;
extern uint16_t Source_X, Source_Y;
extern uint8_t blade_state;
extern uint8_t Soil_PlantType;
extern uint8_t MaixCam_Mode[2][12];

/* 视觉模块接口 */
void UART4DMA_Init(uint32_t __baud);
void UART4DMA_SendData(uint8_t *data, uint16_t size);
void Maixcam_SetMode(uint8_t mode);
void Maixcam_DataProc(uint8_t *UART4_data, uint16_t Size);

#endif

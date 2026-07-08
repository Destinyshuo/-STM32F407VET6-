#ifndef __USART5_H__
#define __USART5_H__

#include "My.h"

#define UART5_MAX_LEN 400

#define PlantSoftpack_head_1    0xAA
#define PlantSoftpack_tail      0x55

#define PlantSoft_CMD_MaixMode   0x10
#define PlantSoft_CMD_WaterCtrl  0x20
#define PlantSoft_CMD_LightCtrl  0x30
#define PlantSoft_CMD_SystemCtr  0x40
#define PlantSoft_CMD_Time       0x60
#define PlantSoft_CMD_PlantType  0x70

#ifndef UART4_MODE_SHADE
#define UART4_MODE_SHADE        0x01
#endif
#ifndef UART4_MODE_HALF_SHADE
#define UART4_MODE_HALF_SHADE   0x02
#endif
#ifndef UART4_MODE_SUN
#define UART4_MODE_SUN          0x03
#endif

/* 上位机接收数据 */
extern uint8_t UART5DMA_RX_BUF[UART5_MAX_LEN];
extern uint8_t UART5_rx_len;
extern float My_temp, My_humi;
extern float OLED_MQ2ppm;
extern float OLED_MQ135ppm;
extern float OLED_SoilPercent;

extern uint8_t Soilpack[6];
extern uint8_t Temppack[6];
extern uint8_t Humipack[6];
extern uint8_t Airpack[6];
extern uint8_t Obspack[7];
extern uint8_t Plantstatepack[4];
extern uint8_t PlantTypepack[4];

extern uint8_t Motor_flag;
extern uint8_t MaixMode_flag;
extern uint8_t Water_flag;

/* 上位机通信接口 */
void UART5DMA_Init(uint32_t __baud);
void UART5DMA_SendData(uint8_t *data, uint16_t size);
void SendData_to_PlantSoft(void);
void PlantSoft_DataProc(uint8_t *PlantSoft_data, uint16_t Size);

#endif

#ifndef __USART2DMA_IMU_H__
#define __USART2DMA_IMU_H__

#include "My.h"

#define USART2_MAX_LEN   400

/* 姿态角和时间 */
extern float Yaw, Roll, Pitch;
extern uint8_t Year, Month, Day, Hour, Minute, Second;
extern uint8_t TimeCalibration_pack1[5];
extern uint8_t TimeCalibration_pack2[5];
extern uint8_t TimeCalibration_pack3[5];
extern uint8_t TimeCalibration_pack4[5];
extern uint8_t TimeCalibration_pack5[5];
extern uint8_t TimeCalibration_pack6[5];

/* 姿态模块接口 */
void Usart2DMA_Init(uint32_t __baud);
void Usart2DMA_SendData(uint8_t *data, uint16_t size);
void Time_Calibration(void);
void HWT101_DataProc(uint8_t *RxBuffer, uint16_t Size);

#endif

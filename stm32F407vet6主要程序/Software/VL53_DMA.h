#ifndef __USART1DMA_VL53_H__
#define __USART1DMA_VL53_H__

#include "My.h"

#define USART1_MAX_LEN   400

#define MY_DAY        2787
#define MY_TIME       2788
#define MY_YAW        2701
#define MY_ROLL       2702
#define MY_PITCH      2703
#define MY_MOTOR1     2704
#define MY_MOTOR2     2705
#define MY_ANGLE      2706
#define MY_MOTOR      2707
#define MY_Distance   2708

#define VL53_DANGER_DISTANCE_MM  300

#define VL53pack_head_1 0x65
#define VL53pack_head_2 0x3a
#define VL53pack_tail   0x6D
#define VL53useful_bites 4
#define VL53pack_num    (VL53useful_bites + 3)

/* 测距结果 */
extern uint16_t Distance;

/* 测距串口接口 */
void USART1DMA_Init(uint32_t __baud);
void Usart1DMA_SendData(uint8_t *data, uint16_t size);
void VL53_DataProc(uint8_t *VL53_data, uint16_t Size);
void My_GetActual(uint16_t acutal_name);

#endif

#ifndef __MOTOR_ENCODER_H__
#define __MOTOR_ENCODER_H__

#include "My.h"

/* 编码器速度和里程 */
extern int Encoder_Speed1, Encoder_Speed2;
extern float Milleage1, Milleage2, Milleage;
extern uint8_t Milleage1_flag, Milleage2_flag;

/* 编码器采样接口 */
void Encoder1_Init(void);
void Encoder2_Init(void);
void Encoder_Init(void);
void GetSpeed_Milleage_1(int *EncoderSpeed, float *EncoderMilleage, uint8_t *Mode);
void GetSpeed_Milleage_2(int *EncoderSpeed, float *EncoderMilleage, uint8_t *Mode);

#endif

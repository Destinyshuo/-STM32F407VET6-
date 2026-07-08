#ifndef __USART3_PORT_H__
#define __USART3_PORT_H__

#include <rtthread.h>
#include <stdint.h>

/* 步进电机串口发送接口 */
void usart_SendCmd(volatile uint8_t *cmd, uint8_t len);
void BuJinMotor_X_Speed(uint8_t Addr, int8_t Speed);
void BuJinMotor_Y_Speed(uint8_t Addr, int8_t Speed);
void BuJinMotor_Circle(uint8_t Addr, uint8_t Dir, uint16_t Speed, uint8_t Circle);
void BuJinMotor_PitchView(uint8_t Addr, uint8_t Dir, uint16_t Speed, float Pitch);
float calculate_slope(float x1, float y1, float x2, float y2, float *k, float *b);

#endif

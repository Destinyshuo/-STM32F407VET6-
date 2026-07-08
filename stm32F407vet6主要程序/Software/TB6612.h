#ifndef __TB6612_H__
#define __TB6612_H__

#include "My.h"

#define AIN_SET()   do { HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET); } while (0)
#define AIN_RESET() do { HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_RESET); } while (0)
#define BIN_SET()   do { HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET); } while (0)
#define BIN_RESET() do { HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_RESET); } while (0)

/* 直流电机控制接口 */
void PWM_Init(void);
void Motor_SetCCR(int32_t MotorCCR1, int32_t MotorCCR2);
void Motor_PidTarget(float Speed1, float Speed2);
void Motor_PidTarget_Acce(float Speed1, float Speed2);
void Motor_PidTarget_Dece(float Speed1, float Speed2);
void Motor_Stop(void);

#endif

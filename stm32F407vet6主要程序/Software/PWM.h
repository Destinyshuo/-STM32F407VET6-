#ifndef __PWM_H
#define __PWM_H

#include "My.h"

#define LED_GPIO_PROT    GPIOE
#define LED_GPIO_PIN     GPIO_PIN_13

extern uint8_t lightONOFF_flag;

/* 补光灯 PWM 接口 */
void LED_PWM_Init(void);
void PWM_SetCompare3(uint16_t Compare);
void LED_Toggle(void);
void LED_PWM(uint16_t target_pwm);
void LightContral(uint8_t contral_flag);

#endif

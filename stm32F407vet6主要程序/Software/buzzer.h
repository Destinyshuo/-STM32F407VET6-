#ifndef __BUZZER_H
#define __BUZZER_H

#include "My.h"

#define BEE_GPIO_PROT    GPIOB
#define BEE_GPIO_PIN     GPIO_PIN_1

/* 蜂鸣器控制接口 */
void Buzzer_Init(void);
void Buzzer_open(void);
void Buzzer_close(void);
void Buzzer_warm(uint8_t times);

#endif

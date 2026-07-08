#ifndef __WATER_H
#define __WATER_H

#include "My.h"

#define WATER_GPIO_PORT    GPIOD
#define WATER_GPIO_PIN     GPIO_PIN_4

/* 水泵控制接口 */
void water_Init(void);
void water_Open(void);
void water_close(void);
void water_auto(uint8_t mode, uint8_t agree);

#endif

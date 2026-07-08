#ifndef __MQ135_H
#define __MQ135_H

#include "My.h"

#define MQ135_READ_TIMES    10
#define MODE                1

#if MODE
#define MQ135_AO_GPIO_PORT  GPIOA
#define MQ135_AO_GPIO_PIN   GPIO_PIN_2
#define MQ135_ADC_CHANNEL   ADC_CHANNEL_2
#else
#define MQ135_DO_GPIO_PORT  GPIOA
#define MQ135_DO_GPIO_PIN   GPIO_PIN_3
#endif

extern uint8_t MQ135_AlarmFlag;

/* 空气质量采样接口 */
void MQ135_Init(void);
uint16_t MQ135_ADC_Read(void);
uint16_t MQ135_GetData(void);
float MQ135_GetData_PPM(void);
void MQ135_UpdateAlarmFlag(void);

#endif

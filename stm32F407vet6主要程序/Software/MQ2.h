#ifndef __MQ2_H
#define __MQ2_H

#include "My.h"

#define MQ2_READ_TIMES    10

#ifndef MODE
#define MODE              1
#endif

#if MODE
#define MQ2_AO_GPIO_PORT  GPIOA
#define MQ2_AO_GPIO_PIN   GPIO_PIN_0
#define ADC_CHANNEL       ADC_CHANNEL_0
#else
#define MQ2_DO_GPIO_PORT  GPIOA
#define MQ2_DO_GPIO_PIN   GPIO_PIN_1
#endif

extern uint8_t MQ2_AlarmFlag;

/* 烟雾浓度采样接口 */
void MQ2_Init(void);
uint16_t MQ2_ADC_Read(void);
uint16_t MQ2_GetData(void);
float MQ2_GetData_PPM(void);
void MQ2_UpdateAlarmFlag(void);

#endif

#ifndef __SOIL_H
#define __SOIL_H

#include "My.h"

#define SOIL_MODE           1
#define SOIL_READ_TIMES     10

#define SOIL_AO_GPIO_PORT   GPIOA
#define SOIL_AO_GPIO_PIN    GPIO_PIN_5
#define SOIL_ADC_CHANNEL    ADC_CHANNEL_5

#define SOIL_DO_GPIO_PORT   GPIOA
#define SOIL_DO_GPIO_PIN    GPIO_PIN_6

extern float percentSoil;
extern uint8_t Soil_AlarmFlag;

/* 土壤湿度采样接口 */
void Soil_Init(void);
uint16_t Soil_ADC_Read(void);
uint16_t Soil_GetData(void);
float Soil_GetData_Percent(void);
void Soil_UpdateAlarmFlag(void);

#endif

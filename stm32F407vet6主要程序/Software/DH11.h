#ifndef __DH11_H
#define __DH11_H

#include "My.h"

#define DHT11_GPIO_PORT        GPIOB
#define DHT11_GPIO_PIN         GPIO_PIN_5
#define DHT11_EXTI_LINE        GPIO_PIN_5

extern uint8_t DHT11_AlarmFlag;

/* 温湿度模块接口 */
uint8_t DHT11_Init(void);
void DHT11_Task10ms(void);
float DHT11_GetTemperature(void);
float DHT11_GetHumidity(void);
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);
void DH11_UpdateAlarmFlag(void);
void DHT11_EXTI_Callback(uint16_t GPIO_Pin);

#endif

#ifndef _ADC1_H_
#define _ADC1_H_

#include "My.h"

#define ADCx    ADC1

/* ADC 采样接口 */
void ADCx_Init(void);
uint16_t ADC_GetValue(uint32_t ADC_Channel, uint32_t ADC_SampleTime);

#endif

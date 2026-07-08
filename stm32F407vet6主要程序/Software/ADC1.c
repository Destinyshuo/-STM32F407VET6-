#include "ADC1.h"

/* MQ2、MQ135、土壤模拟量共用 ADC1 */
void ADCx_Init(void)
{
    ADC_ChannelConfTypeDef sConfig;

    __HAL_RCC_ADC1_CLK_ENABLE();

    ADCx->CR1 = 0;
    ADCx->CR2 = 0;
    ADCx->SQR1 = 0;
    ADCx->CR1 &= ~ADC_CR1_RES;
    ADCx->CR2 &= ~ADC_CR2_CONT;
    ADCx->CR2 &= ~ADC_CR2_ALIGN;
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |= ADC_CLOCK_SYNC_PCLK_DIV4;

    (void)sConfig;
    ADCx->CR2 |= ADC_CR2_ADON;
}

uint16_t ADC_GetValue(uint32_t ADC_Channel, uint32_t ADC_SampleTime)
{
    ADC_ChannelConfTypeDef sConfig;

    /* 单通道软件触发 */
    sConfig.Channel = ADC_Channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SampleTime;
    sConfig.Offset = 0;

    ADCx->SQR3 = sConfig.Channel;
    ADCx->SMPR2 &= ~(7U << (sConfig.Channel * 3U));
    ADCx->SMPR2 |= (sConfig.SamplingTime << (sConfig.Channel * 3U));

    ADCx->CR2 |= ADC_CR2_SWSTART;
    while ((ADCx->SR & ADC_SR_EOC) == 0U)
    {
    }

    return (uint16_t)ADCx->DR;
}

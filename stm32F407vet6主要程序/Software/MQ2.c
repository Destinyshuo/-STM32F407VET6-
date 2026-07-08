#include "MQ2.h"

uint16_t MQ2value;
char OLED_String2[16];
float MQ2ppm;
uint8_t MQ2_AlarmFlag = 0;

/* MQ2 烟雾/可燃气体 */
static float MQ2_CalcPPM_FromRaw(uint16_t raw)
{
    float Vol = 0.0f;
    float RS = 0.0f;
    float R0 = 6.64f;
    float ppm = 0.0f;

    if (raw >= 4095)
    {
        return 0.0f;
    }

    Vol = (raw * 3.3f / 4096.0f);
    if (Vol <= 0.01f)
    {
        return 999.0f;
    }

    RS = (5.0f - Vol) / (Vol * 0.5f);
    if (RS <= 0.01f)
    {
        return 999.0f;
    }

    ppm = powf(11.5428f * R0 / RS, 0.6549f);
    if (!(ppm >= 0.0f))
    {
        return 0.0f;
    }

    if (ppm > 999.0f)
    {
        return 999.0f;
    }

    return ppm;
}

void MQ2_Init(void)
{
#if MODE
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = MQ2_AO_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MQ2_AO_GPIO_PORT, &GPIO_InitStruct);

    ADCx_Init();
#else
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = MQ2_DO_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MQ2_DO_GPIO_PORT, &GPIO_InitStruct);
#endif
}

#if MODE
uint16_t MQ2_ADC_Read(void)
{
    return ADC_GetValue(ADC_CHANNEL, ADC_SAMPLETIME_56CYCLES);
}
#endif

uint16_t MQ2_GetData(void)
{
#if MODE
    uint32_t tempData = 0;
    uint8_t i;

    for (i = 0; i < MQ2_READ_TIMES; i++)
    {
        tempData += MQ2_ADC_Read();
    }

    tempData /= MQ2_READ_TIMES;
    return (uint16_t)tempData;
#else
    uint16_t tempData;

    tempData = !HAL_GPIO_ReadPin(MQ2_DO_GPIO_PORT, MQ2_DO_GPIO_PIN);
    return tempData;
#endif
}

float MQ2_GetData_PPM(void)
{
#if MODE
    /* 先取 ADC，再换算 ppm */
    MQ2value = MQ2_GetData();
    MQ2ppm = MQ2_CalcPPM_FromRaw(MQ2value);
    return MQ2ppm;
#else
    MQ2ppm = MQ2_GetData() ? 999.0f : 0.0f;
    return MQ2ppm;
#endif
}

void MQ2_UpdateAlarmFlag(void)
{
    if (MQ2ppm >= 3.0f)
    {
        MQ2_AlarmFlag = 1;
    }
    else
    {
        MQ2_AlarmFlag = 0;
    }
}

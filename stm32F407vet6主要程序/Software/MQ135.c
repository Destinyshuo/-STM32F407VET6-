#include "MQ135.h"

char OLED_String[20];
uint16_t value135;
float ppm135;
uint8_t MQ135_AlarmFlag = 0;

/* MQ135 空气质量 */
void MQ135_Init(void)
{
#if MODE
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = MQ135_AO_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MQ135_AO_GPIO_PORT, &GPIO_InitStruct);

    ADCx_Init();
#else
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = MQ135_DO_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MQ135_DO_GPIO_PORT, &GPIO_InitStruct);
#endif
}

#if MODE
uint16_t MQ135_ADC_Read(void)
{
    return ADC_GetValue(MQ135_ADC_CHANNEL, ADC_SAMPLETIME_56CYCLES);
}
#endif

uint16_t MQ135_GetData(void)
{
#if MODE
    uint32_t tempData = 0;
    uint8_t i;

    for (i = 0; i < MQ135_READ_TIMES; i++)
    {
        tempData += MQ135_ADC_Read();
    }

    tempData /= MQ135_READ_TIMES;
    return (uint16_t)tempData;
#else
    uint16_t tempData;

    tempData = !HAL_GPIO_ReadPin(MQ135_DO_GPIO_PORT, MQ135_DO_GPIO_PIN);
    return tempData;
#endif
}

float MQ135_GetData_PPM(void)
{
#if MODE
    float tempData = 0;
    float Pin_Vol;
    float Sensor_Vol;
    float RS;
    float R0;
    float ppm;
    uint8_t i;

    /* 多次采样取平均后再换算 ppm */
    for (i = 0; i < MQ135_READ_TIMES; i++)
    {
        tempData += MQ135_ADC_Read();
    }
    tempData /= MQ135_READ_TIMES;
    value135 = (uint16_t)tempData;

    Pin_Vol = (tempData * 3.3f / 4096.0f);
    if (Pin_Vol <= 0.01f)
    {
        ppm135 = 999.0f;
        return ppm135;
    }

    Sensor_Vol = Pin_Vol * 1.5f;
    RS = (5.0f - Sensor_Vol) / (Sensor_Vol * 0.5f);
    if (RS <= 0.01f)
    {
        ppm135 = 999.0f;
        return ppm135;
    }

    R0 = 6.64f;
    ppm = powf(11.5428f * R0 / RS, 0.6549f);
    if (!(ppm >= 0.0f))
    {
        ppm135 = 0.0f;
        return ppm135;
    }
    if (ppm > 999.0f)
    {
        ppm = 999.0f;
    }
    ppm135 = ppm;

    return ppm135;
#else
    ppm135 = MQ135_GetData() ? 999.0f : 0.0f;
    return ppm135;
#endif
}

void MQ135_UpdateAlarmFlag(void)
{
    if (ppm135 >= 10.0f)
    {
        MQ135_AlarmFlag = 1;
    }
    else
    {
        MQ135_AlarmFlag = 0;
    }
}

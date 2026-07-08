#include "Soil.h"

char OLED_String_Soil[20];
uint16_t valueSoil_Raw;
float percentSoil;
uint8_t Soil_AlarmFlag = 0;

/* 土壤传感器，支持 AO/DO 两种接法 */
void Soil_Init(void)
{
#if SOIL_MODE
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = SOIL_AO_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOIL_AO_GPIO_PORT, &GPIO_InitStruct);

    ADCx_Init();
#else
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = SOIL_DO_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOIL_DO_GPIO_PORT, &GPIO_InitStruct);
#endif
}

#if SOIL_MODE
uint16_t Soil_ADC_Read(void)
{
    return ADC_GetValue(SOIL_ADC_CHANNEL, ADC_SAMPLETIME_84CYCLES);
}
#endif

uint16_t Soil_GetData(void)
{
#if SOIL_MODE
    uint32_t tempData = 0;
    uint8_t i;

    for (i = 0; i < SOIL_READ_TIMES; i++)
    {
        tempData += Soil_ADC_Read();
    }
    tempData /= SOIL_READ_TIMES;
    return (uint16_t)tempData;
#else
    uint16_t tempData;

    tempData = !HAL_GPIO_ReadPin(SOIL_DO_GPIO_PORT, SOIL_DO_GPIO_PIN);
    return tempData;
#endif
}

float Soil_GetData_Percent(void)
{
#if SOIL_MODE
    float tempData = 0;
    float percent;
    uint8_t i;

    for (i = 0; i < SOIL_READ_TIMES; i++)
    {
        tempData += Soil_ADC_Read();
    }
    tempData /= SOIL_READ_TIMES;
    valueSoil_Raw = (uint16_t)tempData;

    percent = (4095.0f - tempData) / 4095.0f * 100.0f;

    if (percent > 100.0f) percent = 100.0f;
    if (percent < 0.0f)   percent = 0.0f;
    percentSoil = percent;

    return percentSoil;
#else
    percentSoil = Soil_GetData() ? 100.0f : 0.0f;
    return percentSoil;
#endif
}

void Soil_UpdateAlarmFlag(void)
{
    float lowThreshold = 5.0f;
    float highThreshold = 25.0f;

    /* 阈值跟随识别到的植物类型 */
    switch (Soil_PlantType)
    {
    case UART4_MODE_SHADE:
        lowThreshold = 5.0f;
        highThreshold = 25.0f;
        break;

    case UART4_MODE_HALF_SHADE:
        lowThreshold = 25.0f;
        highThreshold = 40.0f;
        break;

    case UART4_MODE_SUN:
        lowThreshold = 40.0f;
        highThreshold = 75.0f;
        break;

    default:
        lowThreshold = 5.0f;
        highThreshold = 25.0f;
        break;
    }

    if (percentSoil < lowThreshold)
    {
        Soil_AlarmFlag = 1;
    }
    else if (percentSoil >= highThreshold)
    {
        Soil_AlarmFlag = 0;
    }
    else if (percentSoil > lowThreshold)
    {
        Soil_AlarmFlag = 2;
    }
}

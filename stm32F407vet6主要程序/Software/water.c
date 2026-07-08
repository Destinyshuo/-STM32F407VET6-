#include "water.h"

/* 浇水继电器 */
void water_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = WATER_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(WATER_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(WATER_GPIO_PORT, WATER_GPIO_PIN, GPIO_PIN_RESET);
}

void water_Open(void)
{
    HAL_GPIO_WritePin(WATER_GPIO_PORT, WATER_GPIO_PIN, GPIO_PIN_SET);
}

void water_close(void)
{
    HAL_GPIO_WritePin(WATER_GPIO_PORT, WATER_GPIO_PIN, GPIO_PIN_RESET);
}

void water_auto(uint8_t mode, uint8_t agree)
{
    /* agree=0 直接关水，agree=1 时按土壤状态控制 */
    if (agree == 1)
    {
        if (mode == 1)
        {
            water_Open();
        }
        if (mode == 0 || mode == 2)
        {
            water_close();
        }
    }

    if (agree == 0)
    {
        water_close();
    }
}

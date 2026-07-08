#include "buzzer.h"

#include <rtthread.h>

static rt_thread_t buzzer_thread = RT_NULL;

/* 蜂鸣器报警线程 */
static void buzzer_alarm_entry(void *parameter)
{
    (void)parameter;

    while (1)
    {
        Buzzer_open();
        rt_thread_mdelay(100);
    }
}

void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = BEE_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BEE_GPIO_PROT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(BEE_GPIO_PROT, BEE_GPIO_PIN, GPIO_PIN_SET);

    if (buzzer_thread == RT_NULL)
    {
        buzzer_thread = rt_thread_create("buzzer",
                                         buzzer_alarm_entry,
                                         RT_NULL,
                                         768,
                                         20,
                                         10);
        if (buzzer_thread != RT_NULL)
        {
            rt_thread_startup(buzzer_thread);
        }
    }
}

void Buzzer_open(void)
{
    /* 任一路报警标志有效时鸣叫 */
    if (MQ2_AlarmFlag || MQ135_AlarmFlag || Soil_AlarmFlag || DHT11_AlarmFlag)
    {
        Buzzer_warm(10);
    }
    else
    {
        Buzzer_close();
    }
}

void Buzzer_close(void)
{
    HAL_GPIO_WritePin(BEE_GPIO_PROT, BEE_GPIO_PIN, GPIO_PIN_SET);
}

void Buzzer_warm(uint8_t times)
{
    uint8_t i;

    for (i = 0; i < times; i++)
    {
        HAL_GPIO_WritePin(BEE_GPIO_PROT, BEE_GPIO_PIN, GPIO_PIN_RESET);
        rt_thread_mdelay(50);

        HAL_GPIO_WritePin(BEE_GPIO_PROT, BEE_GPIO_PIN, GPIO_PIN_SET);
        rt_thread_mdelay(50);
    }
}

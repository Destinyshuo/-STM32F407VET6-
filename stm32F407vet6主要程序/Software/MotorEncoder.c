#include "MotorEncoder.h"

int Encoder_Speed1 = 0;
int Encoder_Speed2 = 0;
float Milleage1 = 0.0f;
float Milleage2 = 0.0f;
float Milleage = 0.0f;
uint8_t Milleage1_flag = 1;
uint8_t Milleage2_flag = 2;

static TIM_HandleTypeDef htim_encoder1;
static TIM_HandleTypeDef htim_encoder2;
static uint8_t encoder1_inited = 0;
static uint8_t encoder2_inited = 0;

/* 编码器1：TIM4 PD12/PD13 */
static void encoder_gpio_init_pd12_pd13(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/* 编码器2：TIM3 PA6/PA7 */
static void encoder_gpio_init_pa6_pa7(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Encoder1_Init(void)
{
    TIM_Encoder_InitTypeDef sConfig;

    if (encoder1_inited)
    {
        return;
    }

    __HAL_RCC_TIM4_CLK_ENABLE();
    encoder_gpio_init_pd12_pd13();

    htim_encoder1.Instance = TIM4;
    htim_encoder1.Init.Prescaler = 1 - 1;
    htim_encoder1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_encoder1.Init.Period = 65535 - 1;
    htim_encoder1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_encoder1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0x6;
    sConfig.IC2Polarity = TIM_ICPOLARITY_FALLING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0x6;

    (void)HAL_TIM_Encoder_Init(&htim_encoder1, &sConfig);

    __HAL_TIM_SET_COUNTER(&htim_encoder1, 0);
    (void)HAL_TIM_Encoder_Start(&htim_encoder1, TIM_CHANNEL_ALL);

    encoder1_inited = 1;
}

void Encoder2_Init(void)
{
    TIM_Encoder_InitTypeDef sConfig;

    if (encoder2_inited)
    {
        return;
    }

    __HAL_RCC_TIM3_CLK_ENABLE();
    encoder_gpio_init_pa6_pa7();

    htim_encoder2.Instance = TIM3;
    htim_encoder2.Init.Prescaler = 1 - 1;
    htim_encoder2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_encoder2.Init.Period = 65535 - 1;
    htim_encoder2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_encoder2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0x6;
    sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0x6;

    (void)HAL_TIM_Encoder_Init(&htim_encoder2, &sConfig);

    __HAL_TIM_SET_COUNTER(&htim_encoder2, 0);
    (void)HAL_TIM_Encoder_Start(&htim_encoder2, TIM_CHANNEL_ALL);

    encoder2_inited = 1;
}

void Encoder_Init(void)
{
    Encoder1_Init();
    Encoder2_Init();
}

void GetSpeed_Milleage_1(int *EncoderSpeed, float *EncoderMilleage, uint8_t *Mode)
{
    /* 读取后清零，10ms 周期累计里程 */
    if (!(*Mode))
    {
        *EncoderMilleage = 0;
        *Mode = 1;
    }
    else
    {
        *EncoderSpeed = (int16_t)__HAL_TIM_GET_COUNTER(&htim_encoder1);

        if (*EncoderSpeed < 0)
        {
            *EncoderMilleage -= (int16_t)__HAL_TIM_GET_COUNTER(&htim_encoder1) * 204.1f / 60000.0f;
        }
        else
        {
            *EncoderMilleage += (int16_t)__HAL_TIM_GET_COUNTER(&htim_encoder1) * 204.1f / 60000.0f;
        }
    }

    __HAL_TIM_SET_COUNTER(&htim_encoder1, 0);
}

void GetSpeed_Milleage_2(int *EncoderSpeed, float *EncoderMilleage, uint8_t *Mode)
{
    /* 读取后清零，10ms 周期累计里程 */
    if (!(*Mode))
    {
        *EncoderMilleage = 0;
        *Mode = 1;
    }
    else
    {
        *EncoderSpeed = (int16_t)__HAL_TIM_GET_COUNTER(&htim_encoder2);

        if (*EncoderSpeed < 0)
        {
            *EncoderMilleage -= (int16_t)__HAL_TIM_GET_COUNTER(&htim_encoder2) * 204.1f / 60000.0f;
        }
        else
        {
            *EncoderMilleage += (int16_t)__HAL_TIM_GET_COUNTER(&htim_encoder2) * 204.1f / 60000.0f;
        }
    }

    __HAL_TIM_SET_COUNTER(&htim_encoder2, 0);
}

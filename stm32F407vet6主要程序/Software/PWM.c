#include "PWM.h"

uint8_t lightONOFF_flag = 0;

static TIM_HandleTypeDef htim1_led;
static uint8_t led_pwm_inited = 0;

/* 补光灯 PWM */
void LED_PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_OC_InitTypeDef sConfigOC;

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitStruct.Pin = LED_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(LED_GPIO_PROT, &GPIO_InitStruct);

    htim1_led.Instance = TIM1;
    htim1_led.Init.Prescaler = 720 - 1;
    htim1_led.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1_led.Init.Period = 100 - 1;
    htim1_led.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1_led.Init.RepetitionCounter = 0;
    htim1_led.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    (void)HAL_TIM_PWM_Init(&htim1_led);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    (void)HAL_TIM_PWM_ConfigChannel(&htim1_led, &sConfigOC, TIM_CHANNEL_3);
    (void)HAL_TIM_PWM_Start(&htim1_led, TIM_CHANNEL_3);

    led_pwm_inited = 1;
}

void PWM_SetCompare3(uint16_t Compare)
{
    if (!led_pwm_inited)
    {
        LED_PWM_Init();
    }

    __HAL_TIM_SET_COMPARE(&htim1_led, TIM_CHANNEL_3, Compare);
}

void LED_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_GPIO_PROT, LED_GPIO_PIN);
}

void LED_PWM(uint16_t target_pwm)
{
    static uint16_t i = 0;

    /* 亮度缓慢跟随目标值 */
    if (target_pwm > 100)
    {
        target_pwm = 100;
    }

    if (i < target_pwm)
    {
        i++;
    }
    else if (i > target_pwm)
    {
        i--;
    }

    PWM_SetCompare3(i);
}

void LightContral(uint8_t contral_flag)
{
    /* 按植物类型给补光档位 */
    if (contral_flag)
    {
        switch (Soil_PlantType)
        {
        case UART4_MODE_SHADE:
            LED_PWM(50);
            break;

        case UART4_MODE_HALF_SHADE:
            LED_PWM(75);
            break;

        case UART4_MODE_SUN:
            LED_PWM(100);
            break;

        default:
            break;
        }
    }
    else
    {
        LED_PWM(0);
    }
}

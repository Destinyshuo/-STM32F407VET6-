#include "TB6612.h"

static TIM_HandleTypeDef htim12_motor;
static uint8_t motor_pwm_inited = 0;

/* TB6612 左右轮 PWM */
void PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_OC_InitTypeDef sConfigOC;

    if (motor_pwm_inited)
    {
        return;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_TIM12_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_10 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_TIM12;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    htim12_motor.Instance = TIM12;
    htim12_motor.Init.Prescaler = 1 - 1;
    htim12_motor.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim12_motor.Init.Period = 4200 - 1;
    htim12_motor.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim12_motor.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    (void)HAL_TIM_PWM_Init(&htim12_motor);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    (void)HAL_TIM_PWM_ConfigChannel(&htim12_motor, &sConfigOC, TIM_CHANNEL_1);
    (void)HAL_TIM_PWM_ConfigChannel(&htim12_motor, &sConfigOC, TIM_CHANNEL_2);

    (void)HAL_TIM_PWM_Start(&htim12_motor, TIM_CHANNEL_1);
    (void)HAL_TIM_PWM_Start(&htim12_motor, TIM_CHANNEL_2);

    AIN_SET();
    BIN_SET();
    __HAL_TIM_SET_COMPARE(&htim12_motor, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim12_motor, TIM_CHANNEL_2, 0);

    motor_pwm_inited = 1;
}

void Motor_SetCCR(int32_t MotorCCR1, int32_t MotorCCR2)
{
    /* 符号表示方向，绝对值写入占空比 */
    if (!motor_pwm_inited)
    {
        PWM_Init();
    }

    if (MotorCCR1 >= 0)
    {
        AIN_SET();
    }
    else
    {
        AIN_RESET();
        MotorCCR1 = -MotorCCR1;
    }

    if (MotorCCR1 > 4199)
    {
        MotorCCR1 = 4199;
    }

    if (MotorCCR2 >= 0)
    {
        BIN_SET();
    }
    else
    {
        BIN_RESET();
        MotorCCR2 = -MotorCCR2;
    }

    if (MotorCCR2 > 4199)
    {
        MotorCCR2 = 4199;
    }

    __HAL_TIM_SET_COMPARE(&htim12_motor, TIM_CHANNEL_1, (uint32_t)MotorCCR1);
    __HAL_TIM_SET_COMPARE(&htim12_motor, TIM_CHANNEL_2, (uint32_t)MotorCCR2);
}

void Motor_PidTarget(float Speed1, float Speed2)
{
    Pid_Motor1Speed.Target = Speed1;
    Pid_Motor2Speed.Target = Speed2;

    Motor_SetCCR(PID_iOut_Max(&Pid_Motor1Speed, Encoder_Speed1, 4199),
                 PID_iOut_Max(&Pid_Motor2Speed, Encoder_Speed2, 4199));
}

void Motor_PidTarget_Acce(float Speed1, float Speed2)
{
    static float Acce = 40.0f;

    /* 加速过程目标速度逐步逼近 */
    if (Speed1 >= 0)
    {
        if (Pid_Motor1Speed.Target < Speed1) Pid_Motor1Speed.Target += Acce;
        if (Pid_Motor1Speed.Target >= Speed1) Pid_Motor1Speed.Target = Speed1;
    }
    if (Speed1 < 0)
    {
        if (Pid_Motor1Speed.Target > Speed1) Pid_Motor1Speed.Target -= Acce;
        if (Pid_Motor1Speed.Target <= Speed1) Pid_Motor1Speed.Target = Speed1;
    }
    if (Speed2 >= 0)
    {
        if (Pid_Motor2Speed.Target < Speed2) Pid_Motor2Speed.Target += Acce;
        if (Pid_Motor2Speed.Target >= Speed2) Pid_Motor2Speed.Target = Speed2;
    }
    if (Speed2 < 0)
    {
        if (Pid_Motor2Speed.Target > Speed2) Pid_Motor2Speed.Target -= Acce;
        if (Pid_Motor2Speed.Target <= Speed2) Pid_Motor2Speed.Target = Speed2;
    }

    Motor_SetCCR(PID_iOut_Max(&Pid_Motor1Speed, Encoder_Speed1, 4199),
                 PID_iOut_Max(&Pid_Motor2Speed, Encoder_Speed2, 4199));
}

void Motor_PidTarget_Dece(float Speed1, float Speed2)
{
    static float Dece = 40.0f;

    /* 减速过程目标速度逐步回落 */
    if (Speed1 >= 0)
    {
        if (Pid_Motor1Speed.Target > Speed1) Pid_Motor1Speed.Target -= Dece;
        if (Pid_Motor1Speed.Target <= Speed1) Pid_Motor1Speed.Target = Speed1;
    }
    if (Speed1 < 0)
    {
        if (Pid_Motor1Speed.Target < Speed1) Pid_Motor1Speed.Target += Dece;
        if (Pid_Motor1Speed.Target >= Speed1) Pid_Motor1Speed.Target = Speed1;
    }
    if (Speed2 >= 0)
    {
        if (Pid_Motor2Speed.Target > Speed2) Pid_Motor2Speed.Target -= Dece;
        if (Pid_Motor2Speed.Target <= Speed2) Pid_Motor2Speed.Target = Speed2;
    }
    if (Speed2 < 0)
    {
        if (Pid_Motor2Speed.Target < Speed2) Pid_Motor2Speed.Target += Dece;
        if (Pid_Motor2Speed.Target >= Speed2) Pid_Motor2Speed.Target = Speed2;
    }

    Motor_SetCCR(PID_iOut_Max(&Pid_Motor1Speed, Encoder_Speed1, 4199),
                 PID_iOut_Max(&Pid_Motor2Speed, Encoder_Speed2, 4199));
}

void Motor_Stop(void)
{
    Motor_PidTarget(0, 0);
}

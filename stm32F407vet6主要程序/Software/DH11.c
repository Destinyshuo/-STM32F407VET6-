#include "DH11.h"

typedef enum
{
    /* 上电等待 -> 空闲 -> 起始拉低 -> 等待一帧完成 */
    DHT11_STATE_POWERUP_WAIT = 0,
    DHT11_STATE_IDLE,
    DHT11_STATE_START_LOW,
    DHT11_STATE_WAIT_FRAME
} dht11_state_t;

typedef enum
{
    DHT11_CAPTURE_WAIT_RESP_FALL = 0,
    DHT11_CAPTURE_WAIT_RESP_RISE,
    DHT11_CAPTURE_WAIT_FIRST_DATA_FALL,
    DHT11_CAPTURE_WAIT_BIT_RISE,
    DHT11_CAPTURE_WAIT_BIT_FALL
} dht11_capture_state_t;

#define DHT11_POWERUP_WAIT_TICKS      100U
#define DHT11_START_LOW_TICKS         2U
#define DHT11_SAMPLE_INTERVAL_TICKS   100U
#define DHT11_FRAME_TIMEOUT_TICKS     2U
#define DHT11_BIT_HIGH_THRESHOLD_US   50U

static volatile dht11_state_t s_dht11_state = DHT11_STATE_POWERUP_WAIT;
static volatile dht11_capture_state_t s_capture_state = DHT11_CAPTURE_WAIT_RESP_FALL;
static volatile uint16_t s_state_ticks = DHT11_POWERUP_WAIT_TICKS;
static volatile uint8_t s_frame_bytes[5] = {0};
static volatile uint8_t s_bit_index = 0;
static volatile uint8_t s_frame_done = 0;
static volatile uint8_t s_frame_error = 0;
static volatile uint8_t s_transaction_active = 0;
static volatile uint32_t s_high_start_cycle = 0;
static volatile uint32_t s_cycles_per_us = 0;
static volatile float s_temperature_c = 0.0f;
static volatile float s_humidity_rh = 0.0f;
static volatile uint8_t s_has_valid_data = 0;

uint8_t DHT11_AlarmFlag = 0;

static void DHT11_Set_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

static void DHT11_Set_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

static void DHT11_Line_High(void)
{
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
}

static void DHT11_Line_Low(void)
{
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
}

static uint8_t DHT11_Read_IO(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
}

static void DHT11_DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    s_cycles_per_us = SystemCoreClock / 1000000U;
    if (s_cycles_per_us == 0U)
    {
        s_cycles_per_us = 1U;
    }
}

static uint32_t DHT11_ElapsedUs(uint32_t start_cycle, uint32_t end_cycle)
{
    return (end_cycle - start_cycle) / s_cycles_per_us;
}

static void DHT11_EXTI_Enable(rt_bool_t state)
{
    if (state)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(DHT11_EXTI_LINE);
        EXTI->IMR |= DHT11_EXTI_LINE;
    }
    else
    {
        EXTI->IMR &= ~DHT11_EXTI_LINE;
        __HAL_GPIO_EXTI_CLEAR_IT(DHT11_EXTI_LINE);
    }
}

static void DHT11_EXTI_Init(void)
{
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    DHT11_EXTI_Enable(RT_FALSE);
}

static void DHT11_Reset_Capture(void)
{
    uint8_t i;

    for (i = 0; i < 5U; i++)
    {
        s_frame_bytes[i] = 0U;
    }

    s_bit_index = 0U;
    s_frame_done = 0U;
    s_frame_error = 0U;
    s_capture_state = DHT11_CAPTURE_WAIT_RESP_FALL;
    s_high_start_cycle = 0U;
}

static void DHT11_Enter_Idle(uint16_t wait_ticks)
{
    DHT11_EXTI_Enable(RT_FALSE);
    s_transaction_active = 0U;
    s_dht11_state = DHT11_STATE_IDLE;
    s_state_ticks = wait_ticks;
    DHT11_Set_Output();
    DHT11_Line_High();
}

static void DHT11_Start_Request(void)
{
    DHT11_Reset_Capture();
    DHT11_Set_Output();
    DHT11_Line_Low();
    s_dht11_state = DHT11_STATE_START_LOW;
    s_state_ticks = DHT11_START_LOW_TICKS;
}

static void DHT11_Release_And_Listen(void)
{
    DHT11_Line_High();
    DHT11_Set_Input();
    DHT11_Reset_Capture();
    s_transaction_active = 1U;
    s_dht11_state = DHT11_STATE_WAIT_FRAME;
    s_state_ticks = DHT11_FRAME_TIMEOUT_TICKS;
    DHT11_EXTI_Enable(RT_TRUE);
}

static void DHT11_Process_Frame(void)
{
    uint8_t checksum;

    checksum = (uint8_t)(s_frame_bytes[0] + s_frame_bytes[1] + s_frame_bytes[2] + s_frame_bytes[3]);
    if (checksum == s_frame_bytes[4])
    {
        s_humidity_rh = (float)s_frame_bytes[0] + ((float)s_frame_bytes[1] * 0.1f);
        s_temperature_c = (float)s_frame_bytes[2] + ((float)s_frame_bytes[3] * 0.1f);
        s_has_valid_data = 1U;
        DHT11_AlarmFlag = 0U;
    }

    DHT11_Enter_Idle(DHT11_SAMPLE_INTERVAL_TICKS);
}

static void DHT11_Handle_Failed_Frame(void)
{
    DHT11_AlarmFlag = 0U;
    DHT11_Enter_Idle(DHT11_SAMPLE_INTERVAL_TICKS);
}

uint8_t DHT11_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    DHT11_DWT_Init();
    DHT11_EXTI_Init();

    DHT11_Set_Output();
    DHT11_Line_High();

    s_temperature_c = 0.0f;
    s_humidity_rh = 0.0f;
    s_has_valid_data = 0U;
    s_dht11_state = DHT11_STATE_POWERUP_WAIT;
    s_state_ticks = DHT11_POWERUP_WAIT_TICKS;
    s_transaction_active = 0U;
    s_frame_done = 0U;
    s_frame_error = 0U;
    DHT11_AlarmFlag = 0U;

    return 0U;
}

void DHT11_Task10ms(void)
{
    /* DHT11 的时序节拍放在 10ms 主任务里跑 */
    if (s_frame_done != 0U)
    {
        s_frame_done = 0U;
        DHT11_Process_Frame();
        return;
    }

    if (s_frame_error != 0U)
    {
        s_frame_error = 0U;
        DHT11_Handle_Failed_Frame();
        return;
    }

    switch (s_dht11_state)
    {
    case DHT11_STATE_POWERUP_WAIT:
        if (s_state_ticks > 0U)
        {
            s_state_ticks--;
        }
        else
        {
            DHT11_Start_Request();
        }
        break;

    case DHT11_STATE_IDLE:
        if (s_state_ticks > 0U)
        {
            s_state_ticks--;
        }
        else
        {
            DHT11_Start_Request();
        }
        break;

    case DHT11_STATE_START_LOW:
        if (s_state_ticks > 0U)
        {
            s_state_ticks--;
        }
        if (s_state_ticks == 0U)
        {
            DHT11_Release_And_Listen();
        }
        break;

    case DHT11_STATE_WAIT_FRAME:
        if (s_state_ticks > 0U)
        {
            s_state_ticks--;
        }
        else
        {
            DHT11_Handle_Failed_Frame();
        }
        break;

    default:
        DHT11_Enter_Idle(DHT11_SAMPLE_INTERVAL_TICKS);
        break;
    }
}

float DHT11_GetTemperature(void)
{
    return s_temperature_c;
}

float DHT11_GetHumidity(void)
{
    return s_humidity_rh;
}

uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    if ((temp == RT_NULL) || (humi == RT_NULL) || (s_has_valid_data == 0U))
    {
        return 1U;
    }

    *temp = (uint8_t)s_temperature_c;
    *humi = (uint8_t)s_humidity_rh;
    return 0U;
}

void DH11_UpdateAlarmFlag(void)
{
    DHT11_AlarmFlag = 0U;
}

void DHT11_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now_cycle;
    uint32_t high_us;
    uint8_t bit_value;
    uint8_t byte_index;

    if (GPIO_Pin != DHT11_GPIO_PIN)
    {
        return;
    }

    if (s_transaction_active == 0U)
    {
        return;
    }

    now_cycle = DWT->CYCCNT;

    if (DHT11_Read_IO() == 0U)
    {
        /* 下降沿：响应结束或数据位高电平结束 */
        switch (s_capture_state)
        {
        case DHT11_CAPTURE_WAIT_RESP_FALL:
            s_capture_state = DHT11_CAPTURE_WAIT_RESP_RISE;
            break;

        case DHT11_CAPTURE_WAIT_FIRST_DATA_FALL:
            s_capture_state = DHT11_CAPTURE_WAIT_BIT_RISE;
            break;

        case DHT11_CAPTURE_WAIT_BIT_FALL:
            high_us = DHT11_ElapsedUs(s_high_start_cycle, now_cycle);
            bit_value = (high_us > DHT11_BIT_HIGH_THRESHOLD_US) ? 1U : 0U;
            byte_index = (uint8_t)(s_bit_index >> 3);
            s_frame_bytes[byte_index] <<= 1;
            s_frame_bytes[byte_index] |= bit_value;
            s_bit_index++;

            if (s_bit_index >= 40U)
            {
                s_transaction_active = 0U;
                DHT11_EXTI_Enable(RT_FALSE);
                s_frame_done = 1U;
            }
            else
            {
                s_capture_state = DHT11_CAPTURE_WAIT_BIT_RISE;
            }
            break;

        default:
            s_transaction_active = 0U;
            DHT11_EXTI_Enable(RT_FALSE);
            s_frame_error = 1U;
            break;
        }
    }
    else
    {
        /* 上升沿：响应开始或数据位高电平开始 */
        switch (s_capture_state)
        {
        case DHT11_CAPTURE_WAIT_RESP_RISE:
            s_capture_state = DHT11_CAPTURE_WAIT_FIRST_DATA_FALL;
            break;

        case DHT11_CAPTURE_WAIT_BIT_RISE:
            s_high_start_cycle = now_cycle;
            s_capture_state = DHT11_CAPTURE_WAIT_BIT_FALL;
            break;

        default:
            s_transaction_active = 0U;
            DHT11_EXTI_Enable(RT_FALSE);
            s_frame_error = 1U;
            break;
        }
    }
}

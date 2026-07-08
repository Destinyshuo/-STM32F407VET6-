#include "usart3_port.h"
#include "Emm_V5.h"

#ifdef RT_USING_DEVICE
#include <rtdevice.h>
#endif

#define USART3_PORT_DEVICE_NAME "uart3"

static rt_device_t usart3_port_dev = RT_NULL;

/* USART3: 步进电机 */
static rt_err_t usart3_port_open(void)
{
    if (usart3_port_dev == RT_NULL)
    {
        usart3_port_dev = rt_device_find(USART3_PORT_DEVICE_NAME);
        if (usart3_port_dev == RT_NULL)
        {
            return -RT_ERROR;
        }
    }

    if ((usart3_port_dev->open_flag & RT_DEVICE_OFLAG_OPEN) == 0)
    {
        return rt_device_open(usart3_port_dev, RT_DEVICE_OFLAG_RDWR);
    }

    return RT_EOK;
}

void usart_SendCmd(volatile uint8_t *cmd, uint8_t len)
{
    if ((cmd == RT_NULL) || (len == 0))
    {
        return;
    }

    if (usart3_port_open() != RT_EOK)
    {
        return;
    }

    rt_device_write(usart3_port_dev, 0, (const void *)cmd, len);
}

void BuJinMotor_X_Speed(uint8_t Addr, int8_t Speed)
{
    uint8_t Dir;

    if (Speed < 0)
    {
        Dir = 0;
        Speed = -Speed;
    }
    else
    {
        Dir = 1;
    }

    Emm_V5_Vel_Control(Addr, Dir, (uint16_t)Speed, 0, 0);
}

void BuJinMotor_Y_Speed(uint8_t Addr, int8_t Speed)
{
    uint8_t Dir;

    if (Speed < 0)
    {
        Dir = 0;
        Speed = -Speed;
    }
    else
    {
        Dir = 1;
    }

    Emm_V5_Vel_Control(Addr, Dir, (uint16_t)Speed, 0, 0);
    rt_thread_mdelay(5);
}

void BuJinMotor_Circle(uint8_t Addr, uint8_t Dir, uint16_t Speed, uint8_t Circle)
{
    Emm_V5_Pos_Control(Addr, Dir, Speed, 0, (uint32_t)Circle * 3200, 0, 0);
}

void BuJinMotor_PitchView(uint8_t Addr, uint8_t Dir, uint16_t Speed, float Pitch)
{
    uint16_t Clk;

    (void)Addr;
    Clk = (uint16_t)(3200.0f * (Pitch / 360.0f));

    Emm_V5_Pos_Control(2, Dir, Speed, 0, Clk, 1, 0);
}

float calculate_slope(float x1, float y1, float x2, float y2, float *k, float *b)
{
    if (fabsf(x1 - x2) >= 1e-9f)
    {
        *k = (y2 - y1) / (x2 - x1);
        *b = y1 - (*k * x1);
    }
    else
    {
        return 0;
    }

    return 1;
}

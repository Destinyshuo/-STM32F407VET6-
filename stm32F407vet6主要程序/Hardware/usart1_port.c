#include "VL53_DMA.h"

#ifdef RT_USING_DEVICE
#include <rtdevice.h>
#endif

#define USART1_PORT_DEVICE_NAME "uart1"
#define USART1_RX_THREAD_STACK_SIZE 1024
#define USART1_RX_THREAD_PRIORITY 15
#define USART1_RX_THREAD_TICK 10

uint8_t USART1DMA_RX_BUF[USART1_MAX_LEN] = {0};
uint8_t VL53_rx_len;

static uint8_t BufferA_U1[USART1_MAX_LEN / 2];
static uint8_t BufferB_U1[USART1_MAX_LEN / 2];
static volatile uint8_t ActiveBuffer_U1 = 0;

static rt_device_t usart1_port_dev = RT_NULL;
static struct rt_semaphore usart1_rx_sem;
static rt_thread_t usart1_rx_thread = RT_NULL;
static rt_bool_t usart1_rx_sem_inited = RT_FALSE;

static void usart1_shift_left(uint8_t *buffer, uint16_t *length, uint16_t count)
{
    if (count >= *length)
    {
        *length = 0;
        return;
    }

    memmove(buffer, buffer + count, *length - count);
    *length -= count;
}

/* USART1: VL53 数据 */
static void usart1_try_parse(uint8_t *buffer, uint16_t *length)
{
    uint16_t i = 0;

    while ((i + VL53pack_num) <= *length)
    {
        if (buffer[i] == VL53pack_head_1 &&
            buffer[i + 1] == VL53pack_head_2 &&
            buffer[i + VL53pack_num - 1] == VL53pack_tail)
        {
            VL53_DataProc(&buffer[i], VL53pack_num);
            usart1_shift_left(buffer, length, i + VL53pack_num);
            i = 0;
            continue;
        }

        i++;
    }

    if (i > 0)
    {
        usart1_shift_left(buffer, length, i);
    }
}

static rt_err_t usart1_rx_ind(rt_device_t dev, rt_size_t size)
{
    (void)dev;
    (void)size;

    if (usart1_rx_sem_inited)
    {
        rt_sem_release(&usart1_rx_sem);
    }

    return RT_EOK;
}

static void usart1_rx_entry(void *parameter)
{
    uint8_t parse_buf[USART1_MAX_LEN] = {0};
    uint16_t parse_len = 0;
    rt_size_t size;

    (void)parameter;

    while (1)
    {
        if (rt_sem_take(&usart1_rx_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        do
        {
            size = rt_device_read(usart1_port_dev, 0, USART1DMA_RX_BUF, USART1_MAX_LEN);
            if (size > 0)
            {
                if ((parse_len + size) > USART1_MAX_LEN)
                {
                    parse_len = 0;
                }

                memcpy(parse_buf + parse_len, USART1DMA_RX_BUF, size);
                parse_len += (uint16_t)size;
                VL53_rx_len = (uint8_t)size;
                usart1_try_parse(parse_buf, &parse_len);
            }
        } while (size > 0);
    }
}

static rt_err_t usart1_port_open(void)
{
    if (usart1_port_dev == RT_NULL)
    {
        usart1_port_dev = rt_device_find(USART1_PORT_DEVICE_NAME);
        if (usart1_port_dev == RT_NULL)
        {
            return -RT_ERROR;
        }
    }

    if (!usart1_rx_sem_inited)
    {
        if (rt_sem_init(&usart1_rx_sem, "u1rx", 0, RT_IPC_FLAG_FIFO) != RT_EOK)
        {
            return -RT_ERROR;
        }
        usart1_rx_sem_inited = RT_TRUE;
    }

    if ((usart1_port_dev->open_flag & RT_DEVICE_OFLAG_OPEN) == 0)
    {
        if (rt_device_open(usart1_port_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    rt_device_set_rx_indicate(usart1_port_dev, usart1_rx_ind);

    if (usart1_rx_thread == RT_NULL)
    {
        usart1_rx_thread = rt_thread_create("u1rx",
                                            usart1_rx_entry,
                                            RT_NULL,
                                            USART1_RX_THREAD_STACK_SIZE,
                                            USART1_RX_THREAD_PRIORITY,
                                            USART1_RX_THREAD_TICK);
        if (usart1_rx_thread == RT_NULL)
        {
            return -RT_ERROR;
        }

        rt_thread_startup(usart1_rx_thread);
    }

    return RT_EOK;
}

void USART1DMA_Init(uint32_t __baud)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (usart1_port_open() != RT_EOK)
    {
        return;
    }

    config.baud_rate = __baud;
    rt_device_control(usart1_port_dev, RT_DEVICE_CTRL_CONFIG, &config);
}

void Usart1DMA_SendData(uint8_t *data, uint16_t size)
{
    uint8_t *target;

    if ((data == RT_NULL) || (size == 0))
    {
        return;
    }

    if (usart1_port_open() != RT_EOK)
    {
        return;
    }

    target = (ActiveBuffer_U1 == 0) ? BufferA_U1 : BufferB_U1;
    if (size > (USART1_MAX_LEN / 2))
    {
        size = USART1_MAX_LEN / 2;
    }

    memcpy(target, data, size);
    rt_device_write(usart1_port_dev, 0, target, size);
    ActiveBuffer_U1 ^= 1;
}

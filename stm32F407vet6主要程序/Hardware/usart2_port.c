#include "Usart2DMA_IMU.h"

#ifdef RT_USING_DEVICE
#include <rtdevice.h>
#endif

#define USART2_PORT_DEVICE_NAME "uart2"
#define USART2_RX_THREAD_STACK_SIZE 1024
#define USART2_RX_THREAD_PRIORITY 15
#define USART2_RX_THREAD_TICK 10
#define HWT101_FRAME_LEN 11

uint8_t USART2DMA_RX_BUF[USART2_MAX_LEN] = {0};
uint8_t HWT101_rx_len;

static uint8_t BufferA_U2[USART2_MAX_LEN / 2];
static uint8_t BufferB_U2[USART2_MAX_LEN / 2];
static volatile uint8_t ActiveBuffer_U2 = 0;

static rt_device_t usart2_port_dev = RT_NULL;
static struct rt_semaphore usart2_rx_sem;
static rt_thread_t usart2_rx_thread = RT_NULL;
static rt_bool_t usart2_rx_sem_inited = RT_FALSE;

static void usart2_shift_left(uint8_t *buffer, uint16_t *length, uint16_t count)
{
    if (count >= *length)
    {
        *length = 0;
        return;
    }

    memmove(buffer, buffer + count, *length - count);
    *length -= count;
}

/* USART2: HWT101 IMU */
static void usart2_try_parse(uint8_t *buffer, uint16_t *length)
{
    uint16_t i = 0;

    while ((i + HWT101_FRAME_LEN) <= *length)
    {
        if (buffer[i] == 0x55 && buffer[i + 1] == 0x53)
        {
            HWT101_DataProc(&buffer[i], HWT101_FRAME_LEN);
            usart2_shift_left(buffer, length, i + HWT101_FRAME_LEN);
            i = 0;
            continue;
        }

        i++;
    }

    if (i > 0)
    {
        usart2_shift_left(buffer, length, i);
    }
}

static rt_err_t usart2_rx_ind(rt_device_t dev, rt_size_t size)
{
    (void)dev;
    (void)size;

    if (usart2_rx_sem_inited)
    {
        rt_sem_release(&usart2_rx_sem);
    }

    return RT_EOK;
}

static void usart2_rx_entry(void *parameter)
{
    uint8_t parse_buf[USART2_MAX_LEN] = {0};
    uint16_t parse_len = 0;
    rt_size_t size;

    (void)parameter;

    while (1)
    {
        if (rt_sem_take(&usart2_rx_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        do
        {
            size = rt_device_read(usart2_port_dev, 0, USART2DMA_RX_BUF, USART2_MAX_LEN);
            if (size > 0)
            {
                if ((parse_len + size) > USART2_MAX_LEN)
                {
                    parse_len = 0;
                }

                memcpy(parse_buf + parse_len, USART2DMA_RX_BUF, size);
                parse_len += (uint16_t)size;
                HWT101_rx_len = (uint8_t)size;
                usart2_try_parse(parse_buf, &parse_len);
            }
        } while (size > 0);
    }
}

static rt_err_t usart2_port_open(void)
{
    if (usart2_port_dev == RT_NULL)
    {
        usart2_port_dev = rt_device_find(USART2_PORT_DEVICE_NAME);
        if (usart2_port_dev == RT_NULL)
        {
            return -RT_ERROR;
        }
    }

    if (!usart2_rx_sem_inited)
    {
        if (rt_sem_init(&usart2_rx_sem, "u2rx", 0, RT_IPC_FLAG_FIFO) != RT_EOK)
        {
            return -RT_ERROR;
        }
        usart2_rx_sem_inited = RT_TRUE;
    }

    if ((usart2_port_dev->open_flag & RT_DEVICE_OFLAG_OPEN) == 0)
    {
        if (rt_device_open(usart2_port_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    rt_device_set_rx_indicate(usart2_port_dev, usart2_rx_ind);

    if (usart2_rx_thread == RT_NULL)
    {
        usart2_rx_thread = rt_thread_create("u2rx",
                                            usart2_rx_entry,
                                            RT_NULL,
                                            USART2_RX_THREAD_STACK_SIZE,
                                            USART2_RX_THREAD_PRIORITY,
                                            USART2_RX_THREAD_TICK);
        if (usart2_rx_thread == RT_NULL)
        {
            return -RT_ERROR;
        }

        rt_thread_startup(usart2_rx_thread);
    }

    return RT_EOK;
}

void Usart2DMA_Init(uint32_t __baud)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (usart2_port_open() != RT_EOK)
    {
        return;
    }

    config.baud_rate = __baud;
    rt_device_control(usart2_port_dev, RT_DEVICE_CTRL_CONFIG, &config);
}

void Usart2DMA_SendData(uint8_t *data, uint16_t size)
{
    uint8_t *target;

    if ((data == RT_NULL) || (size == 0))
    {
        return;
    }

    if (usart2_port_open() != RT_EOK)
    {
        return;
    }

    target = (ActiveBuffer_U2 == 0) ? BufferA_U2 : BufferB_U2;
    if (size > (USART2_MAX_LEN / 2))
    {
        size = USART2_MAX_LEN / 2;
    }

    memcpy(target, data, size);
    rt_device_write(usart2_port_dev, 0, target, size);
    ActiveBuffer_U2 ^= 1;
}

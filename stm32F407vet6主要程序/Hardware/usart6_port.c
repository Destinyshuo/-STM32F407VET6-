#include "usart6.h"

#ifdef RT_USING_DEVICE
#include <rtdevice.h>
#endif

#define USART6_PORT_DEVICE_NAME "uart6"
#define USART6_RX_THREAD_STACK_SIZE 1024
#define USART6_RX_THREAD_PRIORITY 15
#define USART6_RX_THREAD_TICK 10

uint8_t USART6DMA_RX_BUF[USART6_MAX_LEN] = {0};
uint8_t USART6_rx_len;

static uint8_t BufferA_U6[USART6_MAX_LEN / 2];
static uint8_t BufferB_U6[USART6_MAX_LEN / 2];
static volatile uint8_t ActiveBuffer_U6 = 0;

static rt_device_t usart6_port_dev = RT_NULL;
static struct rt_semaphore usart6_rx_sem;
static rt_thread_t usart6_rx_thread = RT_NULL;
static rt_bool_t usart6_rx_sem_inited = RT_FALSE;

/* USART6: 语音模块 */
static rt_err_t usart6_rx_ind(rt_device_t dev, rt_size_t size)
{
    (void)dev;
    (void)size;

    if (usart6_rx_sem_inited)
    {
        rt_sem_release(&usart6_rx_sem);
    }

    return RT_EOK;
}

static void usart6_rx_entry(void *parameter)
{
    rt_size_t size;

    (void)parameter;

    while (1)
    {
        if (rt_sem_take(&usart6_rx_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        do
        {
            size = rt_device_read(usart6_port_dev, 0, USART6DMA_RX_BUF, USART6_MAX_LEN);
            if (size > 0)
            {
                USART6_rx_len = (uint8_t)size;
                USART6_DataProc(USART6DMA_RX_BUF, USART6_rx_len);
            }
        } while (size > 0);
    }
}

static rt_err_t usart6_port_open(void)
{
    if (usart6_port_dev == RT_NULL)
    {
        usart6_port_dev = rt_device_find(USART6_PORT_DEVICE_NAME);
        if (usart6_port_dev == RT_NULL)
        {
            return -RT_ERROR;
        }
    }

    if (!usart6_rx_sem_inited)
    {
        if (rt_sem_init(&usart6_rx_sem, "u6rx", 0, RT_IPC_FLAG_FIFO) != RT_EOK)
        {
            return -RT_ERROR;
        }
        usart6_rx_sem_inited = RT_TRUE;
    }

    if ((usart6_port_dev->open_flag & RT_DEVICE_OFLAG_OPEN) == 0)
    {
        if (rt_device_open(usart6_port_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    rt_device_set_rx_indicate(usart6_port_dev, usart6_rx_ind);

    if (usart6_rx_thread == RT_NULL)
    {
        usart6_rx_thread = rt_thread_create("u6rx",
                                            usart6_rx_entry,
                                            RT_NULL,
                                            USART6_RX_THREAD_STACK_SIZE,
                                            USART6_RX_THREAD_PRIORITY,
                                            USART6_RX_THREAD_TICK);
        if (usart6_rx_thread == RT_NULL)
        {
            return -RT_ERROR;
        }

        rt_thread_startup(usart6_rx_thread);
    }

    return RT_EOK;
}

void USART6DMA_Init(uint32_t __baud)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (usart6_port_open() != RT_EOK)
    {
        return;
    }

    config.baud_rate = __baud;
    rt_device_control(usart6_port_dev, RT_DEVICE_CTRL_CONFIG, &config);
}

void USART6DMA_SendData(uint8_t *data, uint16_t size)
{
    uint8_t *target;

    if ((data == RT_NULL) || (size == 0))
    {
        return;
    }

    if (usart6_port_open() != RT_EOK)
    {
        return;
    }

    target = (ActiveBuffer_U6 == 0) ? BufferA_U6 : BufferB_U6;
    if (size > (USART6_MAX_LEN / 2))
    {
        size = USART6_MAX_LEN / 2;
    }

    memcpy(target, data, size);
    rt_device_write(usart6_port_dev, 0, target, size);
    ActiveBuffer_U6 ^= 1;
}

#include "usart5.h"

#ifdef RT_USING_DEVICE
#include <rtdevice.h>
#endif

#define UART5_PORT_DEVICE_NAME "uart5"
#define UART5_RX_THREAD_STACK_SIZE 1024
#define UART5_RX_THREAD_PRIORITY 15
#define UART5_RX_THREAD_TICK 10

uint8_t UART5DMA_RX_BUF[UART5_MAX_LEN] = {0};
uint8_t UART5_rx_len;

static uint8_t BufferA_U5[UART5_MAX_LEN / 2];
static uint8_t BufferB_U5[UART5_MAX_LEN / 2];
static volatile uint8_t ActiveBuffer_U5 = 0;

static rt_device_t uart5_port_dev = RT_NULL;
static struct rt_semaphore uart5_rx_sem;
static rt_thread_t uart5_rx_thread = RT_NULL;
static rt_bool_t uart5_rx_sem_inited = RT_FALSE;

static void uart5_shift_left(uint8_t *buffer, uint16_t *length, uint16_t count)
{
    if (count >= *length)
    {
        *length = 0;
        return;
    }

    memmove(buffer, buffer + count, *length - count);
    *length -= count;
}

/* UART5: PlantSoft */
static void uart5_try_parse(uint8_t *buffer, uint16_t *length)
{
    uint16_t i = 0;

    while (i < *length)
    {
        if (buffer[i] != PlantSoftpack_head_1)
        {
            i++;
            continue;
        }

        if ((i + 8 < *length) &&
            (buffer[i + 1] == PlantSoft_CMD_Time) &&
            (buffer[i + 8] == PlantSoftpack_tail))
        {
            PlantSoft_DataProc(&buffer[i], 9);
            uart5_shift_left(buffer, length, i + 9);
            i = 0;
            continue;
        }

        if ((i + 3 < *length) && (buffer[i + 3] == PlantSoftpack_tail))
        {
            PlantSoft_DataProc(&buffer[i], 4);
            uart5_shift_left(buffer, length, i + 4);
            i = 0;
            continue;
        }

        if (i > 0)
        {
            uart5_shift_left(buffer, length, i);
        }
        return;
    }

    if (*length > 1)
    {
        buffer[0] = buffer[*length - 1];
        *length = 1;
    }
}

static rt_err_t uart5_rx_ind(rt_device_t dev, rt_size_t size)
{
    (void)dev;
    (void)size;

    if (uart5_rx_sem_inited)
    {
        rt_sem_release(&uart5_rx_sem);
    }

    return RT_EOK;
}

static void uart5_rx_entry(void *parameter)
{
    uint8_t parse_buf[UART5_MAX_LEN] = {0};
    uint16_t parse_len = 0;
    rt_size_t size;

    (void)parameter;

    while (1)
    {
        if (rt_sem_take(&uart5_rx_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        do
        {
            size = rt_device_read(uart5_port_dev, 0, UART5DMA_RX_BUF, UART5_MAX_LEN);
            if (size > 0)
            {
                if ((parse_len + size) > UART5_MAX_LEN)
                {
                    parse_len = 0;
                }

                memcpy(parse_buf + parse_len, UART5DMA_RX_BUF, size);
                parse_len += (uint16_t)size;
                UART5_rx_len = (uint8_t)size;
                uart5_try_parse(parse_buf, &parse_len);
            }
        } while (size > 0);
    }
}

static rt_err_t uart5_port_open(void)
{
    if (uart5_port_dev == RT_NULL)
    {
        uart5_port_dev = rt_device_find(UART5_PORT_DEVICE_NAME);
        if (uart5_port_dev == RT_NULL)
        {
            return -RT_ERROR;
        }
    }

    if (!uart5_rx_sem_inited)
    {
        if (rt_sem_init(&uart5_rx_sem, "u5rx", 0, RT_IPC_FLAG_FIFO) != RT_EOK)
        {
            return -RT_ERROR;
        }
        uart5_rx_sem_inited = RT_TRUE;
    }

    if ((uart5_port_dev->open_flag & RT_DEVICE_OFLAG_OPEN) == 0)
    {
        if (rt_device_open(uart5_port_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    rt_device_set_rx_indicate(uart5_port_dev, uart5_rx_ind);

    if (uart5_rx_thread == RT_NULL)
    {
        uart5_rx_thread = rt_thread_create("u5rx",
                                           uart5_rx_entry,
                                           RT_NULL,
                                           UART5_RX_THREAD_STACK_SIZE,
                                           UART5_RX_THREAD_PRIORITY,
                                           UART5_RX_THREAD_TICK);
        if (uart5_rx_thread == RT_NULL)
        {
            return -RT_ERROR;
        }

        rt_thread_startup(uart5_rx_thread);
    }

    return RT_EOK;
}

void UART5DMA_Init(uint32_t __baud)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (uart5_port_open() != RT_EOK)
    {
        return;
    }

    config.baud_rate = __baud;
    rt_device_control(uart5_port_dev, RT_DEVICE_CTRL_CONFIG, &config);
}

void UART5DMA_SendData(uint8_t *data, uint16_t size)
{
    uint8_t *target;

    if ((data == RT_NULL) || (size == 0))
    {
        return;
    }

    if (uart5_port_open() != RT_EOK)
    {
        return;
    }

    target = (ActiveBuffer_U5 == 0) ? BufferA_U5 : BufferB_U5;
    if (size > (UART5_MAX_LEN / 2))
    {
        size = UART5_MAX_LEN / 2;
    }

    memcpy(target, data, size);
    rt_device_write(uart5_port_dev, 0, target, size);
    ActiveBuffer_U5 ^= 1;
}

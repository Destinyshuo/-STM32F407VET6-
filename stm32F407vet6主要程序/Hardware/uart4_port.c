#include "usart4.h"

#ifdef RT_USING_DEVICE
#include <rtdevice.h>
#endif

#define UART4_PORT_DEVICE_NAME "uart4"
#define UART4_RX_THREAD_STACK_SIZE 1024
#define UART4_RX_THREAD_PRIORITY 15
#define UART4_RX_THREAD_TICK 10

uint8_t UART4DMA_RX_BUF[UART4_MAX_LEN] = {0};
uint8_t UART4_rx_len;

static uint8_t BufferA_U4[UART4_MAX_LEN / 2];
static uint8_t BufferB_U4[UART4_MAX_LEN / 2];
static volatile uint8_t ActiveBuffer_U4 = 0;

static rt_device_t uart4_port_dev = RT_NULL;
static struct rt_semaphore uart4_rx_sem;
static rt_thread_t uart4_rx_thread = RT_NULL;
static rt_bool_t uart4_rx_sem_inited = RT_FALSE;

static void uart4_shift_left(uint8_t *buffer, uint16_t *length, uint16_t count)
{
    if (count >= *length)
    {
        *length = 0;
        return;
    }

    memmove(buffer, buffer + count, *length - count);
    *length -= count;
}

/* UART4: MaixCam */
static void uart4_try_parse(uint8_t *buffer, uint16_t *length)
{
    uint16_t i = 0;

    while (i < *length)
    {
        if (buffer[i] != UART4_FRAME_HEAD1)
        {
            i++;
            continue;
        }

        if ((i + 6 < *length) &&
            (buffer[i + 1] == UART4_FRAME_HEAD2_LOC) &&
            (buffer[i + 6] == UART4_FRAME_TAIL1))
        {
            Maixcam_DataProc(&buffer[i], 7);
            uart4_shift_left(buffer, length, i + 7);
            i = 0;
            continue;
        }

        if ((i + 3 < *length) &&
            ((buffer[i + 1] == UART4_FRAME_HEAD2_STA) ||
             (buffer[i + 1] == UART4_FRAME_HEAD2_TYP)) &&
            (buffer[i + 3] == UART4_FRAME_TAIL1))
        {
            Maixcam_DataProc(&buffer[i], 4);
            uart4_shift_left(buffer, length, i + 4);
            i = 0;
            continue;
        }

        if ((i + 2 < *length) &&
            (buffer[i + 1] == UART4_FRAME_HEAD2_LED) &&
            (buffer[i + 2] == UART4_FRAME_TAIL1))
        {
            Maixcam_DataProc(&buffer[i], 3);
            uart4_shift_left(buffer, length, i + 3);
            i = 0;
            continue;
        }

        if (i > 0)
        {
            uart4_shift_left(buffer, length, i);
        }
        return;
    }

    if (*length > 1)
    {
        buffer[0] = buffer[*length - 1];
        *length = 1;
    }
}

static rt_err_t uart4_rx_ind(rt_device_t dev, rt_size_t size)
{
    (void)dev;
    (void)size;

    if (uart4_rx_sem_inited)
    {
        rt_sem_release(&uart4_rx_sem);
    }

    return RT_EOK;
}

static void uart4_rx_entry(void *parameter)
{
    uint8_t parse_buf[UART4_MAX_LEN] = {0};
    uint16_t parse_len = 0;
    rt_size_t size;

    (void)parameter;

    while (1)
    {
        if (rt_sem_take(&uart4_rx_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        do
        {
            size = rt_device_read(uart4_port_dev, 0, UART4DMA_RX_BUF, UART4_MAX_LEN);
            if (size > 0)
            {
                if ((parse_len + size) > UART4_MAX_LEN)
                {
                    parse_len = 0;
                }

                memcpy(parse_buf + parse_len, UART4DMA_RX_BUF, size);
                parse_len += (uint16_t)size;
                UART4_rx_len = (uint8_t)size;
                uart4_try_parse(parse_buf, &parse_len);
            }
        } while (size > 0);
    }
}

static rt_err_t uart4_port_open(void)
{
    if (uart4_port_dev == RT_NULL)
    {
        uart4_port_dev = rt_device_find(UART4_PORT_DEVICE_NAME);
        if (uart4_port_dev == RT_NULL)
        {
            return -RT_ERROR;
        }
    }

    if (!uart4_rx_sem_inited)
    {
        if (rt_sem_init(&uart4_rx_sem, "u4rx", 0, RT_IPC_FLAG_FIFO) != RT_EOK)
        {
            return -RT_ERROR;
        }
        uart4_rx_sem_inited = RT_TRUE;
    }

    if ((uart4_port_dev->open_flag & RT_DEVICE_OFLAG_OPEN) == 0)
    {
        if (rt_device_open(uart4_port_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
        {
            return -RT_ERROR;
        }
    }

    rt_device_set_rx_indicate(uart4_port_dev, uart4_rx_ind);

    if (uart4_rx_thread == RT_NULL)
    {
        uart4_rx_thread = rt_thread_create("u4rx",
                                           uart4_rx_entry,
                                           RT_NULL,
                                           UART4_RX_THREAD_STACK_SIZE,
                                           UART4_RX_THREAD_PRIORITY,
                                           UART4_RX_THREAD_TICK);
        if (uart4_rx_thread == RT_NULL)
        {
            return -RT_ERROR;
        }

        rt_thread_startup(uart4_rx_thread);
    }

    return RT_EOK;
}

void UART4DMA_Init(uint32_t __baud)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (uart4_port_open() != RT_EOK)
    {
        return;
    }

    config.baud_rate = __baud;
    rt_device_control(uart4_port_dev, RT_DEVICE_CTRL_CONFIG, &config);
}

void UART4DMA_SendData(uint8_t *data, uint16_t size)
{
    uint8_t *target;

    if ((data == RT_NULL) || (size == 0))
    {
        return;
    }

    if (uart4_port_open() != RT_EOK)
    {
        return;
    }

    target = (ActiveBuffer_U4 == 0) ? BufferA_U4 : BufferB_U4;
    if (size > (UART4_MAX_LEN / 2))
    {
        size = UART4_MAX_LEN / 2;
    }

    memcpy(target, data, size);
    rt_device_write(uart4_port_dev, 0, target, size);
    ActiveBuffer_U4 ^= 1;
}

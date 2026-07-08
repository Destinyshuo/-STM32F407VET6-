/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-31     RT-Thread    first version
 */

#include <rtthread.h>

#include "My.h"

/* OLED 显示字符串拼接 */
static void main_append_char(char **buffer, char ch)
{
    **buffer = ch;
    (*buffer)++;
    **buffer = '\0';
}

static void main_append_string(char **buffer, const char *str)
{
    while (*str != '\0')
    {
        main_append_char(buffer, *str);
        str++;
    }
}

static void main_append_unsigned(char **buffer, unsigned long value)
{
    char temp[16];
    int index = 0;

    if (value == 0)
    {
        main_append_char(buffer, '0');
        return;
    }

    while (value > 0)
    {
        temp[index++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (index > 0)
    {
        main_append_char(buffer, temp[--index]);
    }
}

static void main_append_fixed(char **buffer, float value, uint8_t decimals)
{
    unsigned long scale = 1;
    unsigned long integer;
    unsigned long fraction;
    uint8_t i;

    for (i = 0; i < decimals; i++)
    {
        scale *= 10;
    }

    if (value < 0.0f)
    {
        main_append_char(buffer, '-');
        value = -value;
    }

    value += 0.5f / (float)scale;
    integer = (unsigned long)value;
    fraction = (unsigned long)((value - (float)integer) * (float)scale);

    main_append_unsigned(buffer, integer);

    if (decimals > 0)
    {
        main_append_char(buffer, '.');
        for (i = decimals; i > 0; i--)
        {
            unsigned long div = 1;
            uint8_t j;

            for (j = 1; j < i; j++)
            {
                div *= 10;
            }

            main_append_char(buffer, (char)('0' + (fraction / div) % 10));
        }
    }
}

static void main_pad_line(char *line, uint8_t width)
{
    uint8_t len = 0;

    while ((line[len] != '\0') && (len < width))
    {
        len++;
    }

    while (len < width)
    {
        line[len++] = ' ';
    }
    line[len] = '\0';
}

static void main_show_oled_data(void)
{
    char line[20];
    char *ptr;

    ptr = line;
    if (OLED_MQ2ppm >= 999.0f)
    {
        main_append_string(&ptr, "MQ2:>999");
    }
    else
    {
        main_append_string(&ptr, "MQ2:");
        main_append_fixed(&ptr, OLED_MQ2ppm, 2);
    }
    main_pad_line(line, 13);
    OLED_ShowStr(0, 0, line, 2);

    ptr = line;
    if (OLED_MQ135ppm >= 999.0f)
    {
        main_append_string(&ptr, "MQ135:>999");
    }
    else
    {
        main_append_string(&ptr, "MQ135:");
        main_append_fixed(&ptr, OLED_MQ135ppm, 2);
    }
    main_pad_line(line, 13);
    OLED_ShowStr(0, 2, line, 2);

    ptr = line;
    main_append_string(&ptr, "Soil:");
    main_append_fixed(&ptr, OLED_SoilPercent, 1);
    main_append_char(&ptr, '%');
    main_pad_line(line, 13);
    OLED_ShowStr(0, 4, line, 2);

    ptr = line;
    main_append_string(&ptr, "T:");
    main_append_fixed(&ptr, My_temp, 1);
    main_append_string(&ptr, " H:");
    main_append_fixed(&ptr, My_humi, 1);
    main_append_char(&ptr, '%');
    main_pad_line(line, 13);
    OLED_ShowStr(0, 6, line, 2);
}

int main(void)
{
    Soil_Init();
    MQ2_Init();
    MQ135_Init();
    DHT11_Init();
    Buzzer_Init();
    LED_PWM_Init();
    water_Init();
    PWM_Init();
    Encoder_Init();
    PID_Init();
    OLED_Init();
    OLED_Clear();
    rt_schedule_init();

    while (1)
    {
        main_show_oled_data();
        rt_thread_mdelay(500);
    }

    return RT_EOK;
}

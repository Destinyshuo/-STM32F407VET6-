#include "VL53_DMA.h"

uint16_t Distance;

extern uint8_t mystate;

/* 调试字符串拼接 */
static void VL53_AppendChar(char **buffer, char ch)
{
    **buffer = ch;
    (*buffer)++;
    **buffer = '\0';
}

static void VL53_AppendString(char **buffer, const char *str)
{
    while (*str != '\0')
    {
        **buffer = *str;
        (*buffer)++;
        str++;
    }
    **buffer = '\0';
}

static void VL53_AppendUnsigned(char **buffer, unsigned long value)
{
    char temp[16];
    int index = 0;

    if (value == 0)
    {
        VL53_AppendChar(buffer, '0');
        return;
    }

    while (value > 0)
    {
        temp[index++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (index > 0)
    {
        VL53_AppendChar(buffer, temp[--index]);
    }
}

static void VL53_AppendSigned(char **buffer, long value)
{
    if (value < 0)
    {
        VL53_AppendChar(buffer, '-');
        value = -value;
    }

    VL53_AppendUnsigned(buffer, (unsigned long)value);
}

static void VL53_AppendFloat(char **buffer, float value, uint8_t decimals)
{
    unsigned long scale = 1;
    long int_part;
    unsigned long frac_part;
    uint8_t i;

    for (i = 0; i < decimals; i++)
    {
        scale *= 10;
    }

    if (value < 0.0f)
    {
        VL53_AppendChar(buffer, '-');
        value = -value;
    }

    int_part = (long)value;
    frac_part = (unsigned long)((value - (float)int_part) * (float)scale + 0.5f);
    if (frac_part >= scale)
    {
        int_part++;
        frac_part -= scale;
    }

    VL53_AppendSigned(buffer, int_part);

    if (decimals == 0)
    {
        return;
    }

    VL53_AppendChar(buffer, '.');
    for (i = 0; i < decimals; i++)
    {
        unsigned long div = scale / 10;
        VL53_AppendChar(buffer, (char)('0' + (frac_part / div) % 10));
        scale = div;
    }
}

void VL53_DataProc(uint8_t *VL53_data, uint16_t Size)
{
    static uint8_t receive_qian = 0;
    static uint8_t receive_bai = 0;
    static uint8_t receive_shi = 0;
    static uint8_t receive_ge = 0;
    int i;

    /* VL53 距离帧解析 */
    for (i = 0; i < (int)Size - 1; i++)
    {
        if ((i + VL53pack_num - 1 < (int)Size) &&
            VL53_data[i] == VL53pack_head_1 &&
            VL53_data[i + 1] == VL53pack_head_2 &&
            VL53_data[i + VL53pack_num - 1] == VL53pack_tail)
        {
            if (VL53_data[i + 2] == 0x20) receive_qian = 0;
            else receive_qian = VL53_data[i + 2] - 0x30;

            if (VL53_data[i + 3] == 0x20) receive_bai = 0;
            else receive_bai = VL53_data[i + 3] - 0x30;

            if (VL53_data[i + 4] == 0x20) receive_shi = 0;
            else receive_shi = VL53_data[i + 4] - 0x30;

            if (VL53_data[i + 5] == 0x20) receive_ge = 0;
            else receive_ge = VL53_data[i + 5] - 0x30;
        }
    }

    Distance = receive_qian * 1000 + receive_bai * 100 + receive_shi * 10 + receive_ge;
}

void My_GetActual(uint16_t acutal_name)
{
    static char Debug_String[128];
    char *ptr = Debug_String;

    /* USART1 调试输出 */
    Debug_String[0] = '\0';

    switch (acutal_name)
    {
    case MY_DAY:
        VL53_AppendString(&ptr, "Date--20");
        VL53_AppendUnsigned(&ptr, Year);
        VL53_AppendChar(&ptr, '-');
        VL53_AppendUnsigned(&ptr, Month);
        VL53_AppendChar(&ptr, '-');
        VL53_AppendUnsigned(&ptr, Day);
        VL53_AppendString(&ptr, "\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_TIME:
        VL53_AppendString(&ptr, "Time--20");
        VL53_AppendUnsigned(&ptr, Year);
        VL53_AppendChar(&ptr, '-');
        VL53_AppendUnsigned(&ptr, Month);
        VL53_AppendChar(&ptr, '-');
        VL53_AppendUnsigned(&ptr, Day);
        VL53_AppendString(&ptr, "--");
        VL53_AppendUnsigned(&ptr, Hour);
        VL53_AppendChar(&ptr, ':');
        VL53_AppendUnsigned(&ptr, Minute);
        VL53_AppendChar(&ptr, ':');
        VL53_AppendUnsigned(&ptr, Second);
        VL53_AppendString(&ptr, " type:");
        VL53_AppendUnsigned(&ptr, Soil_PlantType);
        VL53_AppendString(&ptr, " state:");
        VL53_AppendUnsigned(&ptr, MaixMode_flag);
        VL53_AppendString(&ptr, " Motor:");
        VL53_AppendUnsigned(&ptr, Motor_flag);
        VL53_AppendString(&ptr, " LED:");
        VL53_AppendUnsigned(&ptr, lightONOFF_flag);
        VL53_AppendString(&ptr, " x:");
        VL53_AppendUnsigned(&ptr, Source_X);
        VL53_AppendString(&ptr, " y:");
        VL53_AppendUnsigned(&ptr, Source_Y);
        VL53_AppendString(&ptr, " D:");
        VL53_AppendUnsigned(&ptr, Distance);
        VL53_AppendString(&ptr, "mm\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_YAW:
        VL53_AppendString(&ptr, "Yaw:");
        VL53_AppendFloat(&ptr, Yaw, 2);
        VL53_AppendString(&ptr, ", Target:");
        VL53_AppendFloat(&ptr, Pid_Trun.Target, 2);
        VL53_AppendString(&ptr, ", err:");
        VL53_AppendFloat(&ptr, fabsf(Yaw - Pid_Trun.Target), 2);
        VL53_AppendString(&ptr, ", obsstate:");
        VL53_AppendUnsigned(&ptr, mystate);
        VL53_AppendString(&ptr, ", Distance:");
        VL53_AppendUnsigned(&ptr, Distance);
        VL53_AppendString(&ptr, ", Mil:");
        VL53_AppendFloat(&ptr, Milleage1, 1);
        VL53_AppendString(&ptr, "\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_ROLL:
        VL53_AppendString(&ptr, "Roll:");
        VL53_AppendFloat(&ptr, Roll, 2);
        VL53_AppendString(&ptr, "\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_PITCH:
        VL53_AppendString(&ptr, "Pitch:");
        VL53_AppendFloat(&ptr, Pitch, 2);
        VL53_AppendString(&ptr, "\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_MOTOR1:
        VL53_AppendString(&ptr, "Motor1:");
        VL53_AppendSigned(&ptr, Encoder_Speed1);
        VL53_AppendString(&ptr, ", ");
        VL53_AppendFloat(&ptr, Pid_Motor1Speed.Target, 2);
        VL53_AppendString(&ptr, "\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_MOTOR2:
        VL53_AppendString(&ptr, "Motor2:");
        VL53_AppendSigned(&ptr, Encoder_Speed2);
        VL53_AppendString(&ptr, ", ");
        VL53_AppendFloat(&ptr, Pid_Motor2Speed.Target, 2);
        VL53_AppendString(&ptr, "\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_ANGLE:
        VL53_AppendString(&ptr, "Angle:");
        VL53_AppendFloat(&ptr, Yaw, 2);
        VL53_AppendString(&ptr, ", ");
        VL53_AppendFloat(&ptr, Roll, 2);
        VL53_AppendString(&ptr, ", ");
        VL53_AppendFloat(&ptr, Pitch, 2);
        VL53_AppendString(&ptr, "\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_MOTOR:
        VL53_AppendString(&ptr, "Motor:");
        VL53_AppendSigned(&ptr, Encoder_Speed1);
        VL53_AppendString(&ptr, ", ");
        VL53_AppendSigned(&ptr, Encoder_Speed2);
        VL53_AppendString(&ptr, "\r\n");
        UART4DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case MY_Distance:
        VL53_AppendString(&ptr, "D:");
        VL53_AppendUnsigned(&ptr, Distance);
        VL53_AppendString(&ptr, "mm\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    case 6888:
        VL53_AppendString(&ptr, "T:");
        VL53_AppendFloat(&ptr, My_temp, 6);
        VL53_AppendString(&ptr, " H:");
        VL53_AppendFloat(&ptr, My_humi, 6);
        VL53_AppendString(&ptr, "\r\n");
        Usart1DMA_SendData((uint8_t *)Debug_String, strlen(Debug_String));
        break;

    default:
        break;
    }
}

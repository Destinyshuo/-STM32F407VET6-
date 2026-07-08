#include "Usart2DMA_IMU.h"

float Roll, Pitch, Yaw, Last_Yaw, Yaw_flag = 0;
uint8_t Year, Month, Day, Hour, Minute, Second;
uint8_t high[] = {180, 2, 3, 90, 60, 55, 20, 101, 100, 102};
uint8_t TimeCalibration_pack1[5] = {0xFF, 0xAA, 0x69, 0x88, 0xB5};
uint8_t TimeCalibration_pack2[5] = {0xFF, 0xAA, 0x30, 0x1A, 0x02};
uint8_t TimeCalibration_pack3[5] = {0xFF, 0xAA, 0x31, 0x02, 0x11};
uint8_t TimeCalibration_pack4[5] = {0xFF, 0xAA, 0x32, 0x20, 0x2B};
uint8_t TimeCalibration_pack5[5] = {0xFF, 0xAA, 0x33, 0xDC, 0x03};
uint8_t TimeCalibration_pack6[5] = {0xFF, 0xAA, 0x00, 0x00, 0x00};

/* IMU 时间校准包轮询发送 */
void Time_Calibration(void)
{
    static uint8_t cnt = 0;

    cnt++;

    if (cnt == 1) Usart2DMA_SendData(TimeCalibration_pack1, sizeof(TimeCalibration_pack1));
    if (cnt == 2) Usart2DMA_SendData(TimeCalibration_pack2, sizeof(TimeCalibration_pack2));
    if (cnt == 3) Usart2DMA_SendData(TimeCalibration_pack3, sizeof(TimeCalibration_pack3));
    if (cnt == 4) Usart2DMA_SendData(TimeCalibration_pack4, sizeof(TimeCalibration_pack4));
    if (cnt == 5) Usart2DMA_SendData(TimeCalibration_pack5, sizeof(TimeCalibration_pack5));
    if (cnt == 6) Usart2DMA_SendData(TimeCalibration_pack6, sizeof(TimeCalibration_pack6));
    cnt %= 6;
}

void HWT101_DataProc(uint8_t *RxBuffer, uint16_t Size)
{
    int i;

    /* HWT101 姿态帧解析 */
    for (i = 0; i < (int)Size - 1; i++)
    {
        if ((i + 7 < (int)Size) && RxBuffer[i] == 0x55 && RxBuffer[i + 1] == 0x53)
        {
            Roll = ((int16_t)((int16_t)RxBuffer[i + 3] << 8 | (int16_t)RxBuffer[i + 2])) / 32768.0f * 180.0f;
            Pitch = ((int16_t)((int16_t)RxBuffer[i + 5] << 8 | (int16_t)RxBuffer[i + 4])) / 32768.0f * 180.0f;
            Yaw = ((int16_t)((int16_t)RxBuffer[i + 7] << 8 | (int16_t)RxBuffer[i + 6])) / 32768.0f * 180.0f;

            Obspack[3] = RxBuffer[i + 6];
            Obspack[2] = RxBuffer[i + 7];
        }
    }
}

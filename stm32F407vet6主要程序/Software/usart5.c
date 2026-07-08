#include "usart5.h"

uint8_t Soilpack[]       = {0xAA, 0x10, 0x10, 0x00, 0x00, 0x55};
uint8_t Temppack[]       = {0xAA, 0x10, 0x11, 0x00, 0x00, 0x55};
uint8_t Humipack[]       = {0xAA, 0x10, 0x12, 0x00, 0x00, 0x55};
uint8_t Airpack[]        = {0xAA, 0x10, 0x13, 0x00, 0x00, 0x55};
uint8_t Obspack[]        = {0xAA, 0x20, 0x00, 0x00, 0x00, 0x00, 0x55};
uint8_t Plantstatepack[] = {0xAA, 0x30, 0x00, 0x55};
uint8_t PlantTypepack[]  = {0xAA, 0x70, 0x00, 0x55};

float My_temp, My_humi;
float OLED_MQ2ppm = 0.0f;
float OLED_MQ135ppm = 0.0f;
float OLED_SoilPercent = 0.0f;

uint8_t Motor_flag = 1;
uint8_t MaixMode_flag = 2;
uint8_t Water_flag = 1;

extern uint16_t Distance;
extern uint8_t blade_state;
extern uint8_t Soil_PlantType;
extern uint8_t Plantleaf_insflag;
extern uint8_t Modestate_light;
extern uint16_t starttime_cnt;
extern uint8_t lightONOFF_flag;
extern uint8_t Year, Month, Day, Hour, Minute, Second;

/* 上位机数据轮询上传 */
void SendData_to_PlantSoft(void)
{
    static uint16_t percentSoil;
    static uint16_t airppm;
    static uint8_t cnt = 0;

    percentSoil = (uint16_t)(OLED_SoilPercent * 10);
    Soilpack[3] = (percentSoil >> 8) & 0xff;
    Soilpack[4] = percentSoil & 0xff;

    Temppack[3] = (((uint16_t)My_temp * 10) >> 8) & 0xff;
    Temppack[4] = ((uint16_t)My_temp * 10) & 0xff;
    Humipack[3] = (((uint16_t)My_humi * 10) >> 8) & 0xff;
    Humipack[4] = ((uint16_t)My_humi * 10) & 0xff;

    airppm = (uint16_t)(OLED_MQ135ppm);
    Airpack[3] = (airppm >> 8) & 0xff;
    Airpack[4] = airppm & 0xff;

    Obspack[4] = (Distance >> 8) & 0xff;
    Obspack[5] = Distance & 0xff;

    Plantstatepack[2] = blade_state;
    PlantTypepack[2] = Soil_PlantType;

    cnt++;
    if (cnt == 1 * 4) UART5DMA_SendData(Soilpack, sizeof(Soilpack));
    if (cnt == 2 * 4) UART5DMA_SendData(Temppack, sizeof(Temppack));
    if (cnt == 3 * 4) UART5DMA_SendData(Humipack, sizeof(Humipack));
    if (cnt == 4 * 4) UART5DMA_SendData(Airpack, sizeof(Airpack));
    if (cnt == 5 * 4) UART5DMA_SendData(Obspack, sizeof(Obspack));
    if (cnt == 6 * 4) UART5DMA_SendData(Plantstatepack, sizeof(Plantstatepack));
    if (cnt == 7 * 4) UART5DMA_SendData(PlantTypepack, sizeof(PlantTypepack));
    cnt %= 28;
}

void PlantSoft_DataProc(uint8_t *PlantSoft_data, uint16_t Size)
{
    int i;

    /* 上位机控制命令解析 */
    for (i = 0; i < (int)Size - 1; i++)
    {
        if (PlantSoft_data[i] == PlantSoftpack_head_1 && PlantSoft_data[i + 3] == PlantSoftpack_tail)
        {
            if (PlantSoft_data[i + 1] == PlantSoft_CMD_MaixMode)
            {
                if (PlantSoft_data[i + 2] == 0x01)
                {
                    MaixMode_flag = 1;
                    Plantleaf_insflag = 1;
                    starttime_cnt = 0;
                }
                if (PlantSoft_data[i + 2] == 0x02)
                {
                    if (Modestate_light == 5)
                    {
                        Modestate_light = 1;
                    }
                    MaixMode_flag = 2;
                    starttime_cnt = 0;
                }
            }
            if (PlantSoft_data[i + 1] == PlantSoft_CMD_WaterCtrl)
            {
                if (PlantSoft_data[i + 2] == 0x00)
                {
                    Water_flag = 0;
                }
                if (PlantSoft_data[i + 2] == 0x01)
                {
                    Water_flag = 1;
                }
            }
            if (PlantSoft_data[i + 1] == PlantSoft_CMD_LightCtrl)
            {
                if (PlantSoft_data[i + 2] == 0x00)
                {
                    lightONOFF_flag = 0;
                }
                if (PlantSoft_data[i + 2] == 0x01)
                {
                    lightONOFF_flag = 1;
                }
            }
            if (PlantSoft_data[i + 1] == PlantSoft_CMD_SystemCtr)
            {
                if (PlantSoft_data[i + 2] == 0x01)
                {
                    Motor_flag = 0;
                }
                if (PlantSoft_data[i + 2] == 0x00)
                {
                    Motor_flag = 1;
                }
            }
            if (PlantSoft_data[i + 1] == PlantSoft_CMD_PlantType)
            {
                if (PlantSoft_data[i + 2] == 0x01)
                {
                    Soil_PlantType = UART4_MODE_SHADE;
                }
                if (PlantSoft_data[i + 2] == 0x02)
                {
                    Soil_PlantType = UART4_MODE_HALF_SHADE;
                }
                if (PlantSoft_data[i + 2] == 0x03)
                {
                    Soil_PlantType = UART4_MODE_SUN;
                }
            }
        }

        if (PlantSoft_data[i] == PlantSoftpack_head_1 && PlantSoft_data[i + 1] == PlantSoft_CMD_Time &&
            PlantSoft_data[i + 8] == PlantSoftpack_tail)
        {
            Year = PlantSoft_data[i + 2];
            Month = PlantSoft_data[i + 3];
            Day = PlantSoft_data[i + 4];
            Hour = PlantSoft_data[i + 5];
            Minute = PlantSoft_data[i + 6];
            Second = PlantSoft_data[i + 7];
        }
    }
}

#include "usart4.h"

uint16_t Source_X, Source_Y;
uint8_t blade_state = 0x03;
uint8_t Soil_PlantType = UART4_MODE_SHADE;
uint8_t MaixCam_Mode[2][12] =
{
    {0xa4, 0xb4, 0x01, 0xc4, 0xa4, 0xb4, 0x01, 0xc4, 0xa4, 0xb4, 0x01, 0xc4},
    {0xa4, 0xb4, 0x02, 0xc4, 0xa4, 0xb4, 0x02, 0xc4, 0xa4, 0xb4, 0x02, 0xc4}
};

extern uint8_t finding_flag;
extern uint8_t lightONOFF_flag;

/* MaixCam 模式下发 */
void Maixcam_SetMode(uint8_t mode)
{
    static uint16_t cnt = 0;

    cnt++;

    if (cnt >= 2)
    {
        if (mode == 1)
        {
            UART4DMA_SendData(MaixCam_Mode[mode - 1], sizeof(MaixCam_Mode[0]));
        }
        if (mode == 2)
        {
            UART4DMA_SendData(MaixCam_Mode[mode - 1], sizeof(MaixCam_Mode[0]));
        }

        cnt = 0;
    }
}

void Maixcam_DataProc(uint8_t *UART4_data, uint16_t Size)
{
    static uint8_t Source_head_X, Source_tail_X, Source_head_Y, Source_tail_Y;
    int i;

    /* MaixCam 坐标和状态解析 */
    for (i = 0; i < (int)Size - 1; i++)
    {
        if (UART4_data[i] == UART4_FRAME_HEAD1 &&
            UART4_data[i + 1] == UART4_FRAME_HEAD2_LOC &&
            UART4_data[i + 6] == UART4_FRAME_TAIL1)
        {
            Source_head_X = UART4_data[i + 2];
            Source_tail_X = UART4_data[i + 3];
            Source_head_Y = UART4_data[i + 4];
            Source_tail_Y = UART4_data[i + 5];

            Source_X = (uint16_t)(Source_head_X << 8) | Source_tail_X;
            Source_Y = (uint16_t)(Source_head_Y << 8) | Source_tail_Y;

            finding_flag = 1;
            lightONOFF_flag = 0;
        }

        if (UART4_data[i] == UART4_FRAME_HEAD1 &&
            UART4_data[i + 1] == UART4_FRAME_HEAD2_STA &&
            UART4_data[i + 3] == UART4_FRAME_TAIL1)
        {
            blade_state = UART4_data[i + 2];
        }

        if (UART4_data[i] == UART4_FRAME_HEAD1 &&
            UART4_data[i + 1] == UART4_FRAME_HEAD2_TYP &&
            UART4_data[i + 3] == UART4_FRAME_TAIL1)
        {
            if (UART4_data[i + 2] == 0x04) Soil_PlantType = UART4_MODE_SHADE;
            if (UART4_data[i + 2] == 0x05) Soil_PlantType = UART4_MODE_HALF_SHADE;
            if (UART4_data[i + 2] == 0x06)
            {
                Soil_PlantType = UART4_MODE_SUN;
            }
        }

        if (UART4_data[i] == UART4_FRAME_HEAD1 &&
            UART4_data[i + 1] == UART4_FRAME_HEAD2_LED &&
            UART4_data[i + 2] == UART4_FRAME_TAIL1)
        {
            lightONOFF_flag = 1;
            finding_flag = 0;
        }
    }
}

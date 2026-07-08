#include "usart6.h"

extern uint16_t Distance;
extern float OLED_MQ135ppm;
extern uint8_t Soil_AlarmFlag;
extern float My_humi;

/* 语音播报命令 */
void ASR_VOICE(uint8_t voice_choose)
{
    static uint8_t voice_pack[] = {0xa1, 0xb1, 0x00, 0xc1};

    voice_pack[2] = voice_choose;
    USART6DMA_SendData(voice_pack, sizeof(voice_pack));
}

void Check_vioce(void)
{
    static uint8_t obsvoice_flag = 1;
    static uint16_t obsvoice_waittime = 0;
    static uint8_t airvoice_flag = 1;
    static uint8_t soilvoice_flag = 1;
    static uint8_t humivoice_flag = 1;

    /* 报警语音防重复触发 */
    if (obsvoice_waittime > 0)
    {
        obsvoice_waittime--;
    }

    if (1)
    {
        if (Distance < 400 && Distance > 0 && obsvoice_flag != 0)
        {
            if (obsvoice_waittime == 0)
            {
                ASR_VOICE(OBS_VOICE);
                obsvoice_waittime = 800;
                obsvoice_flag--;
            }
        }
        else if (OLED_MQ135ppm >= 9.0f && airvoice_flag != 0)
        {
            ASR_VOICE(AIR_VOICE);
            airvoice_flag--;
        }
        else if (Soil_AlarmFlag == 1 && soilvoice_flag != 0)
        {
            ASR_VOICE(SOIL_VOICE);
            soilvoice_flag--;
        }
        else if (My_humi >= 70 && humivoice_flag != 0)
        {
            ASR_VOICE(HUMI_VOICE);
            humivoice_flag--;
        }
    }

    if (Distance >= 400)
    {
        obsvoice_flag = 1;
    }
    if (OLED_MQ135ppm < 7.4f)
    {
        airvoice_flag = 1;
    }
    if (Soil_AlarmFlag == 0)
    {
        soilvoice_flag = 1;
    }
    if (My_humi < 65)
    {
        humivoice_flag = 1;
    }
}

void USART6_DataProc(uint8_t *USART6_data, uint16_t Size)
{
    (void)USART6_data;
    (void)Size;
}

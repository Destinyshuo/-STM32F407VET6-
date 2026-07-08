#include "rt_schedule.h"

#include <rtthread.h>

static rt_thread_t schedule_thread = RT_NULL;

/*
 * 10ms 周期任务
 * 调用顺序对应原 TIM7 中断主链
 */
static void rt_schedule_entry(void *parameter)
{
    static uint16_t cnt = 0;

    (void)parameter;

    while (1)
    {
        cnt++;

        /* 10ms 主任务链 */
        GetSpeed_Milleage_1(&Encoder_Speed1, &Milleage1, &Milleage1_flag);
        GetSpeed_Milleage_2(&Encoder_Speed2, &Milleage2, &Milleage2_flag);
        SendData_to_PlantSoft();
        My_GetActual(MY_YAW);
        Maixcam_SetMode(MaixMode_flag);
        LightContral(lightONOFF_flag);
        water_auto(Soil_AlarmFlag, Water_flag);
        Check_vioce();
        MaixCam_Sport();
        DHT11_Task10ms();

        /* 采样值和显示变量刷新 */
        OLED_SoilPercent = Soil_GetData_Percent();
        Soil_UpdateAlarmFlag();
        OLED_MQ2ppm = MQ2_GetData_PPM();
        MQ2_UpdateAlarmFlag();
        OLED_MQ135ppm = MQ135_GetData_PPM();
        MQ135_UpdateAlarmFlag();
        My_temp = DHT11_GetTemperature();
        My_humi = DHT11_GetHumidity();
        DH11_UpdateAlarmFlag();

        if (cnt >= 300)
        {
            cnt = 0;
        }

        rt_thread_mdelay(10);
    }
}

int rt_schedule_init(void)
{
    if (schedule_thread != RT_NULL)
    {
        return RT_EOK;
    }

    schedule_thread = rt_thread_create("sched10ms",
                                       rt_schedule_entry,
                                       RT_NULL,
                                       1024,
                                       18,
                                       10);
    if (schedule_thread == RT_NULL)
    {
        return -RT_ERROR;
    }

    rt_thread_startup(schedule_thread);

    return RT_EOK;
}

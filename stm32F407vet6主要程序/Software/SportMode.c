#include "SportMode.h"

uint8_t Plantleaf_insflag = 1;
uint8_t Modestate_light = 1;
uint8_t finding_flag = 0;
uint16_t starttime_cnt = 0;
uint8_t mystate = 0;

/* 叶片检查动作 */
void Plantleaf_ins(void)
{
    static uint16_t time_cnt = 0;

    if (Plantleaf_insflag == 1)
    {
        time_cnt++;
        if (time_cnt == 1)
        {
            BuJinMotor_PitchView(2, 1, 100, 30);
        }
        else if (time_cnt == 200)
        {
            BuJinMotor_Circle(1, 1, 100, 1);
        }
        else if (time_cnt == 900)
        {
            BuJinMotor_Circle(1, 0, 100, 1);
            Plantleaf_insflag = 0;
        }
    }
    else
    {
        time_cnt = 0;
    }
}

void Finding_light(void)
{
    /* 找光流程 */
    if (Motor_flag == 1)
    {
        switch (Modestate_light)
        {
        case 1:
            Milleage1_flag = 0;
            Milleage2_flag = 0;
            Modestate_light = 2;
            break;

        case 2:
            if (finding_flag == 0)
            {
                if (Milleage1 <= 575)
                {
                    BuJinMotor_PitchView(2, 0, 100, 0);
                    Motor_PidTarget_Acce(300, -300);
                }
                else if (Milleage1 <= 1150)
                {
                    BuJinMotor_PitchView(2, 0, 100, 35);
                    Motor_PidTarget_Acce(300, -300);
                }
                else
                {
                    Modestate_light = 4;
                }
            }

            if (finding_flag == 1)
            {
                Modestate_light = 3;
            }
            break;

        case 3:
            Modestate_light = 4;
            break;

        case 4:
            BuJinMotor_PitchView(2, 0, 100, 0);
            Motor_SetCCR(0, 0);
            Modestate_light = 5;
            break;

        case 5:
            break;

        default:
            break;
        }
    }
    if (Motor_flag == 0)
    {
        Motor_SetCCR(0, 0);
    }
}

void Finding_obs(void)
{
    static uint8_t Modestate = 0;
    static uint16_t time_cntobs = 0;
    static float yaw_temprary, yaw_temprary1, yaw_temprary2;

    /* 避障流程 */
    if (Motor_flag == 1)
    {
        switch (Modestate)
        {
        case 0:
            if (Distance > 400)
            {
                time_cntobs = 0;
            }
            if (Distance <= 400)
            {
                time_cntobs++;
                if (time_cntobs >= 50)
                {
                    yaw_temprary = Yaw + 180;
                    Modestate = 1;
                    mystate = 1;
                }
            }
            break;

        case 1:
            Yaw_change(yaw_temprary);
            if (fabsf(Yaw - Pid_Trun.Target) < 5.0f) { Modestate = 2; mystate = 2; }
            time_cntobs = 0;
            break;

        case 2:
            Motor_SetCCR(0, 0);
            time_cntobs++;
            if (time_cntobs >= 100)
            {
                if (Distance > 400) { Modestate = 11; mystate = 11; }
                else { yaw_temprary1 = yaw_temprary + 60; Modestate = 3; mystate = 3; }
            }
            break;

        case 3:
            Yaw_change(yaw_temprary1);
            if (fabsf(Yaw - Pid_Trun.Target) < 5.0f) { Modestate = 4; mystate = 4; }
            time_cntobs = 0;
            break;

        case 4:
            Motor_SetCCR(0, 0);
            time_cntobs++;
            if (time_cntobs >= 100)
            {
                if (Distance > 400) { Modestate = 11; mystate = 11; }
                else { yaw_temprary2 = yaw_temprary - 60; Modestate = 5; mystate = 5; }
            }
            break;

        case 5:
            Yaw_change(yaw_temprary2);
            if (fabsf(Yaw - Pid_Trun.Target) < 5.0f) { Modestate = 6; mystate = 6; }
            time_cntobs = 0;
            break;

        case 6:
            Motor_SetCCR(0, 0);
            time_cntobs++;
            if (time_cntobs >= 100)
            {
                if (Distance > 400) { Modestate = 11; mystate = 11; }
                else { yaw_temprary1 = yaw_temprary + 90; Modestate = 7; mystate = 7; }
            }
            break;

        case 7:
            Yaw_change(yaw_temprary1);
            if (fabsf(Yaw - Pid_Trun.Target) < 5.0f) { Modestate = 8; mystate = 8; }
            time_cntobs = 0;
            break;

        case 8:
            Motor_SetCCR(0, 0);
            time_cntobs++;
            if (time_cntobs >= 100)
            {
                if (Distance > 400) { Modestate = 11; mystate = 11; }
                else { yaw_temprary2 = yaw_temprary - 90; Modestate = 9; mystate = 9; }
            }
            break;

        case 9:
            Yaw_change(yaw_temprary2);
            if (fabsf(Yaw - Pid_Trun.Target) < 5.0f) { Modestate = 10; mystate = 10; }
            time_cntobs = 0;
            break;

        case 10:
            Motor_SetCCR(0, 0);
            time_cntobs++;
            if (time_cntobs >= 100)
            {
                if (Distance > 400) { Modestate = 11; mystate = 11; }
                else { yaw_temprary2 = yaw_temprary - 90; Modestate = 0; mystate = 0; }
            }
            break;

        case 11:
            Milleage1 = 0;
            Milleage2 = 0;
            yaw_temprary = Yaw;
            Modestate = 12;
            mystate = 12;
            break;

        case 12:
            Milleage = (Milleage1 + Milleage2) / 2;
            if (Milleage <= 350)
            {
                Car_Goline(yaw_temprary, 600);
            }
            else
            {
                Motor_SetCCR(0, 0);
                time_cntobs = 0;
                Modestate = 0;
                mystate = 0;
            }
            break;

        default:
            break;
        }
    }
    if (Motor_flag == 0)
    {
        Motor_SetCCR(0, 0);
    }
}

void MaixCam_Sport(void)
{
    /* MaixCam 运动模式入口 */
    if (MaixMode_flag == 1)
    {
        starttime_cnt++;
        if (starttime_cnt >= 300)
        {
            Plantleaf_ins();
        }

        Finding_obs();
    }

    if (MaixMode_flag == 2)
    {
        starttime_cnt++;
        if (starttime_cnt >= 300)
        {
            Finding_light();
        }

        if (Modestate_light == 5)
        {
            Finding_obs();
        }
    }
}

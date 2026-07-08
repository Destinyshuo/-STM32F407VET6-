#include "CarTurn.h"

float Turn_Out, Yaw_Read;
float Turn_EndTargetLeft, Turn_EndTargetRight;

/* 按目标航向直行修正 */
void Car_Goline(float Target_Yaw, int16_t Turn_Speed)
{
    Pid_Trun.Target = Target_Yaw;
    Yaw_Read = Yaw;

    if (Yaw_Read < Target_Yaw - 180)
    {
        Yaw_Read = Yaw_Read + 360;
    }
    else if (Yaw_Read > Target_Yaw + 180)
    {
        Yaw_Read = Yaw_Read - 360;
    }

    Turn_Out = PID_pOut(&Pid_Trun, Yaw_Read);

    Turn_EndTargetLeft = Turn_Speed - Turn_Out;
    Turn_EndTargetRight = Turn_Speed + Turn_Out;

    Motor_PidTarget(Turn_EndTargetLeft, Turn_EndTargetRight);
}

void Car_Turn(float Target_Yaw)
{
    int16_t Turn_Speed = 0;

    /* 原地转向到目标航向 */
    Pid_Trun.Target = Target_Yaw;
    Yaw_Read = Yaw;

    if (Yaw_Read < Target_Yaw - 180)
    {
        Yaw_Read = Yaw_Read + 360;
    }
    else if (Yaw_Read > Target_Yaw + 180)
    {
        Yaw_Read = Yaw_Read - 360;
    }

    Turn_Out = PID_pOut_Max(&Pid_Trun, Yaw_Read, 200);

    Turn_EndTargetLeft = Turn_Speed - Turn_Out;
    Turn_EndTargetRight = Turn_Speed + Turn_Out;

    Motor_PidTarget(Turn_EndTargetLeft, Turn_EndTargetRight);
}

void Yaw_change(float change_range)
{
    /* 航向差限制到 -180~180 */
    if (change_range > 180.0f)
    {
        change_range -= 360.0f;
    }
    if (!(change_range > -180.0f))
    {
        change_range += 360.0f;
    }

    Car_Turn(change_range);
}

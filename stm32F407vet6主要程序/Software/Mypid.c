#include "Mypid.h"

Mypid Pid_Motor1Speed;
Mypid Pid_Motor2Speed;
Mypid Pid_BuJinMotor1_x;
Mypid Pid_BuJinMotor2_y;
Mypid Pid_Trun;

/* 电机、步进和转向 PID 参数 */
void PID_Init(void)
{
    Pid_Motor1Speed.Target = 0.0f;
    Pid_Motor1Speed.Actual = 0.0f;
    Pid_Motor1Speed.Kp = 0.17f;
    Pid_Motor1Speed.Ki = 0.060f;
    Pid_Motor1Speed.Kd = 0.08f;
    Pid_Motor1Speed.Error0 = 0.0f;
    Pid_Motor1Speed.Error1 = 0.0f;
    Pid_Motor1Speed.Error_Sum = 0.0f;
    Pid_Motor1Speed.Out = 0.0f;

    Pid_Motor2Speed.Target = 0.0f;
    Pid_Motor2Speed.Actual = 0.0f;
    Pid_Motor2Speed.Kp = 0.20f;
    Pid_Motor2Speed.Ki = 0.042f;
    Pid_Motor2Speed.Kd = 0.012f;
    Pid_Motor2Speed.Error0 = 0.0f;
    Pid_Motor2Speed.Error1 = 0.0f;
    Pid_Motor2Speed.Error_Sum = 0.0f;
    Pid_Motor2Speed.Out = 0.0f;

    Pid_BuJinMotor1_x.Target = 320.0f;
    Pid_BuJinMotor1_x.Actual = 0.0f;
    Pid_BuJinMotor1_x.Kp = 0.0f;
    Pid_BuJinMotor1_x.Ki = 0.0f;
    Pid_BuJinMotor1_x.Kd = 0.0f;
    Pid_BuJinMotor1_x.Error0 = 0.0f;
    Pid_BuJinMotor1_x.Error1 = 0.0f;
    Pid_BuJinMotor1_x.Error_Sum = 0.0f;
    Pid_BuJinMotor1_x.Out = 0.0f;

    Pid_BuJinMotor2_y.Target = 240.0f;
    Pid_BuJinMotor2_y.Actual = 0.0f;
    Pid_BuJinMotor2_y.Kp = 0.0f;
    Pid_BuJinMotor2_y.Ki = 0.0f;
    Pid_BuJinMotor2_y.Kd = 0.0f;
    Pid_BuJinMotor2_y.Error0 = 0.0f;
    Pid_BuJinMotor2_y.Error1 = 0.0f;
    Pid_BuJinMotor2_y.Error_Sum = 0.0f;
    Pid_BuJinMotor2_y.Out = 0.0f;

    Pid_Trun.Target = 0.0f;
    Pid_Trun.Actual = 0.0f;
    Pid_Trun.Kp = 5.0f;
    Pid_Trun.Ki = 0.0f;
    Pid_Trun.Kd = 0.0f;
    Pid_Trun.Error0 = 0.0f;
    Pid_Trun.Error1 = 0.0f;
    Pid_Trun.Error_Sum = 0.0f;
    Pid_Trun.Out = 0.0f;
}

float PID_pOut(Mypid *PID, float actual)
{
    /* 位置式 PID */
    PID->Actual = actual;
    PID->Error0 = PID->Target - PID->Actual;
    PID->Error_Sum += PID->Error0;
    PID->Out = PID->Kp * PID->Error0 + PID->Ki * PID->Error_Sum + PID->Kd * (PID->Error0 - PID->Error1);
    PID->Error1 = PID->Error0;

    return PID->Out;
}

float PID_pOut_Max(Mypid *PID, float actual, float Max)
{
    PID->Actual = actual;
    PID->Error0 = PID->Target - PID->Actual;
    PID->Error_Sum += PID->Error0;

    if (PID->Out > Max) PID->Out = Max;
    if (PID->Out < -Max) PID->Out = -Max;

    PID->Out = PID->Kp * PID->Error0 + PID->Ki * PID->Error_Sum + PID->Kd * (PID->Error0 - PID->Error1);
    PID->Error1 = PID->Error0;

    return PID->Out;
}

float PID_pOut_bujinMax(Mypid *PID, float actual, float Max)
{
    (void)actual;

    PID->Error_Sum += PID->Error0;

    if (PID->Out > Max) PID->Out = Max;
    if (PID->Out < -Max) PID->Out = -Max;

    PID->Out = PID->Kp * PID->Error0 + PID->Ki * PID->Error_Sum + PID->Kd * (PID->Error0 - PID->Error1);
    PID->Error1 = PID->Error0;

    return PID->Out;
}

float PID_iOut(Mypid *PID, float actual)
{
    /* 增量式 PID */
    PID->Actual = actual;
    PID->Error0 = PID->Target - PID->Actual;
    PID->Out += PID->Kp * (PID->Error0 - PID->Error1) + PID->Ki * PID->Error0;
    PID->Error1 = PID->Error0;

    return PID->Out;
}

float PID_iOut_Max(Mypid *PID, float actual, float Max)
{
    PID->Actual = actual;
    PID->Error0 = PID->Target - PID->Actual;
    PID->Out += PID->Kp * (PID->Error0 - PID->Error1) + PID->Ki * PID->Error0;

    if (PID->Out > Max)
    {
        PID->Out = Max;
    }
    else if (PID->Out < -Max)
    {
        PID->Out = -Max;
    }

    PID->Error1 = PID->Error0;

    return PID->Out;
}

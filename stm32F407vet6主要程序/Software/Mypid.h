#ifndef __MYPID_H__
#define __MYPID_H__

#include "My.h"

typedef struct
{
    float Target;
    float Actual;
    float Error0;
    float Error1;
    float Error_Sum;
    float Kp, Ki, Kd, Out;
} Mypid;

extern Mypid Pid_Motor1Speed;
extern Mypid Pid_Motor2Speed;
extern Mypid Pid_BuJinMotor1_x;
extern Mypid Pid_BuJinMotor2_y;
extern Mypid Pid_Trun;

/* 位置式和增量式控制接口 */
void PID_Init(void);
float PID_pOut(Mypid *PID, float actual);
float PID_pOut_Max(Mypid *PID, float actual, float Max);
float PID_pOut_bujinMax(Mypid *PID, float actual, float Max);
float PID_iOut(Mypid *PID, float actual);
float PID_iOut_Max(Mypid *PID, float actual, float Max);

#endif

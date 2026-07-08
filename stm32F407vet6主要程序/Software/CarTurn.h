#ifndef __CARTURN_H__
#define __CARTURN_H__

#include "My.h"

/* 转向控制变量 */
extern float Turn_Out, Yaw_Read;
extern float Turn_EndTargetLeft, Turn_EndTargetRight;

/* 车体转向接口 */
void Car_Goline(float Target_Yaw, int16_t Turn_Speed);
void Car_Turn(float Target_Yaw);
void Yaw_change(float change_range);

#endif

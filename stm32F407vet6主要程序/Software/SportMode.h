#ifndef __SPORTMODE_H__
#define __SPORTMODE_H__

#include "My.h"

extern uint8_t finding_flag;
extern uint8_t Plantleaf_insflag;
extern uint8_t Modestate_light;
extern uint16_t starttime_cnt;
extern uint8_t mystate;

/* 运动状态入口 */
void Plantleaf_ins(void);
void Finding_light(void);
void Finding_obs(void);
void MaixCam_Sport(void);

#endif

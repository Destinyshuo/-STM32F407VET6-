#ifndef __MY_H
#define __MY_H
/* 工程总头文件 */
#include <stm32f4xx.h>

#include <stdio.h>
#include <board.h>
#include "stdint.h"
#include "string.h"
#include "stdbool.h"
#include "math.h"
#include "stdlib.h"

/* 业务模块头文件 */
#include "oled.h"
#include "oled_i2c.h"
#include "board.h"
#include "fifo.h"
#include "Emm_V5.h"
#include "Mypid.h"
#include "TB6612.h"
#include "MotorEncoder.h"
#include "CarTurn.h"
#include "SportMode.h"
#include "PWM.h"
#include "ADC1.h"
#include "MQ2.h"
#include "MQ135.h"
#include "DH11.h"
#include "buzzer.h"
#include "Usart2DMA_IMU.h"
#include "VL53_DMA.h"
#include "usart3_port.h"
#include "usart4.h"
#include "usart5.h"
#include "usart6.h"
#include "rt_schedule.h"
#include "water.h"
#include "Soil.h"

//#include "bsp_uart.h"

#if 0
#include "buzzer.h"
#include "MQ2.h"
#include "MQ135.h"
#include "ADC1.h"
#include "Soil.h"
#include "usart3.h"

#include "water.h"
#include "PWM.h"
#include "DH11.h"
#include "TB6612.h"
#include "Mypid.h"
#include "MotorEncoder.h"
#include "CarTurn.h"
#include "TIM67_CNT.h"
#include "SportMode.h"
#endif


#endif

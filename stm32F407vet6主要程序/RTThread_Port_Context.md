# RT-Thread Port Context

本文件用于在新对话中无缝继续当前 STM32F407VET6 从 `Keil5 + STM32 标准库裸机工程` 到 `RT-Thread Studio + CubeMX HAL + RT-Thread` 的移植任务。  
新对话开始时，必须先完整阅读本文件，再继续后续阶段，不要直接大规模改代码。

---

## 1. 当前任务总目标

将原工程完整移植到当前 RT-Thread Studio 工程中，要求不是“能编译就算完成”，而是：

- 保持原工程变量名不变
- 保持变量参数不变
- 保持函数接口语义不变
- 保持引脚映射不变
- 保持业务行为和时序关系不变
- 保持串口交互协议和变量赋值链路不丢失
- 每个阶段完成后都必须达到 `0 error / 0 warning`

核心原则：**迁移的是执行载体，不能擅自改原业务语义。**

---

## 2. 任务总阶段划分

建议将总移植任务按以下阶段推进：

### 阶段 1：构建链路打通与基础独立模块迁移

- GCC/RT-Thread 构建环境修通
- 先迁移低耦合模块
- 建立 `Software` / `Hardware` 目录结构

### 阶段 2：串口与通信链路迁移

- USART1 / USART2 / USART3 / UART4 / UART5 / USART6
- DMA / 接收状态机 / 变量赋值链 / 上位机交互链路

### 阶段 3：环境传感器与执行器迁移

- Soil / MQ2 / MQ135 / DHT11 / Buzzer / PWM / water

### 阶段 4：10ms 周期调度骨架迁移

- 将原 `TIM7_IRQHandler()` 的周期逻辑映射到 RT-Thread 线程节拍

### 阶段 5：编码器里程采集迁移

- `MotorEncoder.c/.h`
- `GetSpeed_Milleage_1()`
- `GetSpeed_Milleage_2()`

### 阶段 6：运动控制相关迁移

- 电机 PWM
- PID
- SportMode
- 运动状态联动

### 阶段 7：联调、清理、注释补全与最终验收

- 行为等价核对
- 删除空白工程示例残留
- 全工程 `0 error / 0 warning`

---

## 3. 当前任务所处阶段与完成度

- 当前总完成度估计：**约 98%**
- 当前所在阶段：**阶段 7C 静态验收已完成，软件侧移植已收口**
- 当前任务目标：**在保持 `0 error / 0 warning` 的前提下，进入实板联调 / 运行时行为验证**

重要说明：

- 用户此前要求执行顺序必须固定为：  
  `任务阶段性方案 -> 得到确认后执行 -> 报告当前总工程移植任务完成度 -> 用户确认无问题 -> 出具下一阶段任务方案`
- 因此即使本文件已经说明下一阶段是最终收尾，**新对话也不能直接开改，必须先给出该阶段方案并等待用户确认。**

---

## 4. 工程路径

### 原工程路径

原始业务工程主路径：

`C:\Users\ZJShuooo\Desktop\Work\WorkFile\隐藏or临时文件\智能植物助手 - 副本`

原工程压缩备份路径（如需回溯）：

`C:\Users\ZJShuooo\Desktop\Work\WorkFile\隐藏or临时文件\智能植物助手 (备份最终版).zip`

### 新工程路径

`D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31`

---

## 5. 用户硬约束与移植规范

以下内容必须长期保持，严禁在新对话中丢失：

### 5.1 变量相关硬约束

- **变量名不能改**
- **变量参数不能改**
- **函数参数和业务接口语义不能乱改**
- **变量赋值不能遗漏**

特别强调：

- 不能只盯标志位
- **除了标志位以外，所有变量赋值也必须追踪**
- 包括但不限于：
  - 初始化赋值
  - 中断中的赋值
  - `while(1)` 中的赋值
  - 定时调度中的赋值
  - 模块内部对全局变量的赋值
  - 清零
  - 累加/递减
  - 限幅
  - 状态切换
  - 条件分支下的更新

### 5.2 硬件与引脚约束

- **不能改引脚**
- **不能改外设映射**
- **不能为了方便改串口分配**
- **不能为了方便改传感器接线定义**

### 5.3 业务约束

- 不能把原裸机业务逻辑随意重写成“另一套 RT-Thread 逻辑”
- 只能做 HAL/RT-Thread 层面的等效适配
- 不允许借迁移名义擅自重构原协议、状态机、变量关系

### 5.4 工作方式约束

- 分阶段执行，不一次性大迁移
- 每阶段先出方案，等用户确认
- 每阶段完成后必须汇报：
  - 本阶段完成内容
  - 当前编译结果
  - 当前总工程完成度
- 用户确认“无问题”后，再进入下阶段方案

### 5.5 质量约束

- 每次代码阶段完成后必须编译
- 目标必须是：
  - `0 error`
  - `0 warning`
- 发现编译告警也要处理，不允许带 warning 进入下一阶段

### 5.6 注释约束

- 目前以迁移正确性为主
- 注释可后补
- 最终阶段再统一整理注释

---

## 6. 原工程理解

### 6.1 原工程不是简单 while 工程

原工程本质上是：

- 上电初始化
- `while(1)` 环境采样与显示
- `TIM7_IRQHandler()` 作为 10ms 周期软调度器
- 多路串口/DMA/中断共同驱动变量更新和业务行为

即：

**裸机轮询 + 定时节拍 + 串口中断事件驱动** 的组合结构。

### 6.2 原工程初始化大致职责

包含以下几类初始化：

- 传感器：`MQ2_Init()` / `MQ135_Init()` / `Soil_Init()` / `DHT11_Init()` / `ADCx_Init()`
- 执行器：`LED_PWM_Init()` / `Buzzer_Init()` / `water_Init()`
- 运动控制：`PWM_Init()` / `Encoder_Init()` / `PID_Init()`
- 串口通信：
  - `USART1DMA_Init(115200)`：VL53 / 调试
  - `Usart2DMA_Init(115200)`：IMU
  - `Usart3_Init(115200)`：步进电机 / `Emm_V5`
  - `UART4DMA_Init(115200)`：MaixCam
  - `UART5DMA_Init(115200)`：PlantSoft / 上位机
  - `USART6DMA_Init(115200)`：语音模块
- 显示：`OLED_Init()` / `OLED_Clear()`

### 6.3 原工程 `while(1)` 职责

主要负责：

- DHT11 温湿度读取
- MQ2 读取
- MQ135 读取
- Soil 读取
- 报警标志更新
- OLED 显示更新

### 6.4 原工程 `TIM7_IRQHandler()` 原顺序

已确认原定时节拍核心顺序为：

1. `GetSpeed_Milleage_1(&Encoder_Speed1, &Milleage1, &Milleage1_flag);`
2. `GetSpeed_Milleage_2(&Encoder_Speed2, &Milleage2, &Milleage2_flag);`
3. `SendData_to_PlantSoft();`
4. `My_GetActual(MY_YAW);`
5. `Maixcam_SetMode(MaixMode_flag);`
6. `LightContral(lightONOFF_flag);`
7. `water_auto(Soil_AlarmFlag, Water_flag);`
8. `Check_vioce();`
9. `MaixCam_Sport();`
10. `DHT11_Task10ms();`

后续迁移时必须特别注意执行顺序，不要随意改调度顺序。

### 6.5 串口业务理解方向

后续每次读原工程时，重点不是“有没有标志位”，而是：

- 哪个串口负责哪个业务模块
- 收包完成后改了哪些变量
- 哪些变量用于上位机回传
- 哪些变量用于运动控制联动
- 哪些变量虽然不是 flag，但仍然是业务核心变量

---

## 7. 当前 RT-Thread 工程现状

### 7.1 `applications/main.c` 当前状态

当前文件：  
`D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\applications\main.c`

已接入初始化：

- `Soil_Init();`
- `MQ2_Init();`
- `MQ135_Init();`
- `DHT11_Init();`
- `Buzzer_Init();`
- `LED_PWM_Init();`
- `water_Init();`
- `PWM_Init();`
- `Encoder_Init();`
- `PID_Init();`
- `OLED_Init();`
- `OLED_Clear();`
- `rt_schedule_init();`

当前主循环已不再保留空白工程 `Hello RT-Thread` 模板示例，而是改为：

- 周期刷新 OLED 显示
- 使用已由 10ms 调度更新的变量：
  - `OLED_MQ2ppm`
  - `OLED_MQ135ppm`
  - `OLED_SoilPercent`
  - `My_temp`
  - `My_humi`

说明：

- `applications/main.c` 已完成“模板 while 清理 + OLED 主循环显示等价迁移”
- 当前显示格式化未使用 `sprintf()`，用于避免 newlib heap 链接风险回归

### 7.2 当前 `rt_schedule.c` 状态

当前文件：  
`D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\rt_schedule.c`

已建立 RT-Thread 10ms 周期线程，当前已按原 `TIM7_IRQHandler()` 主链顺序校准：

1. `GetSpeed_Milleage_1(&Encoder_Speed1, &Milleage1, &Milleage1_flag);`
2. `GetSpeed_Milleage_2(&Encoder_Speed2, &Milleage2, &Milleage2_flag);`
3. `SendData_to_PlantSoft();`
4. `My_GetActual(MY_YAW);`
5. `Maixcam_SetMode(MaixMode_flag);`
6. `LightContral(lightONOFF_flag);`
7. `water_auto(Soil_AlarmFlag, Water_flag);`
8. `Check_vioce();`
9. `MaixCam_Sport();`
10. `DHT11_Task10ms();`

为保持原主循环/OLED/上位机变量链，10ms 线程中仍保留以下环境变量刷新：

- `OLED_SoilPercent = Soil_GetData_Percent();`
- `Soil_UpdateAlarmFlag();`
- `OLED_MQ2ppm = MQ2_GetData_PPM();`
- `MQ2_UpdateAlarmFlag();`
- `OLED_MQ135ppm = MQ135_GetData_PPM();`
- `MQ135_UpdateAlarmFlag();`
- `My_temp = DHT11_GetTemperature();`
- `My_humi = DHT11_GetHumidity();`
- `DH11_UpdateAlarmFlag();`

### 7.3 当前 `board.h` 串口映射

当前文件：  
`D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\drivers\board.h`

已启用串口如下：

- UART1：`PA9 / PA10`
- UART2：`PD5 / PD6`
- UART3：`PD8 / PD9`
- UART4：`PC10 / PC11`
- UART5：`PC12 / PD2`
- UART6：`PC6 / PC7`

---

## 8. 已完成迁移文件

### 8.1 已迁移/新增的 `Software` 文件

- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\fifo.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\fifo.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\Emm_V5.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\Emm_V5.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\oled.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\oled.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\codetab.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\oledfont.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\ADC1.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\ADC1.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\Soil.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\Soil.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\MQ2.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\MQ2.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\MQ135.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\MQ135.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\DH11.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\DH11.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\buzzer.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\buzzer.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\PWM.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\PWM.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\TB6612.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\TB6612.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\Mypid.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\Mypid.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\MotorEncoder.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\MotorEncoder.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\CarTurn.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\CarTurn.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\SportMode.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\SportMode.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\water.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\water.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\Usart2DMA_IMU.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\Usart2DMA_IMU.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\VL53_DMA.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\VL53_DMA.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\usart4.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\usart4.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\usart5.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\usart5.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\usart6.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\usart6.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\rt_schedule.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\rt_schedule.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\My.h`

### 8.2 已迁移/新增的 `Hardware` 文件

- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\oled_i2c.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\oled_i2c.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\usart1_port.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\usart2_port.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\usart3_port.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\usart3_port.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\uart4_port.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\uart5_port.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\usart6_port.c`

### 8.3 已删除的阶段性占位文件

- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\port_state_weak.c`

说明：

- 该文件曾用于阶段性弱定义占位
- 当前真实模块归属已基本落实
- 该文件已经从工程与构建中删除

---

## 9. 已修改的工程文件

以下文件已被修改以支撑当前阶段迁移：

- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\applications\main.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Software\SConscript`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\Hardware\SConscript`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\libraries\SConscript`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\SConstruct`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\rtconfig.py`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\drivers\stm32f4xx_hal_conf.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\drivers\drv_gpio.c`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\drivers\board.h`
- `D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31\drivers\drv_usart.c`

其中关键改动如下：

- `Software\SConscript`
  - 已加入当前迁移的软件模块
  - 已加入 `LIBS = ['m']`，用于 `powf()` 等数学函数链接
- `Hardware\SConscript`
  - 已加入各路串口适配层
- `libraries\SConscript`
  - 已加入 HAL 核心文件以及 `ADC/TIM/UART` 支持
- `SConstruct`
  - 已定义 `STM32F407xx`
  - 已定义 `SOC_SERIES_STM32F4`
  - 已定义 `USE_HAL_DRIVER`
- `rtconfig.py`
  - 已配置 `cortex-m4`
  - 已配置 `-mthumb`
  - 已配置 `-mfpu=fpv4-sp-d16`
  - 已配置 `-mfloat-abi=hard`
- `drivers\stm32f4xx_hal_conf.h`
  - 已启用 `HAL_ADC_MODULE_ENABLED`
  - 已启用 `HAL_TIM_MODULE_ENABLED`
  - 已启用 `HAL_UART_MODULE_ENABLED`
- `drivers\drv_gpio.c`
  - 已将 `GPIO_PIN_5` 的 EXTI 回调转发到 `DHT11_EXTI_Callback(GPIO_Pin)`
- `applications\main.c`
  - 已接入当前迁移完成的初始化函数
  - 已删除 RT-Thread 模板 Hello 循环
  - 已恢复 OLED 初始化与主循环显示刷新

---

## 10. 已完成的关键迁移动作与业务状态

### 10.1 OLED / FIFO / Emm_V5

- `oled`、`fifo`、`Emm_V5` 已完成基础适配
- `USART3` 已完成当前工程实际使用到的发送控制接口迁移：
  - `usart_SendCmd()`
  - `BuJinMotor_X_Speed()`
  - `BuJinMotor_Y_Speed()`
  - `BuJinMotor_Circle()`
  - `BuJinMotor_PitchView()`
  - `calculate_slope()`
- 经当前源码检索，原 `rxFrameFlag` / `rxCmd` / `rxCount` 接收帧状态变量未发现外部业务引用，因此本工程未保留该空转接收链路

### 10.2 UART5 上位机链路

`PlantSoft` / 上位机交互相关链路已迁入，核心变量已保留真实定义，不允许再次误删或弱化。  
重点变量包括但不限于：

- `My_temp`
- `My_humi`
- `OLED_MQ2ppm`
- `OLED_MQ135ppm`
- `OLED_SoilPercent`
- `Motor_flag`
- `MaixMode_flag`
- `Water_flag`

这些变量是**与软件上位机交互的核心变量**，后续只允许在“确认真实归属后进行等效迁移”，不允许因为“弱定义已移除”就漏掉它们的正式定义或赋值链。

### 10.3 UART4 MaixCam 链路

已保留关键变量：

- `Source_X`
- `Source_Y`
- `blade_state`
- `Soil_PlantType`
- `MaixCam_Mode`

### 10.4 USART2 IMU 链路

已保留关键变量：

- `Roll`
- `Pitch`
- `Yaw`
- `Last_Yaw`
- `Yaw_flag`
- `Year`
- `Month`
- `Day`
- `Hour`
- `Minute`
- `Second`
- `TimeCalibration_pack1` ~ `TimeCalibration_pack6`

### 10.5 USART1 / VL53 / 调试链路

已保留：

- `Distance`
- `My_GetActual()`

说明：

- `My_GetActual()` 已恢复
- 当前实现使用局部字符串拼装，避免 `sprintf` 触发 `_sbrk` / newlib heap 链接问题

### 10.6 USART6 语音链路

已保留：

- `ASR_VOICE()`
- `Check_vioce()`
- 语音命令相关常量

### 10.7 环境传感器与执行器

已完成并接入：

- Soil
- MQ2
- MQ135
- DHT11
- Buzzer
- LED PWM
- water

其中：

- `lightONOFF_flag` 已有真实定义
- `Soil_AlarmFlag` 已有真实定义
- `MQ2_AlarmFlag` 已有真实定义
- `MQ135_AlarmFlag` 已有真实定义
- `DHT11_AlarmFlag` 已有真实定义

### 10.8 Buzzer 调度方式

`Buzzer_warm(10)` 存在长延时，因此蜂鸣器已放到独立线程中，避免阻塞 10ms 周期调度线程。

### 10.9 编码器 / 电机 / PID / 运动状态机

已完成并接入：

- `MotorEncoder.c/.h`
- `TB6612.c/.h`
- `Mypid.c/.h`
- `CarTurn.c/.h`
- `SportMode.c/.h`

已落实真实定义的关键变量包括但不限于：

- `Encoder_Speed1`
- `Encoder_Speed2`
- `Milleage1`
- `Milleage2`
- `Milleage`
- `Milleage1_flag`
- `Milleage2_flag`
- `Pid_Motor1Speed`
- `Pid_Motor2Speed`
- `Pid_BuJinMotor1_x`
- `Pid_BuJinMotor2_y`
- `Pid_Trun`
- `Plantleaf_insflag`
- `finding_flag`
- `Modestate_light`
- `starttime_cnt`
- `mystate`

---

## 11. 最终验收后仍需关注的运行时事项

当前已不再依赖弱定义占位文件，`Software\port_state_weak.c` 已删除。

静态核对与源码编译已完成，但以下内容仍建议在实板上继续核对“运行时行为等价”：

- 10ms 调度主链在实板上的节拍稳定性是否与原工程足够接近
- 主循环 OLED 刷新显示在实板上的格式/节奏是否与原工程足够接近
- 各串口链路在真实外设接入后是否与原工程协议行为一致
- `mystate`、`Distance`、`Yaw`、`Milleage1` 等调试/联动变量的运行时行为是否与原工程一致
- `main.c` 与 `rt_schedule.c` 间的变量职责边界是否清晰且稳定

---

## 12. 当前构建状态

最近一次源码编译验证结论：

- 已执行：`scons.bat -c`
- 已执行：`scons.bat -j4`
- 结果：**`0 error / 0 warning`**
- 已生成：`rt-thread.elf`
- 复验日期：**2026-07-08**

说明：

- 该编译状态对应的是：
  - `MotorEncoder` 已迁移
  - `TB6612 + Mypid` 已迁移
  - `CarTurn + SportMode` 已迁移
  - `main.c` 已删除模板 while 并恢复 OLED 刷新
  - `rt_schedule.c` 已按原 `TIM7_IRQHandler()` 主链顺序校准
  - `RTThread_Port_Context.md` 已更新到静态验收完成状态

---

## 13. 构建命令与工具链

### 13.1 工具链路径

必须使用以下 ARM GCC 工具链：

`D:\Myself\ZjsApp\RT-ThreadStudio\repo\Extract\ToolChain_Support_Packages\ARM\GNU_Tools_for_ARM_Embedded_Processors\5.4.1\bin`

### 13.2 构建命令

在工程根目录  
`D:\Myself\RT-ThreadStudio_Project\Test_2026_5_31`  
执行：

```powershell
$tool='D:\Myself\ZjsApp\RT-ThreadStudio\repo\Extract\ToolChain_Support_Packages\ARM\GNU_Tools_for_ARM_Embedded_Processors\5.4.1\bin'
$env:RTT_EXEC_PATH=$tool
$env:PATH="$tool;D:\Myself\ZjsApp\RT-ThreadStudio\platform\env_released\env\tools\bin;D:\Myself\ZjsApp\RT-ThreadStudio\platform\env_released\env\tools\Python27;D:\Myself\ZjsApp\RT-ThreadStudio\platform\env_released\env\tools\Python27\Scripts;" + $env:PATH
scons.bat -j4
```

### 13.3 阶段完成后的标准复验命令

```powershell
$tool='D:\Myself\ZjsApp\RT-ThreadStudio\repo\Extract\ToolChain_Support_Packages\ARM\GNU_Tools_for_ARM_Embedded_Processors\5.4.1\bin'
$env:RTT_EXEC_PATH=$tool
$env:PATH="$tool;D:\Myself\ZjsApp\RT-ThreadStudio\platform\env_released\env\tools\bin;D:\Myself\ZjsApp\RT-ThreadStudio\platform\env_released\env\tools\Python27;D:\Myself\ZjsApp\RT-ThreadStudio\platform\env_released\env\tools\Python27\Scripts;" + $env:PATH
scons.bat -c
scons.bat -j4
```

### 13.4 不要切换到其他 GCC 路径

不要切到较新的 mingw ARM GCC 包，以免再次出现头文件兼容和链接问题。

### 13.5 `clean` 与 `build` 必须顺序执行

`scons.bat -c` 和 `scons.bat -j4` 必须顺序执行，不要并行触发。  
`-c` 会删除 `cconfig.h`，若与编译并发执行，会出现临时性缺头文件错误。

---

## 14. 当前关键引脚/外设信息

- OLED 软件 I2C：
  - `PB8`：SCL
  - `PB9`：SDA
- Buzzer：
  - `PB1`
- DHT11：
  - `PB5`
  - EXTI：`EXTI9_5`
- MQ2：
  - `PA0`：AO
  - `PA1`：DO
- MQ135：
  - `PA2`：AO
  - `PA3`：DO
- Soil：
  - `PA5`：AO
  - `PA6`：DO
- LED PWM：
  - `PE13 / TIM1_CH3`
- water：
  - `PD4`
- Encoder：
  - `TIM4 CH1/CH2 -> PD12 / PD13`
  - `TIM3 CH1/CH2 -> PA6 / PA7`
- USART2 IMU：
  - `PD5 / PD6`
- USART3 步进电机：
  - `PD8 / PD9`
- UART4 MaixCam：
  - `PC10 / PC11`
- UART5 PlantSoft：
  - `PC12 / PD2`
- USART6 Voice：
  - `PC6 / PC7`

---

## 15. 下一阶段计划

### 下一阶段名称

实板联调 / 运行时行为验证

### 下一阶段目标

在当前已达到 `0 error / 0 warning` 的基础上，结合真实传感器、串口外设和执行器，确认运行时行为与原工程一致。

### 下一阶段预计处理内容

- 核对 10ms 调度线程在实板上的周期稳定性
- 核对 OLED 显示刷新节奏、传感器数值刷新和报警行为
- 核对 UART2 / UART4 / UART5 / UART6 / USART1 / USART3 与真实外设交互结果
- 核对运动控制链 `Distance / Yaw / Milleage / mystate` 的联动结果
- 如实板发现偏差，仅做保持原语义的小范围修正

### 下一阶段明确不做的内容

除非用户明确要求，否则最终收尾阶段不要：

- 擅自改变量名
- 擅自改参数
- 擅自改引脚
- 擅自重写业务状态机
- 擅自为了“更优雅”而重构原业务逻辑

---

## 16. 严禁遗漏事项

下面这些提醒必须在后续每个阶段反复自检：

### 16.1 与上位机交互核心变量不能丢

尤其注意以下变量不能因“弱定义移除”或“模块拆分”而漏定义、漏赋值、漏发送：

- `My_temp`
- `My_humi`
- `OLED_MQ2ppm`
- `OLED_MQ135ppm`
- `OLED_SoilPercent`
- `Motor_flag`
- `MaixMode_flag`
- `Water_flag`

### 16.2 不能只跟踪 flag

必须追踪所有业务变量，不限于：

- 采样值
- 坐标值
- 姿态值
- 里程值
- 速度值
- 模式值
- 目标值
- 状态值

### 16.3 不能把“能编译”误判为“已迁移完成”

以下情况都只能算“阶段性打通”，不能算完全完成：

- 有弱定义占位
- 只有头文件，没有真实实现
- 只打通了发送，没有补齐接收和变量更新
- 只接了初始化，还没接入调度或业务链

### 16.4 `applications/main.c` 的模板 while 不能误认

再次强调：

- `Hello RT-Thread` 循环不是原工程逻辑
- 可以删除
- 但删除动作也应放在合适阶段处理，不要误认为当前业务依赖它

---

## 17. 新对话开始时必须先做什么

新对话开始后，必须先做以下动作，不要直接大规模继续迁移：

1. 先读取本文件 `RTThread_Port_Context.md`
2. 先复述并确认以下硬约束：
   - 变量名不能改
   - 变量参数不能改
   - 变量赋值不能遗漏
   - 除了标志位以外，所有变量赋值也必须追踪
3. 先核对当前工程实际状态：
   - `applications/main.c`
   - `Software/rt_schedule.c`
   - `Software/SConscript`
   - `drivers/board.h`
4. 再去读原工程下一阶段/最终复核相关文件，不要急着写代码：
   - `app/main.c`
   - `TIM7_IRQHandler()`
   - 相关串口链路和运动控制链路
5. 先输出“下一阶段方案”和“对原工程变量赋值链的理解”
6. **等待用户确认后再执行**

---

## 18. 可直接复制到新对话的启动提示词

```text
请先读取当前工程根目录的 RTThread_Port_Context.md，并严格按其中约束继续 STM32F407VET6 从 Keil5 标准库裸机工程到 RT-Thread Studio + CubeMX HAL + RT-Thread 的移植任务。
先不要直接大规模改代码。
请先：
1. 复述你理解到的硬约束，特别是变量名不能改、变量参数不能改、变量赋值不能遗漏，且除了标志位以外所有变量赋值也必须追踪；
2. 核对当前工程中的 applications/main.c、Software/rt_schedule.c、Software/SConscript、drivers/board.h；
3. 读取原工程下一阶段/最终复核相关文件，优先是 app/main.c、TIM7_IRQHandler()、相关串口链路和运动控制链路；
4. 先输出下一阶段方案、当前总工程完成度、以及你对原工程变量赋值链和调度顺序的理解；
5. 等我确认后再开始改代码。
另外，applications/main.c 里的 Hello RT-Thread while 循环已经删除；后续不要再把模板逻辑当成原业务逻辑。
```

---

## 19. 最终提醒

本文件的作用不是“概括一下做过什么”，而是确保新对话能直接接手继续干活，且不走偏。  
后续任何阶段如果出现下面任一种情况，都应先停下来回读本文件：

- 想改变量名
- 想改参数
- 想改引脚
- 想跳过某些赋值链
- 想把多个未读清楚的模块一次性重写
- 想把“模板代码”当作“原工程逻辑”

如果发生这些倾向，说明任务已经偏离用户要求。

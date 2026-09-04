/**
  ******************************************************************************
  * @file    motor_config.h
  * @brief   电机控制配置头文件(开环模式)
  * @author  可以航行
  * @version V2.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 电流环(FOC闭环)已清除,本工程仅保留开环电压模式(vMotorOpenLoopRun)。
  * 开环电压指令(vd/vq)、自动生成电频率等测试参数目前直接在 App/Src/user.c
  * 顶部(OL_VD_V / OL_VQ_V / OL_START_FREQ_HZ)配置,便于快速修改。
  ******************************************************************************
  */

#ifndef __MOTOR_CONFIG_H
#define __MOTOR_CONFIG_H

/* ==================== 电机本体参数 ==================== */
#define MOTOR_CONFIG_POLE_PAIRS		7		/* 极对数(须与 bsp_config.h 的 MOTOR_POLE_PAIRS 一致) */

#endif /* __MOTOR_CONFIG_H */

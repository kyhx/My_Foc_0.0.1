/**
  ******************************************************************************
  * @file    motor_config.h
  * @brief   电机控制(FOC电流环)整定参数配置头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 本文件集中放置电流环(内环)的采样频率、PI 参数与各类限幅,便于调试时
  * 统一修改。当前电流环代码位于 App/Src/app_motor.c,由 PWM 同步注入组
  * ADC 中断(HAL_ADCEx_InjectedConvCpltCallback)每周期调用一次执行。
  ******************************************************************************
  */

#ifndef __MOTOR_CONFIG_H
#define __MOTOR_CONFIG_H

/* ==================== 电机本体参数 ==================== */
#define MOTOR_CONFIG_POLE_PAIRS		7		/* 极对数(须与 bsp_config.h 的 MOTOR_POLE_PAIRS 一致) */

/* ==================== 电流环(内环)采样频率 ==================== */
/* TIM1 中心对齐 PWM: TIM1 时钟=170MHz, ARR=8400 → 开关频率≈170MHz/(2×8400)≈10.1kHz;
 * 注入组由 TRGO=UPDATE 在 PWM 峰/谷触发,每开关周期约触发 2 次,
 * 故电流环 ISR 频率≈20kHz,周期 50us。如与实际触发频率不符,请修正此值。 */
#define CUR_LOOP_FS_HZ				20000.0f	/* 电流环 ISR 频率(Hz) */
#define CUR_LOOP_TS					(1.0f/CUR_LOOP_FS_HZ)	/* 电流环控制周期(s) */

/* ==================== 电流环 PI(d/q 共用,需按实际电机调节) ==================== */
#define CUR_LOOP_KP					0.6f	/* 比例系数 */
#define CUR_LOOP_KI					30.0f	/* 积分系数 */
#define CUR_LOOP_KD					0.0f	/* 微分系数(电流环通常不用) */
#define CUR_LOOP_DER_FC				0.0f	/* 微分滤波截止频率(Hz),0=禁用 */

/* PI 输出/积分限幅(电压量纲)。最终仍会按实测母线电压自适应钳位 */
#define CUR_LOOP_PI_OUT_V			20.0f	/* PID 输出限幅(V) */
#define CUR_LOOP_PI_INT_V			20.0f	/* PID 积分限幅(V,抗积分饱和) */
/* 每周期可用电压上限 = 比例 × 实测母线电压(V),防过调制 */
#define CUR_LOOP_VOLT_RATIO			0.9f

/* ==================== 电流参考限幅(A) ==================== */
#define CUR_LOOP_IQ_REF_MAX			5.0f
#define CUR_LOOP_IQ_REF_MIN			(-5.0f)
#define CUR_LOOP_ID_REF_MAX			3.0f
#define CUR_LOOP_ID_REF_MIN			(-3.0f)

#endif /* __MOTOR_CONFIG_H */

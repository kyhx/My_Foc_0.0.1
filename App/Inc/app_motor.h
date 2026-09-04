/**
  ******************************************************************************
  * @file    app_motor.h
  * @brief   电机控制应用层(FOC电流环 + 开环)头文件
  * @author  可以航行
  * @version V2.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 电机控制应用层,支持两种模式(运行时切换,按键长按或串口 mode):
  *   - OpenLoop 开环电压控制: 由主循环 vMotorOpenLoopRun 驱动,按固定 vd/vq 与
  *                           当前电角度逆Park→SVPWM→三相PWM,用于对相/测试。
  *   - Current  FOC电流闭环:  由 PWM 同步注入组中断 vAppMotorIsr 驱动,每周期
  *                           采样三相电流 → Clarke/Park → id/iq PI → 逆Park →
  *                           SVPWM → 三相PWM。电流环需编码器来源的真实电角度。
  * 本文件实现 motor.h 中声明的运行控制/开环/启停接口(vMotorSetRun、vMotorOpenLoopRun 等)。
  ******************************************************************************
  */

#ifndef __APP_MOTOR_H
#define __APP_MOTOR_H

#include "top_config.h"
#include <stdint.h>

/**	@brief 			电机控制模式枚举
 * 	@note
 *
 * */
typedef enum
{
	emAppMotorMode_OpenLoop = 0,	//开环电压控制(vd/vq),主循环驱动
	emAppMotorMode_Current,			//FOC电流闭环(id/iq PI),PWM中断驱动
}
emAppMotorModeTdf;

/*	@brief 						电机控制应用初始化
 * 	@note							初始化 id/iq 两路电流环 PI、电流参考与开环电压、默认开环模式,
 * 								并向 BSP_DRV8313 注册 PWM 同步 ISR 钩子(vAppMotorIsr)。
 *
 * */
void vAppMotorInit(void);

/*	@brief 						设置电机控制模式(运行时切换)
 * 	@param		emMode			OpenLoop 开环 / Current 电流闭环
 * 	@note							切入 Current 时清空两路电流环积分,避免旧积分冲出。
 *
 * */
void vAppMotorSetMode(emAppMotorModeTdf emMode);

/*	@brief 						切换电机控制模式(开环 ↔ 电流环)
 * 	@note							按键长按/串口调用。
 *
 * */
void vAppMotorToggleMode(void);

/*	@brief 						获取当前控制模式
 * 	@retval						当前模式枚举
 *
 * */
emAppMotorModeTdf emAppMotorGetMode(void);

/*	@brief 						设置 d 轴电流参考(A,通常恒为0)
 * 	@param		fId				d 轴电流参考(A),自动限幅
 *
 * */
void vAppMotorSetIdRef(float fId);

/*	@brief 						设置 q 轴电流参考(A,决定电磁转矩)
 * 	@param		fIq			q 轴电流参考(A),自动限幅
 *
 * */
void vAppMotorSetIqRef(float fIq);

/*	@brief 						获取 d 轴电流参考
 * 	@retval					d 轴电流参考(A)
 *
 * */
float fAppMotorGetIdRef(void);

/*	@brief 						获取 q 轴电流参考
 * 	@retval					q 轴电流参考(A)
 *
 * */
float fAppMotorGetIqRef(void);

/*	@brief 						获取电流环反馈 d 轴电流(实测)
 * 	@retval					d 轴电流(A)
 *
 * */
float fAppMotorGetId(void);

/*	@brief 						获取电流环反馈 q 轴电流(实测)
 * 	@retval					q 轴电流(A)
 *
 * */
float fAppMotorGetIq(void);

/*	@brief 						获取电流环 d 轴电压输出
 * 	@retval					d 轴电压指令(V)
 *
 * */
float fAppMotorGetVd(void);

/*	@brief 						获取电流环 q 轴电压输出
 * 	@retval					q 轴电压指令(V)
 *
 * */
float fAppMotorGetVq(void);

/*	@brief 						PWM同步电流环执行一步(注入组中断内调用)
 * 	@note							仅在 运行 且 模式=Current 时计算并输出三相占空比;
 * 								否则立即返回(此时 PWM 由主循环开环驱动或关断)。
 *
 * */
void vAppMotorIsr(void);

#endif /* __APP_MOTOR_H */

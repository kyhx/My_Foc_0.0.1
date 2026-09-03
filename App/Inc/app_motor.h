/**
  ******************************************************************************
  * @file    app_motor.h
  * @brief   电机应用层(FOC电流环闭环)头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 电机控制应用层,同时支持:
  *   - OpenLoop 开环电压控制(由主循环 vMotorOpenLoopRun 驱动,用于对相/扫频);
  *   - Current  完整 FOC 电流闭环(由 PWM 同步注入组中断 vAppMotorIsr 驱动,
  *              每周期采样三相电流 → Clarke/Park → id/iq PI → 逆Park → SVPWM)。
  * 电流环依赖电角度模块已处于编码器来源(闭环须先完成转子对齐并捕获零位)。
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
 * 	@note							初始化 id/iq 两路电流环 PI、电流参考与开环电压,
 * 								并向 BSP_DRV8313 注册 PWM 同步 ISR 钩子(在注入组
 * 								转换完成回调内执行 vAppMotorIsr)。
 *
 * */
void vAppMotorInit(void);

/*	@brief 						设置电机控制模式(运行时切换)
 * 	@param		emMode			OpenLoop 开环 / Current 电流闭环
 * 	@note							切入 Current 时清空两路电流环积分状态,避免旧积分冲出。
 *
 * */
void vAppMotorSetMode(emAppMotorModeTdf emMode);

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
 * 								运行状态由 vMotorSetRun 控制(与开环共用同一启停标志)。
 *
 * */
void vAppMotorIsr(void);

#endif /* __APP_MOTOR_H */

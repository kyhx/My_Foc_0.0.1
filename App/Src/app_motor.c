/**
  ******************************************************************************
  * @file    app_motor.c
  * @brief   电机应用层(FOC电流环闭环)源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 电机控制应用层,支持两种模式(运行时切换):
  *   - OpenLoop 开环电压控制: 由主循环 vMotorOpenLoopRun 调用,按固定 vd/vq 与
  *                           当前电角度逆Park→SVPWM→三相PWM,用于对相/扫频/对齐。
  *   - Current  FOC电流闭环:  由 PWM 同步注入组中断 vAppMotorIsr 每开关周期调用,
  *                           采样三相电流 → Clarke → Park 得 id/iq,经两路 PI
  *                           (d 轴参考=0,q 轴参考=转矩给定)得 vd/vq → 逆Park →
  *                           SVPWM → 三相PWM。电流环需编码器来源的真实电角度。
  *
  * 本文件同时实现 motor.h 中声明的开环运行/启停接口(vMotorSetRun、
  * vMotorOpenLoopRun 等),使上电即沿用既有开环流程;闭环则通过
  * vAppMotorSetMode(emAppMotorMode_Current) 切入并给定 q 轴电流。
  ******************************************************************************
  */

#include "app_motor.h"
#include "motor_config.h"
#include "motor.h"
#include "foc.h"
#include "bsp_config.h"
#include "bsp_DRV8313.h"
#include "bsp_pwm.h"

/* ------------------ 模块运行状态(文件内静态) ------------------ */
static uint8_t          s_u8Run   = 0;			/* 运行标志(与开环共用) */
static emAppMotorModeTdf s_emMode = emAppMotorMode_OpenLoop;	/* 控制模式 */

/* 开环电压指令 */
static float            s_fOlVd   = 0.0f;
static float            s_fOlVq   = 0.0f;

/* 电流环状态 */
static T_Pid_t          s_IdPid;				/* d轴电流PI */
static T_Pid_t          s_IqPid;				/* q轴电流PI */
static float            s_fIdRef  = 0.0f;		/* d轴电流参考(A) */
static float            s_fIqRef  = 0.0f;		/* q轴电流参考(A) */
static float            s_fId     = 0.0f;		/* d轴电流反馈(实测) */
static float            s_fIq     = 0.0f;		/* q轴电流反馈(实测) */
static float            s_fVd     = 0.0f;		/* d轴电压输出 */
static float            s_fVq     = 0.0f;		/* q轴电压输出 */

/**
 * @brief 								电机控制应用初始化
 * @note									初始化电流环 PI、电流参考、开环电压与模式,
 * 									并向 BSP_DRV8313 注册 PWM 同步 ISR 钩子。
 *
 * */
void vAppMotorInit(void)
{
	PID_Init(&s_IdPid, CUR_LOOP_KP, CUR_LOOP_KI, CUR_LOOP_KD, CUR_LOOP_TS,
				-CUR_LOOP_PI_OUT_V, CUR_LOOP_PI_OUT_V,
				-CUR_LOOP_PI_INT_V, CUR_LOOP_PI_INT_V, CUR_LOOP_DER_FC);
	PID_Init(&s_IqPid, CUR_LOOP_KP, CUR_LOOP_KI, CUR_LOOP_KD, CUR_LOOP_TS,
				-CUR_LOOP_PI_OUT_V, CUR_LOOP_PI_OUT_V,
				-CUR_LOOP_PI_INT_V, CUR_LOOP_PI_INT_V, CUR_LOOP_DER_FC);

	s_u8Run  = 0;
	s_emMode = emAppMotorMode_OpenLoop;

	s_fOlVd  = 0.0f;
	s_fOlVq  = 0.0f;

	s_fIdRef = 0.0f;
	s_fIqRef = 0.0f;
	s_fId    = 0.0f;
	s_fIq    = 0.0f;
	s_fVd    = 0.0f;
	s_fVq    = 0.0f;

	/* 注册 PWM 同步 ISR 钩子: 注入组转换完成回调(读取电流后)调用 vAppMotorIsr */
	vDRV8313RegisterIsrCb(vAppMotorIsr);
}

/**
 * @brief 								设置电机控制模式
 * @param		emMode			OpenLoop 开环 / Current 电流闭环
 * @note									切入 Current 前清空电流环积分,防旧积分冲出。
 *
 * */
void vAppMotorSetMode(emAppMotorModeTdf emMode)
{
	if (emMode == emAppMotorMode_Current)
	{
		PID_Reset(&s_IdPid);
		PID_Reset(&s_IqPid);
		s_emMode = emAppMotorMode_Current;
	}
	else
	{
		s_emMode = emAppMotorMode_OpenLoop;
	}
}

/**
 * @brief 								获取当前控制模式
 * @retval								当前模式枚举
 *
 * */
emAppMotorModeTdf emAppMotorGetMode(void)
{
	return s_emMode;
}

/**
 * @brief 								设置 d 轴电流参考(A)
 * @param		fId				d 轴电流参考,自动限幅
 *
 * */
void vAppMotorSetIdRef(float fId)
{
	if (fId > CUR_LOOP_ID_REF_MAX)      fId = CUR_LOOP_ID_REF_MAX;
	else if (fId < CUR_LOOP_ID_REF_MIN) fId = CUR_LOOP_ID_REF_MIN;
	s_fIdRef = fId;
}

/**
 * @brief 								设置 q 轴电流参考(A,决定转矩)
 * @param		fIq			q 轴电流参考,自动限幅
 *
 * */
void vAppMotorSetIqRef(float fIq)
{
	if (fIq > CUR_LOOP_IQ_REF_MAX)      fIq = CUR_LOOP_IQ_REF_MAX;
	else if (fIq < CUR_LOOP_IQ_REF_MIN) fIq = CUR_LOOP_IQ_REF_MIN;
	s_fIqRef = fIq;
}

float fAppMotorGetIdRef(void) { return s_fIdRef; }
float fAppMotorGetIqRef(void) { return s_fIqRef; }
float fAppMotorGetId(void)    { return s_fId;    }
float fAppMotorGetIq(void)    { return s_fIq;    }
float fAppMotorGetVd(void)    { return s_fVd;    }
float fAppMotorGetVq(void)    { return s_fVq;    }

/* ==================== 开环/启停接口(motor.h 声明,在此实现) ==================== */
/**
 * @brief 								设置运行状态
 * @param		bRun			1=运行, 0=停止
 * @note									上升沿(停止→运行)时清空电流环积分,避免旧积分冲出。
 * 									停止后三相PWM由上层配合 vDRV8313Disable 关断。
 *
 * */
void vMotorSetRun(uint8_t bRun)
{
	if (bRun)
	{
		if (s_u8Run == 0)
		{
			PID_Reset(&s_IdPid);
			PID_Reset(&s_IqPid);
		}
		s_u8Run = 1;
	}
	else
	{
		s_u8Run = 0;
	}
}

/**
 * @brief 								获取运行状态
 * @retval								1=运行, 0=停止
 *
 * */
uint8_t u8MotorGetRun(void)
{
	return s_u8Run;
}

/**
 * @brief 								设置开环 d 轴电压指令
 * @param		fVd			d 轴电压(V)
 *
 * */
void vMotorOpenLoopSetVd(float fVd)
{
	s_fOlVd = fVd;
}

/**
 * @brief 								设置开环 q 轴电压指令
 * @param		fVq			q 轴电压(V)
 *
 * */
void vMotorOpenLoopSetVq(float fVq)
{
	s_fOlVq = fVq;
}

float fMotorGetOpenLoopVd(void) { return s_fOlVd; }
float fMotorGetOpenLoopVq(void) { return s_fOlVq; }

/**
 * @brief 								开环运行一步输出(主循环周期调用)
 * @param		fUdc			母线电压(V),供 SVPWM 归一化
 * @note									仅在 运行 且 模式=OpenLoop 时,按当前 vd/vq 与电角度
 * 									逆Park→SVPWM→三相PWM;电流闭环模式或停止时直接返回
 * 									(闭环 PWM 由 vAppMotorIsr 输出)。调用前需 vMotorAngleUpdate()。
 *
 * */
void vMotorOpenLoopRun(float fUdc)
{
	T_AlphaBeta_t 	stAb;
	float 			fDutyA, fDutyB, fDutyC;

	if (s_u8Run == 0)                          return;
	if (s_emMode != emAppMotorMode_OpenLoop)   return;	/* 闭环由 ISR 驱动 */

	{
		T_Dq_t stDq;
		stDq.fD = s_fOlVd;
		stDq.fQ = s_fOlVq;
		T_InvPark(&stDq, fMotorGetElecAngleRad(), &stAb);
	}
	SVPWM(&stAb, fUdc, &fDutyA, &fDutyB, &fDutyC);
	vPwmSetDutyAll(PWM, fDutyA, fDutyB, fDutyC);
}

/* ==================== FOC 电流环(ISR) ==================== */
/**
 * @brief 								PWM同步电流环执行一步(注入组中断内调用)
 * @note									流程: 读三相电流 → Clarke → Park(当前电角度)
 * 									→ d轴PI(参考0)/q轴PI(参考转矩给定) → 按母线电压
 * 									自适应钳位 → 逆Park → SVPWM → 写三相PWM。
 * 									仅在 运行 且 模式=Current 时执行,否则立即返回。
 * 									ISR 频率 ≈ PWM 开关频率,控制周期见 CUR_LOOP_TS。
 *
 * */
void vAppMotorIsr(void)
{
	T_Abc_t 		stAbc;
	T_AlphaBeta_t 	stAb;
	T_Dq_t 			stDq;
	float 			fUdc, fLim, fTheta;
	float 			fDutyA, fDutyB, fDutyC;

	if (s_u8Run == 0)                          return;
	if (s_emMode != emAppMotorMode_Current)    return;

	fUdc = fDRV8313GetBusVoltage(DRV8313);
	if (fUdc <= 0.0f)                          return;

	/** 1. 采样三相电流(已扣零偏) → Clarke → Park */
	stAbc.fA = fDRV8313GetCurrentA(DRV8313, 0);
	stAbc.fB = fDRV8313GetCurrentA(DRV8313, 1);
	stAbc.fC = fDRV8313GetCurrentA(DRV8313, 2);
	T_Clarke(&stAbc, &stAb);

	fTheta = fMotorGetElecAngleRad();
	T_Park(&stAb, fTheta, &stDq);
	s_fId = stDq.fD;
	s_fIq = stDq.fQ;

	/** 2. 电流环 PI: d 轴参考=0, q 轴参考=转矩给定 */
	stDq.fD = PID_Update(&s_IdPid, s_fIdRef, s_fId);
	stDq.fQ = PID_Update(&s_IqPid, s_fIqRef, s_fIq);

	/** 3. 按实测母线电压自适应钳位(防过调制积分饱和) */
	fLim = CUR_LOOP_VOLT_RATIO * fUdc;
	if (stDq.fD >  fLim)      stDq.fD =  fLim;
	else if (stDq.fD < -fLim) stDq.fD = -fLim;
	if (stDq.fQ >  fLim)      stDq.fQ =  fLim;
	else if (stDq.fQ < -fLim) stDq.fQ = -fLim;
	s_fVd = stDq.fD;
	s_fVq = stDq.fQ;

	/** 4. 逆Park → SVPWM → 三相PWM */
	T_InvPark(&stDq, fTheta, &stAb);
	SVPWM(&stAb, fUdc, &fDutyA, &fDutyB, &fDutyC);
	vPwmSetDutyAll(PWM, fDutyA, fDutyB, fDutyC);
}

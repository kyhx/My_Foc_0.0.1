/**
  ******************************************************************************
  * @file    foc.h
  * @brief   FOC 坐标变换中间层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 三相 abc ↔ αβ(Clarke) ↔ dq(Park) 坐标变换,幅度不变(等幅 2/3)。
  * 供 FOC 电流环使用: 采样三相电流 → Clarke → Park 得 id/iq;
  * 电压指令 dq → 逆Park → 逆Clarke 得三相电压指令(供SVPWM)。
  ******************************************************************************
  */

#ifndef __FOC_H
#define __FOC_H

#include "top_config.h"

/*	@brief 			三相 abc 坐标(电流/电压)
 * 	@note
 *
 * */
typedef struct
{
	float fA;
	float fB;
	float fC;
}
T_Abc_t;

/*	@brief 			αβ 静止两相坐标
 * 	@note
 *
 * */
typedef struct
{
	float fAlpha;
	float fBeta;
}
T_AlphaBeta_t;

/*	@brief 			dq 旋转两相坐标
 * 	@note
 *
 * */
typedef struct
{
	float fD;
	float fQ;
}
T_Dq_t;

/*	@brief 			αβ + 零序(带零序Clarke输出)
 * 	@note
 *
 * */
typedef struct
{
	float fAlpha;
	float fBeta;
	float f0;
}
T_Ab0_t;

/*	@brief 									Clarke变换: abc → αβ (等幅)
 * 	@param		pAbc						三相坐标输入
 * 	@param		pAb						αβ坐标输出
 *
 * */
void T_Clarke(const T_Abc_t *pAbc, T_AlphaBeta_t *pAb);

/*	@brief 									Clarke变换(带零序): abc → αβ0
 * 	@param		pAbc						三相坐标输入
 * 	@param		pAb0						αβ0坐标输出
 *
 * */
void T_Clarke2(const T_Abc_t *pAbc, T_Ab0_t *pAb0);

/*	@brief 									逆Clarke变换: αβ → abc
 * 	@param		pAb						αβ坐标输入
 * 	@param		pAbc						三相坐标输出
 *
 * */
void T_InvClarke(const T_AlphaBeta_t *pAb, T_Abc_t *pAbc);

/*	@brief 									Park变换: αβ → dq
 * 	@param		pAb						αβ坐标输入
 * 	@param		fTheta						电角度(rad)
 * 	@param		pDq						dq坐标输出
 *
 * */
void T_Park(const T_AlphaBeta_t *pAb, float fTheta, T_Dq_t *pDq);

/*	@brief 									逆Park变换: dq → αβ
 * 	@param		pDq						dq坐标输入
 * 	@param		fTheta						电角度(rad)
 * 	@param		pAb						αβ坐标输出
 *
 * */
void T_InvPark(const T_Dq_t *pDq, float fTheta, T_AlphaBeta_t *pAb);

/*	@brief 									SVPWM空间矢量调制(等效中心对齐法)
 * 	@param		pAb						αβ电压指令(逆Park输出,单位V)
 * 	@param		fUdc						母线电压(V)
 * 	@param		pfDutyA					输出A相占空比(0~1,供TIM比较值)
 * 	@param		pfDutyB					输出B相占空比(0~1)
 * 	@param		pfDutyC					输出C相占空比(0~1)
 * 	@note									内部先逆Clarke得三相相电压,再叠加零序分量(中点注入)实现中心对齐
 * 											SVPWM,线性调制区最大相电压峰值为 Udc/√3(较正弦PWM提高15.5%)。
 * 											输出已限幅到[0,1],过调制时自动钳位。
 *
 * */
void SVPWM(const T_AlphaBeta_t *pAb, float fUdc, float *pfDutyA, float *pfDutyB, float *pfDutyC);

/*	@brief 							PID控制器(位置式,带抗饱和与微分滤波)
 * 	@note									u = Kp·e + Ki·∫e·dt + Kd·(-dFbk/dt)
 * 											积分项限幅(fIntMin/fIntMax)防积分饱和;
 * 											微分项取反馈微分并一阶低通滤波,避免设定值突变微分冲击;
 * 											输出限幅(fOutMin/fOutMax)。
 *
 * */
typedef struct
{
	/* 配置参数 */
	float fKp;				//比例系数
	float fKi;				//积分系数
	float fKd;				//微分系数
	float fTs;				//采样周期(s)
	float fOutMin;			//输出下限(限幅)
	float fOutMax;			//输出上限(限幅)
	float fIntMin;			//积分项下限(抗饱和)
	float fIntMax;			//积分项上限(抗饱和)
	float fDerFc;			//微分低通截止频率(Hz),0=禁用微分
	/* 运行状态 */
	float fIntegral;		//积分累加值
	float fPrevMeas;		//上次反馈(微分计算)
	float fDerFilt;			//滤波后微分
	float fOut;				//本次输出
}
T_Pid_t;

/*	@brief 									PID控制器初始化
 * 	@param		pPid						PID结构体指针
 * 	@param		fKp						比例系数
 * 	@param		fKi						积分系数
 * 	@param		fKd						微分系数(0=不使用微分)
 * 	@param		fTs						采样周期(s)
 * 	@param		fOutMin					输出下限
 * 	@param		fOutMax					输出上限
 * 	@param		fIntMin					积分下限
 * 	@param		fIntMax					积分上限
 * 	@param		fDerFc					微分滤波截止频率(Hz),0=禁用微分
 *
 * */
void PID_Init(T_Pid_t *pPid, float fKp, float fKi, float fKd, float fTs,
				float fOutMin, float fOutMax, float fIntMin, float fIntMax, float fDerFc);

/*	@brief 									PID一步运算
 * 	@param		pPid						PID结构体指针
 * 	@param		fRef						设定值
 * 	@param		fFbk						反馈值
 * 	@retval									PID输出(已限幅)
 *
 * */
float PID_Update(T_Pid_t *pPid, float fRef, float fFbk);

/*	@brief 									PID复位(清零积分/微分状态)
 * 	@param		pPid						PID结构体指针
 *
 * */
void PID_Reset(T_Pid_t *pPid);

#endif

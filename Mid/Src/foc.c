/**
  ******************************************************************************
  * @file    foc.c
  * @brief   FOC 坐标变换中间层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 等幅(2/3)坐标变换。Clarke 幅度不变,三相平衡(A+B+C=0)时 α=A。
  * Park 使用 cos/sin 三角函数,需要链接 libm。
  ******************************************************************************
  */

#include "foc.h"
#include <math.h>

#define SQRT3 	1.7320508075688772f	//√3

/**
 * @brief 									Clarke变换: abc → αβ (等幅)
 * @param		pAbc						三相坐标输入
 * @param		pAb						αβ坐标输出
 * @note											α=A; β=(A+2B)/√3 (三相平衡时)
 *
 * */
void T_Clarke(const T_Abc_t *pAbc, T_AlphaBeta_t *pAb)
{
	pAb->fAlpha = pAbc->fA;
	pAb->fBeta  = (pAbc->fA + 2.0f * pAbc->fB) / SQRT3;
}

/**
 * @brief 									Clarke变换(带零序): abc → αβ0
 * @param		pAbc						三相坐标输入
 * @param		pAb0						αβ0坐标输出
 *
 * */
void T_Clarke2(const T_Abc_t *pAbc, T_Ab0_t *pAb0)
{
	pAb0->fAlpha = pAbc->fA;
	pAb0->fBeta  = (pAbc->fA + 2.0f * pAbc->fB) / SQRT3;
	pAb0->f0     = (pAbc->fA + pAbc->fB + pAbc->fC) / 3.0f;
}

/**
 * @brief 									逆Clarke变换: αβ → abc
 * @param		pAb						αβ坐标输入
 * @param		pAbc						三相坐标输出
 *
 * */
void T_InvClarke(const T_AlphaBeta_t *pAb, T_Abc_t *pAbc)
{
	pAbc->fA = pAb->fAlpha;
	pAbc->fB = -0.5f * pAb->fAlpha + (SQRT3 / 2.0f) * pAb->fBeta;
	pAbc->fC = -0.5f * pAb->fAlpha - (SQRT3 / 2.0f) * pAb->fBeta;
}

/**
 * @brief 									Park变换: αβ → dq
 * @param		pAb						αβ坐标输入
 * @param		fTheta						电角度(rad)
 * @param		pDq						dq坐标输出
 * @note											d=αcosθ+βsinθ; q=-αsinθ+βcosθ
 *
 * */
void T_Park(const T_AlphaBeta_t *pAb, float fTheta, T_Dq_t *pDq)
{
	float fCos = cosf(fTheta);
	float fSin = sinf(fTheta);

	pDq->fD = pAb->fAlpha * fCos + pAb->fBeta  * fSin;
	pDq->fQ = -pAb->fAlpha * fSin + pAb->fBeta  * fCos;
}

/**
 * @brief 									逆Park变换: dq → αβ
 * @param		pDq						dq坐标输入
 * @param		fTheta						电角度(rad)
 * @param		pAb						αβ坐标输出
 * @note											α=dcosθ-qsinθ; β=dsinθ+qcosθ
 *
 * */
void T_InvPark(const T_Dq_t *pDq, float fTheta, T_AlphaBeta_t *pAb)
{
	float fCos = cosf(fTheta);
	float fSin = sinf(fTheta);

	pAb->fAlpha = pDq->fD * fCos - pDq->fQ * fSin;
	pAb->fBeta  = pDq->fD * fSin + pDq->fQ * fCos;
}

/** @name SVPWM 空间矢量调制 **/
/* @{ */
/**
 * @brief 									SVPWM空间矢量调制(等效中心对齐法)
 * @param		pAb						αβ电压指令(逆Park输出,单位V)
 * @param		fUdc						母线电压(V)
 * @param		pfDutyA					输出A相占空比(0~1,供TIM比较值)
 * @param		pfDutyB					输出B相占空比(0~1)
 * @param		pfDutyC					输出C相占空比(0~1)
 * @note										实现思路:
 * 											1. 逆Clarke: αβ → 三相相电压 Va/Vb/Vc;
 * 											2. 叠加零序分量 Voff = -(Vmax+Vmin)/2 (中点注入),等效于
 * 											   扇区法SVPWM的中心对齐开关序列,线性调制区最大相电压峰值
 * 											   为 Udc/√3(较纯正弦PWM提高约15.5%);
 * 											3. 以母线电压中点归一化为占空比并限幅[0,1],过调制自动钳位。
 * */
void SVPWM(const T_AlphaBeta_t *pAb, float fUdc, float *pfDutyA, float *pfDutyB, float *pfDutyC)
{
	float fVa, fVb, fVc;		//三相相电压
	float fVmax, fVmin;			//三相最大值/最小值
	float fVoff;				//零序(中点)注入量
	float fDutyA, fDutyB, fDutyC;

	if (fUdc <= 0.0f)
	{
		*pfDutyA = 0.5f;
		*pfDutyB = 0.5f;
		*pfDutyC = 0.5f;
		return;
	}

	/** 1. 逆Clarke: αβ → 三相相电压(等幅) */
	fVa = pAb->fAlpha;
	fVb = -0.5f * pAb->fAlpha + (SQRT3 / 2.0f) * pAb->fBeta;
	fVc = -0.5f * pAb->fAlpha - (SQRT3 / 2.0f) * pAb->fBeta;

	/** 2. 零序中点注入(等效中心对齐SVPWM) */
	fVmax = (fVa > fVb) ? ((fVa > fVc) ? fVa : fVc) : ((fVb > fVc) ? fVb : fVc);
	fVmin = (fVa < fVb) ? ((fVa < fVc) ? fVa : fVc) : ((fVb < fVc) ? fVb : fVc);
	fVoff = -0.5f * (fVmax + fVmin);

	/** 3. 归一化占空比(以母线电压中点0.5为中心)并限幅 */
	fDutyA = 0.5f + (fVa + fVoff) / fUdc;
	fDutyB = 0.5f + (fVb + fVoff) / fUdc;
	fDutyC = 0.5f + (fVc + fVoff) / fUdc;

	if (fDutyA < 0.0f) fDutyA = 0.0f; else if (fDutyA > 1.0f) fDutyA = 1.0f;
	if (fDutyB < 0.0f) fDutyB = 0.0f; else if (fDutyB > 1.0f) fDutyB = 1.0f;
	if (fDutyC < 0.0f) fDutyC = 0.0f; else if (fDutyC > 1.0f) fDutyC = 1.0f;

	*pfDutyA = fDutyA;
	*pfDutyB = fDutyB;
	*pfDutyC = fDutyC;
}
/* @} */

/** @name PID 控制器 **/
/* @{ */
/**
 * @brief 									PID控制器初始化
 * @param		pPid						PID结构体指针
 * @param		fKp						比例系数
 * @param		fKi						积分系数
 * @param		fKd						微分系数(0=不使用微分)
 * @param		fTs						采样周期(s)
 * @param		fOutMin					输出下限
 * @param		fOutMax					输出上限
 * @param		fIntMin					积分下限
 * @param		fIntMax					积分上限
 * @param		fDerFc					微分滤波截止频率(Hz),0=禁用微分
 *
 * */
void PID_Init(T_Pid_t *pPid, float fKp, float fKi, float fKd, float fTs,
				float fOutMin, float fOutMax, float fIntMin, float fIntMax, float fDerFc)
{
	pPid->fKp = fKp;
	pPid->fKi = fKi;
	pPid->fKd = fKd;
	pPid->fTs = (fTs > 0.0f) ? fTs : 1e-4f;
	pPid->fOutMin = fOutMin;
	pPid->fOutMax = fOutMax;
	pPid->fIntMin = fIntMin;
	pPid->fIntMax = fIntMax;
	pPid->fDerFc  = fDerFc;
	PID_Reset(pPid);
}
/**
 * @brief 									PID一步运算(位置式)
 * @param		pPid						PID结构体指针
 * @param		fRef						设定值
 * @param		fFbk						反馈值
 * @retval									PID输出(已限幅)
 * @note											u = Kp·e + Ki·∫e·dt + Kd·(-dFbk/dt)
 * 											积分项累加后限幅(fIntMin/fIntMax)防积分饱和;
 * 											微分项取反馈微分经一阶低通滤波,避免设定值突变微分冲击;
 * 											输出经 fOutMin/fOutMax 限幅。
 *
 * */
float PID_Update(T_Pid_t *pPid, float fRef, float fFbk)
{
	float fErr;
	float fP, fI, fD;
	float fOut;

	fErr = fRef - fFbk;

	/** 比例 */
	fP = pPid->fKp * fErr;

	/** 积分(带限幅抗饱和) */
	pPid->fIntegral += pPid->fKi * fErr * pPid->fTs;
	if (pPid->fIntegral > pPid->fIntMax)
	{
		pPid->fIntegral = pPid->fIntMax;
	}
	else if (pPid->fIntegral < pPid->fIntMin)
	{
		pPid->fIntegral = pPid->fIntMin;
	}
	fI = pPid->fIntegral;

	/** 微分(反馈微分 + 一阶低通滤波,禁用时置0) */
	if ((pPid->fKd != 0.0f) && (pPid->fDerFc > 0.0f))
	{
		float fWc = 6.2831855f * pPid->fDerFc;					//2π·fc
		float fAlpha = (fWc * pPid->fTs) / (fWc * pPid->fTs + 1.0f);	//一阶低通系数
		float fDerRaw = (fFbk - pPid->fPrevMeas) / pPid->fTs;	//反馈微分
		pPid->fDerFilt = (1.0f - fAlpha) * pPid->fDerFilt + fAlpha * fDerRaw;
		fD = pPid->fKd * pPid->fDerFilt;
	}
	else
	{
		fD = 0.0f;
	}
	pPid->fPrevMeas = fFbk;

	/** 求和 + 输出限幅 */
	fOut = fP + fI + fD;
	if (fOut > pPid->fOutMax)
	{
		fOut = pPid->fOutMax;
	}
	else if (fOut < pPid->fOutMin)
	{
		fOut = pPid->fOutMin;
	}
	pPid->fOut = fOut;

	return fOut;
}
/**
 * @brief 									PID复位(清零积分/微分状态)
 * @param		pPid						PID结构体指针
 *
 * */
void PID_Reset(T_Pid_t *pPid)
{
	pPid->fIntegral = 0.0f;
	pPid->fPrevMeas = 0.0f;
	pPid->fDerFilt  = 0.0f;
	pPid->fOut      = 0.0f;
}
/* @} */

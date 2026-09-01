/**
  ******************************************************************************
  * @file    transfer.c
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

#include "transfer.h"
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

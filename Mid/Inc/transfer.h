/**
  ******************************************************************************
  * @file    transfer.h
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

#ifndef __TRANSFER_H
#define __TRANSFER_H

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

#endif

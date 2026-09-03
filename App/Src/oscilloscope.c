/**
  ******************************************************************************
  * @file    oscilloscope.c
  * @brief   App层示波器应用源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 以固定周期(默认1kHz)采集三相电流、母线电压、电角度、转子速度、ABI角度,
  * 通过 VOFA+ JustFloat 协议上报,上位机示波器实时显示。
  * 通道顺序: 0=Ia 1=Ib 2=Ic 3=Udc 4=θelec 5=Speed 6=θabi
  ******************************************************************************
  */

#include "oscilloscope.h"
#include "vofa.h"
#include "bsp_DRV8313.h"
#include "bsp_AS5047.h"
#include "bsp_pwm.h"
#include "motor.h"
#include "top_config.h"

#define OSC_TWO_PI 		6.283185307f	//2π

static VOFA_Handle_t 	stOSc;			//VOFA句柄
static uint32_t 		u32LastTick = 0;	//上次采样时刻(ms)
static int32_t 			i32LastCount = 0;	//上次测速基准计数
static float 			fSpeedRadS = 0.0f;	//转子角速度(rad/s)

/**
 * @brief 										更新转子角速度(rad/s)
 * @note											用ABI累计计数差分 / 固定周期估算:
 * 												ω = Δcount / CPR × 2π / Δt
 *
 * */
static void vOScUpdateSpeed(void)
{
	int32_t i32Now   = i32AS5047PEncGetCount(AS5047P);
	int32_t i32Delta = i32Now - i32LastCount;
	i32LastCount = i32Now;

	/* 每OSC_SAMPLE_PERIOD_MS差分一次, ω = Δcount/CPR×2π / (period_ms/1000) */
	fSpeedRadS = (float)i32Delta / (float)AS5047P_ENC_CPR
					* OSC_TWO_PI * (1000.0f / (float)OSC_SAMPLE_PERIOD_MS);
}

/**
 * @brief 										App层示波器应用初始化
 * @param		无
 * @note											配置VOFA通道数并初始化测速基准
 *
 * */
void OSc_Init(void)
{
	VOFA_Init(&stOSc, VOFA_TX_DMA);
	VOFA_ConfigChannels(&stOSc, OSC_CHANNEL_NUM);

	/* 测速基准初始化 */
	i32LastCount = i32AS5047PEncGetCount(AS5047P);
	u32LastTick  = HAL_GetTick();
}

/**
 * @brief 										立即采集并上报一帧
 * @param		无
 * @note											通道: Ia Ib Ic Udc θelec Speed θabi
 *
 * */
void OSc_ReportOnce(void)
{
    vDRV8313UpdateBusVoltage(DRV8313);	//更新母线电压(Udc)
	float fIa   = fDRV8313GetCurrentA(DRV8313, 0);	//A相电流(A)
	float fIb   = fDRV8313GetCurrentA(DRV8313, 1);	//B相电流(A)
	float fIc   = fDRV8313GetCurrentA(DRV8313, 2);	//C相电流(A)
	float fUdc  = fDRV8313GetBusVoltage(DRV8313);	//母线电压(V)
	//float fTheta= fMotorGetElecAngleRad(MOTOR);		//电角度(rad,按所选来源: 编码器/给定/自动生成)
	//float fAbi  = fAS5047PEncGetAngleRad(AS5047P);	//ABI单圈角度(rad)
	//float fThetare  = fAS5047PGetAngleElecRad(AS5047P);
	float fRad   = fAS5047PGetAngleRad(AS5047P);	
	float fDeg  = fAS5047PGetAngleDeg(AS5047P);	

	/* PWM 比较值(CCR,原始占空比计数,便于核对A/B/C三相PWM是否同步变化) */
	// float fCcr1 = (float)u32PwmGetCompare(PWM, emPwmChannel1);	//CCR1(A相)
	// float fCcr2 = (float)u32PwmGetCompare(PWM, emPwmChannel2);	//CCR2(B相)
	// float fCcr3 = (float)u32PwmGetCompare(PWM, emPwmChannel3);	//CCR3(C相)

	/* 速度 = ABI计数差分 */
	vOScUpdateSpeed();

	VOFA_SetData(&stOSc, 0, fIa);
	VOFA_SetData(&stOSc, 1, fIb);
	VOFA_SetData(&stOSc, 2, fIc);
	VOFA_SetData(&stOSc, 3, fUdc);
	//VOFA_SetData(&stOSc, 4, fTheta);
	VOFA_SetData(&stOSc, 4, fRad);
	VOFA_SetData(&stOSc, 5, fDeg);
	VOFA_SetData(&stOSc, 6, fSpeedRadS);

	// VOFA_SetData(&stOSc, 6, fThetare);
	// VOFA_SetData(&stOSc, 7, fCcr1);
	// VOFA_SetData(&stOSc, 8, fCcr2);
	// VOFA_SetData(&stOSc, 9, fCcr3);

	VOFA_Send(&stOSc);
}

/**
 * @brief 										示波器周期任务
 * @param		无
 * @note											主循环周期调用,到采样周期即上报一帧
 *
 * */
void OSc_Task(void)
{
	if ((HAL_GetTick() - u32LastTick) >= OSC_SAMPLE_PERIOD_MS)
	{
		u32LastTick = HAL_GetTick();
		OSc_ReportOnce();
	}
}

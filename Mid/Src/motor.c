/**
  ******************************************************************************
  * @file    motor.c
  * @brief   电机电角度获取模块源文件
  * @author  可以航行
  * @version V2.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 四种电角度来源: Auto(自动生成斜坡)/ Manual(固定值)/
  * EncoderSpi(AS5047 SPI绝对角)/ EncoderAbi(AS5047 ABI正交编码器)。
  * vMotorAngleUpdate() 每主循环刷新 Auto/EncSpi/EncAbi 三路缓存电角度(VOFA
  * 可同时显示比对),并按所选来源输出电角度供 FOC 使用。
  *
  * 编码器机械角来源: SPI 用 fAS5047PGetAngleRad(AS5047P),ABI 用
  * fAS5047PEncGetAngleRad(AS5047P)。二者需上层主循环已调用 vAS5047PUpdate /
  * vAS5047PEncUpdate 刷新。电角度 = (机械角 - 零位) × 极对数,对 2π 取模。
  ******************************************************************************
  */

#include "motor.h"
#include "bsp_config.h"
#include <math.h>

#define MOTOR_TWO_PI		6.283185307179586f	/* 2π */

/* ------------------ 模块运行状态(文件内静态) ------------------ */
static uint16_t 			s_u16PolePairs  = 1;	/* 极对数 */
static enumMotorAngleSrcTdf s_emSrc         = emMotorAngleSrc_Auto;	/* 当前来源 */

/* Auto 自动生成 */
static float 				s_fAutoFreqHz   = 0.0f;	/* 自动电频率(Hz) */
static float 				s_fAutoTheta    = 0.0f;	/* 自动电角度累加值(rad) */

/* Manual 固定值 */
static float 				s_fManualRad    = 0.0f;	/* 固定电角度(rad) */

/* 编码器零位偏移(机械角,rad) */
static float 				s_fSpiZeroRad   = 0.0f;
static float 				s_fAbiZeroRad   = 0.0f;

/* 三路缓存电角度(rad,供 VOFA 比对) */
static float 				s_fAutoElecRad  = 0.0f;	/* Auto 自动电角度 */
static float 				s_fSpiElecRad   = 0.0f;	/* EncoderSpi 电角度 */
static float 				s_fAbiElecRad   = 0.0f;	/* EncoderAbi 电角度 */

/* 当前输出电角度(所选来源) */
static float 				s_fOutRad       = 0.0f;

/* 自动斜坡时间基准 */
static uint32_t 			s_u32LastTick   = 0;

/**
 * @brief 										将角度归一化到 [0, 2π)
 * @param		fRad						输入角度(rad)
 * @retval										归一化后角度(rad,0~2π)
 *
 * */
static float fMotorNormalizeRad(float fRad)
{
	float fR = fmodf(fRad, MOTOR_TWO_PI);
	if (fR < 0.0f)
	{
		fR += MOTOR_TWO_PI;
	}
	return fR;
}

/**
 * @brief 								电角度模块初始化
 * @param		u16PolePairs			极对数
 * @note								复位默认来源=Auto、频率/相位/零位清零。
 *
 * */
void vMotorAngleInit(uint16_t u16PolePairs)
{
	if (u16PolePairs < 1)
	{
		u16PolePairs = 1;
	}
	s_u16PolePairs  = u16PolePairs;
	s_emSrc         = emMotorAngleSrc_Auto;
	s_fAutoFreqHz   = 0.0f;
	s_fAutoTheta    = 0.0f;
	s_fManualRad    = 0.0f;
	s_fSpiZeroRad   = 0.0f;
	s_fAbiZeroRad   = 0.0f;
	s_fAutoElecRad  = 0.0f;
	s_fSpiElecRad   = 0.0f;
	s_fAbiElecRad   = 0.0f;
	s_fOutRad       = 0.0f;
	s_u32LastTick   = 0;
}

/**
 * @brief 								设置当前电角度来源
 * @param		emSrc			来源枚举
 *
 * */
void vMotorSetAngleSource(enumMotorAngleSrcTdf emSrc)
{
	if ((emSrc >= emMotorAngleSrc_Auto) && (emSrc < emMotorAngleSrc_Max))
	{
		s_emSrc = emSrc;
	}
}

enumMotorAngleSrcTdf emMotorGetAngleSource(void)
{
	return s_emSrc;
}

const char *pMotorGetAngleSourceName(void)
{
	switch (s_emSrc)
	{
		case emMotorAngleSrc_Auto:      return "auto";
		case emMotorAngleSrc_Manual:    return "manual";
		case emMotorAngleSrc_EncoderSpi:return "encspi";
		case emMotorAngleSrc_EncoderAbi:return "encabi";
		default:                        return "?";
	}
}

/* ==================== Auto 自动生成 ==================== */
void vMotorSetAutoFreqHz(float fFreqHz)
{
	s_fAutoFreqHz = fFreqHz;
}

float fMotorGetAutoFreqHz(void)
{
	return s_fAutoFreqHz;
}

void vMotorResetAutoAngle(void)
{
	s_fAutoTheta  = 0.0f;
	s_u32LastTick = 0;
}

float fMotorGetAutoElecRad(void)
{
	return s_fAutoElecRad;
}

/* ==================== Manual 固定值 ==================== */
void vMotorSetManualElecRad(float fRad)
{
	s_fManualRad = fMotorNormalizeRad(fRad);
}

void vMotorSetManualElecDeg(float fDeg)
{
	s_fManualRad = fMotorNormalizeRad(fDeg * (MOTOR_TWO_PI / 360.0f));
}

float fMotorGetManualElecRad(void)
{
	return s_fManualRad;
}

/* ==================== Encoder SPI / ABI ==================== */
float fMotorGetEncoderSpiElecRad(void)
{
	return s_fSpiElecRad;
}

float fMotorGetEncoderAbiElecRad(void)
{
	return s_fAbiElecRad;
}

void vMotorCaptureEncoderZero(void)
{
	s_fSpiZeroRad = fAS5047PGetAngleRad(AS5047P);
	s_fAbiZeroRad = fAS5047PEncGetAngleRad(AS5047P);
}

/* ==================== 刷新 ==================== */
/**
 * @brief 								刷新电角度(主循环周期调用)
 * @note								① Auto 斜坡: θ += 2π·f·Δt(Δt 用 HAL_GetTick 实测);
 * 									② EncoderSpi/Abi: (机械角-零位)×极对数 取模;
 * 									③ 按所选来源输出电角度。
 * 									EncSPI/EncABI 依赖上层已调用 vAS5047PUpdate/vAS5047PEncUpdate。
 *
 * */
void vMotorAngleUpdate(void)
{
	uint32_t 	u32Now  = HAL_GetTick();
	float 		fDtSec;
	float 		fMech, fRel;

	/* 时间基准与增量 */
	if (s_u32LastTick == 0)
	{
		s_u32LastTick = u32Now;
		fDtSec = 0.0f;
	}
	else
	{
		fDtSec = (float)(u32Now - s_u32LastTick) / 1000.0f;
		s_u32LastTick = u32Now;
		/* 长时间停摆保护: 超过 1s 视为无意义增量,避免角度突跳 */
		if (fDtSec > 1.0f)
		{
			fDtSec = 0.0f;
		}
	}

	/** ① Auto 自动生成(斜坡积分) */
	s_fAutoTheta += MOTOR_TWO_PI * s_fAutoFreqHz * fDtSec;
	s_fAutoTheta  = fMotorNormalizeRad(s_fAutoTheta);
	s_fAutoElecRad = s_fAutoTheta;

	/** ② Encoder SPI: (机械角 - 零位) × 极对数,取模 */
	fMech = fAS5047PGetAngleRad(AS5047P);
	fRel  = fMech - s_fSpiZeroRad;
	s_fSpiElecRad = fMotorNormalizeRad(fRel * (float)s_u16PolePairs);

	/** ③ Encoder ABI: (机械角 - 零位) × 极对数,取模 */
	fMech = fAS5047PEncGetAngleRad(AS5047P);
	fRel  = fMech - s_fAbiZeroRad;
	s_fAbiElecRad = fMotorNormalizeRad(fRel * (float)s_u16PolePairs);

	/** ④ 按所选来源输出 */
	switch (s_emSrc)
	{
		case emMotorAngleSrc_Auto:      s_fOutRad = s_fAutoElecRad; break;
		case emMotorAngleSrc_Manual:    s_fOutRad = s_fManualRad;   break;
		case emMotorAngleSrc_EncoderSpi:s_fOutRad = s_fSpiElecRad;  break;
		case emMotorAngleSrc_EncoderAbi:s_fOutRad = s_fAbiElecRad;  break;
		default:                        s_fOutRad = s_fAutoElecRad; break;
	}
	s_fOutRad = fMotorNormalizeRad(s_fOutRad);
}

/* ==================== 输出获取 ==================== */
float fMotorGetElecAngleRad(void)
{
	return s_fOutRad;
}

float fMotorGetElecAngleDeg(void)
{
	return s_fOutRad * (360.0f / MOTOR_TWO_PI);
}

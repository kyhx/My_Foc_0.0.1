/**
  ******************************************************************************
  * @file    motor.c
  * @brief   电机电角度获取来源中间层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 三种电角度来源:
  *   - 自动生成(频率): 以调用时间差 Δt × 电角速度积分,得到一个随设定频率
  *     匀速旋转的电角度(开环旋转磁场)。Δt 用 HAL_GetTick 实测,兼顾主循环
  *     调用周期抖动;并做单步上限保护,防长时间停滞后角度突跳。
  *   - 固定值:         手动给定的恒定电角度,输出保持不变。
  *   - 编码器:         读取 AS5047P 单圈机械角 fAS5047PGetAngleRad,
  *                     减机械零位偏移后 × 极对数得电角度(供闭环)。
  * 所有输出统一归一化到 [0, 2π)。
  ******************************************************************************
  */

#include "motor.h"
#include "bsp_AS5047.h"
#include <math.h>

#define MOTOR_TWO_PI		6.283185307179586f		//2π
#define MOTOR_PI_OVER_180	0.017453292519943295f	//π/180
#define MOTOR_AUTO_DT_MAX	0.05f					//自动积分单步最大间隔(s),防停滞后角度突跳

/* ------------------ 模块运行状态(文件内静态) ------------------ */
static uint16_t  			s_u16PolePairs   = 7;		//极对数
static enumMotorAngleSrcTdf s_emSrc          = emMotorAngleSrc_Auto;	//当前来源
static float     			s_fElecAngle     = 0.0f;	//输出电角度(rad,[0,2π))

/* 自动生成(频率)状态 */
static float     			s_fAutoFreqHz    = 0.0f;	//电频率(Hz)
static float     			s_fAutoAngle     = 0.0f;	//自动积分角度
static uint32_t  			s_u32AutoLastTick= 0;		//上次积分时刻(ms)
static uint8_t   			s_u8AutoInited   = 0;		//自动时间基准是否已建立

/* 固定值状态 */
static float     			s_fManualAngle   = 0.0f;	//手动固定电角度

/* 编码器状态 */
static float     			s_fEncZeroOffset = 0.0f;	//编码器机械零位偏移(rad)

/**
 * @brief 							角度归一化到 [0, 2π)
 * @param		fAngle			输入角度(rad)
 * @retval							归一化角度 [0, 2π)
 *
 * */
static float fNormAngle(float fAngle)
{
	fAngle = fmodf(fAngle, MOTOR_TWO_PI);
	if (fAngle < 0.0f)
	{
		fAngle += MOTOR_TWO_PI;
	}
	return fAngle;
}

/**
 * @brief 							电角度模块初始化
 * @param		u16PolePairs	电机极对数(机械角→电角度)
 * @note								默认来源为自动生成,频率初始为 0;时间基准、角度、零位偏移复位。
 *
 * */
void vMotorAngleInit(uint16_t u16PolePairs)
{
	s_u16PolePairs    = (u16PolePairs != 0) ? u16PolePairs : 1;
	s_emSrc           = emMotorAngleSrc_Auto;
	s_fElecAngle      = 0.0f;

	s_fAutoFreqHz     = 0.0f;
	s_fAutoAngle      = 0.0f;
	s_u32AutoLastTick = 0;
	s_u8AutoInited    = 0;

	s_fManualAngle    = 0.0f;

	s_fEncZeroOffset  = 0.0f;
}

/**
 * @brief 							刷新当前电角度(按所选来源),需周期调用
 * @note								Auto:   按调用时间差 × 电频率积分;
 * 									Manual: 用固定值;
 * 									Encoder:读 AS5047P 机械角 × 极对数(减零位)。
 *
 * */
void vMotorAngleUpdate(void)
{
	switch (s_emSrc)
	{
	case emMotorAngleSrc_Auto:
	{
		uint32_t u32Now = HAL_GetTick();
		float    fDt;

		/** 首拍仅建立时间基准,不推进角度 */
		if (s_u8AutoInited == 0)
		{
			s_u32AutoLastTick = u32Now;
			s_u8AutoInited    = 1;
			break;
		}

		/** 实测相邻两次调用时间差(s),带 HAL 回绕保护与单步上限 */
		fDt = (float)(u32Now - s_u32AutoLastTick) / 1000.0f;
		s_u32AutoLastTick = u32Now;
		if (fDt < 0.0f)
		{
			fDt = 0.0f;
		}
		if (fDt > MOTOR_AUTO_DT_MAX)
		{
			fDt = MOTOR_AUTO_DT_MAX;
		}

		/** 积分推进: θ += 2π·f·Δt(负频率即反转) */
		s_fAutoAngle = fNormAngle(s_fAutoAngle + MOTOR_TWO_PI * s_fAutoFreqHz * fDt);
		s_fElecAngle = s_fAutoAngle;
	}
	break;

	case emMotorAngleSrc_Manual:
		s_fElecAngle = s_fManualAngle;
		break;

	case emMotorAngleSrc_Encoder:
	{
		float fMechRad = fAS5047PGetAngleRad(AS5047P);	//机械角(rad,[0,2π))
		float fElec    = (fMechRad - s_fEncZeroOffset) * (float)s_u16PolePairs;
		s_fElecAngle   = fNormAngle(fElec);
	}
	break;

	default:
		break;
	}
}

/**
 * @brief 							获取当前电角度(弧度)
 * @retval							电角度 [0, 2π)
 *
 * */
float fMotorGetElecAngleRad(void)
{
	return s_fElecAngle;
}

/**
 * @brief 							获取当前电角度(度)
 * @retval							电角度 [0, 360)
 *
 * */
float fMotorGetElecAngleDeg(void)
{
	return s_fElecAngle / MOTOR_PI_OVER_180;
}

/**
 * @brief 							设置电角度获取来源(运行时切换)
 * @param		emSrc			来源: Auto / Manual / Encoder
 *
 * */
void vMotorSetAngleSource(enumMotorAngleSrcTdf emSrc)
{
	if ((emSrc >= emMotorAngleSrc_Auto) && (emSrc <= emMotorAngleSrc_Encoder))
	{
		s_emSrc = emSrc;
	}
}

/**
 * @brief 							获取当前来源
 * @retval							当前来源枚举
 *
 * */
enumMotorAngleSrcTdf emMotorGetAngleSource(void)
{
	return s_emSrc;
}

/* ==================== 自动生成(频率)参数 ==================== */
/**
 * @brief 							设置自动生成电频率
 * @param		fFreqHz			电频率(Hz),正=正转,负=反转
 *
 * */
void vMotorSetAutoFreqHz(float fFreqHz)
{
	s_fAutoFreqHz = fFreqHz;
}

/**
 * @brief 							获取自动生成电频率
 * @retval							当前电频率(Hz)
 *
 * */
float fMotorGetAutoFreqHz(void)
{
	return s_fAutoFreqHz;
}

/**
 * @brief 							设置自动生成电角速度
 * @param		fOmega			电角速度(rad/s),正=正转,负=反转
 *
 * */
void vMotorSetAutoSpeedRadS(float fOmega)
{
	s_fAutoFreqHz = fOmega / MOTOR_TWO_PI;
}

/**
 * @brief 							获取自动生成电角速度
 * @retval							电角速度(rad/s)
 *
 * */
float fMotorGetAutoSpeedRadS(void)
{
	return s_fAutoFreqHz * MOTOR_TWO_PI;
}

/**
 * @brief 							自动生成角度清零(从 0 rad 重新开始扫描)
 *
 * */
void vMotorResetAutoAngle(void)
{
	s_fAutoAngle = 0.0f;
}

/* ==================== 固定值(手动)参数 ==================== */
/**
 * @brief 							设置手动固定电角度
 * @param		fElecRad		固定电角度(rad),自动归一化 [0, 2π)
 *
 * */
void vMotorSetManualElecRad(float fElecRad)
{
	s_fManualAngle = fNormAngle(fElecRad);
}

/**
 * @brief 							设置手动固定电角度(度)
 * @param		fElecDeg		固定电角度(度)
 *
 * */
void vMotorSetManualElecDeg(float fElecDeg)
{
	s_fManualAngle = fNormAngle(fElecDeg * MOTOR_PI_OVER_180);
}

/**
 * @brief 							获取手动设定的固定电角度
 * @retval							手动电角度(rad,归一化 [0, 2π))
 *
 * */
float fMotorGetManualElecRad(void)
{
	return s_fManualAngle;
}

/* ==================== 编码器参数 ==================== */
/**
 * @brief 							设置编码器机械零位偏移
 * @param		fZeroRad		机械零位偏移(rad)
 * @note								电角度 = (θmech - θzero) × 极对数
 *
 * */
void vMotorSetEncoderZeroOffsetRad(float fZeroRad)
{
	s_fEncZeroOffset = fNormAngle(fZeroRad);
}

/**
 * @brief 							获取编码器机械零位偏移
 * @retval							机械零位偏移(rad)
 *
 * */
float fMotorGetEncoderZeroOffsetRad(void)
{
	return s_fEncZeroOffset;
}

/**
 * @brief 							以当前 AS5047P 机械角捕获为零位
 * @note								使此刻电角度为 0
 *
 * */
void vMotorCaptureEncoderZero(void)
{
	s_fEncZeroOffset = fAS5047PGetAngleRad(AS5047P);
}

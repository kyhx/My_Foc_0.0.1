/**
  ******************************************************************************
  * @file    motor.h
  * @brief   电机电角度获取模块头文件
  * @author  可以航行
  * @version V2.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 电角度获取来源(enumMotorAngleSrcTdf),共 4 种,用于 FOC 测试:
  *   - Auto       自动生成(按电频率 2πf·Δt 积分,Δt 用 HAL_GetTick 实测)
  *   - Manual     固定值(给定恒定电角度)
  *   - EncoderSpi AS5047P SPI 模式(SPI 绝对角 × 极对数,减零位偏移)
  *   - EncoderAbi AS5047P ABI/TIM 正交编码器模式(ABI 角 × 极对数,减零位偏移)
  * 模块每次 vMotorAngleUpdate() 同时刷新 Auto / EncSpi / EncAbi 三个电角度缓存
  * (供 VOFA 同时显示比对),并按当前所选来源输出 fMotorGetElecAngleRad() 供 FOC。
  ******************************************************************************
  */

#ifndef __MOTOR_H
#define __MOTOR_H

#include "top_config.h"
#include <stdint.h>

/**	@brief 			电角度获取来源枚举(4 种)
 * 	@note				用作启动/测试来源选择;Max 用于取模循环切换
 *
 * */
typedef enum
{
	emMotorAngleSrc_Auto = 0,	/* 自动生成(斜坡) */
	emMotorAngleSrc_Manual,		/* 固定值 */
	emMotorAngleSrc_EncoderSpi,	/* 编码器 SPI 模式(AS5047 SPI 绝对角) */
	emMotorAngleSrc_EncoderAbi,	/* 编码器 ABI 模式(AS5047 ABI/TIM 正交编码器) */
	emMotorAngleSrc_Max,		/* 来源数量(用于循环切换取模) */
}
enumMotorAngleSrcTdf;

/*	@brief 						电角度模块初始化
 * 	@param		u16PolePairs			极对数(如 motor_config.h 的 MOTOR_CONFIG_POLE_PAIRS=7)
 * 	@note							复位默认来源=Auto、频率=0、零位偏移=0。
 *
 * */
void vMotorAngleInit(uint16_t u16PolePairs);

/*	@brief 						刷新电角度(主循环周期调用)
 * 	@note							同时更新 Auto 斜坡、EncSpi、EncAbi 三个缓存角度,
 * 								并按当前来源计算输出角度 fMotorGetElecAngleRad。
 *
 * */
void vMotorAngleUpdate(void);

/*	@brief 						设置当前电角度来源
 * 	@param		emSrc			来源枚举(Auto/Manual/EncoderSpi/EncoderAbi)
 *
 * */
void vMotorSetAngleSource(enumMotorAngleSrcTdf emSrc);

/*	@brief 						获取当前电角度来源
 * 	@retval						来源枚举
 *
 * */
enumMotorAngleSrcTdf emMotorGetAngleSource(void);

/*	@brief 						获取当前来源名称字符串(供串口/上位机显示)
 * 	@retval						"auto"/"manual"/"encspi"/"encabi"
 *
 * */
const char *pMotorGetAngleSourceName(void);

/*	@brief 						获取当前输出电角度(所选来源,rad,0~2π)
 * 	@retval						电角度(rad),供 FOC Park/逆Park
 *
 * */
float fMotorGetElecAngleRad(void);

/*	@brief 						获取当前输出电角度(所选来源,度)
 * 	@retval						电角度(°)
 *
 * */
float fMotorGetElecAngleDeg(void);

/* ==================== Auto 自动生成 ==================== */
/*	@brief 						设置自动生成电频率(Hz)
 * 	@param		fFreqHz			电频率(Hz),决定自动斜坡转速
 *
 * */
void vMotorSetAutoFreqHz(float fFreqHz);
/*	@brief 						获取自动生成电频率
 * 	@retval						电频率(Hz)
 *
 * */
float fMotorGetAutoFreqHz(void);
/*	@brief 						复位自动生成角度(清零相位与时间基准)
 * 	@note							启停/切换来源时可调用,保证斜坡从 0 平滑起步。
 *
 * */
void vMotorResetAutoAngle(void);
/*	@brief 						获取自动生成电角度(rad,VOFA显示用)
 * 	@retval						自动电角度(rad,0~2π)
 *
 * */
float fMotorGetAutoElecRad(void);

/* ==================== Manual 固定值 ==================== */
/*	@brief 						设置固定电角度(rad)
 * 	@param		fRad			电角度(rad),自动归一化到 0~2π
 *
 * */
void vMotorSetManualElecRad(float fRad);
/*	@brief 						设置固定电角度(度)
 * 	@param		fDeg			电角度(°)
 *
 * */
void vMotorSetManualElecDeg(float fDeg);
/*	@brief 						获取固定电角度
 * 	@retval						电角度(rad)
 *
 * */
float fMotorGetManualElecRad(void);

/* ==================== Encoder SPI / ABI(供 VOFA 比对) ==================== */
/*	@brief 						获取编码器 SPI 模式电角度(rad,0~2π)
 * 	@retval						电角度(rad)= (SPI机械角-零位)×极对数 取模
 *
 * */
float fMotorGetEncoderSpiElecRad(void);
/*	@brief 						获取编码器 ABI 模式电角度(rad,0~2π)
 * 	@retval						电角度(rad)= (ABI机械角-零位)×极对数 取模
 *
 * */
float fMotorGetEncoderAbiElecRad(void);

/*	@brief 						捕获编码器零位偏移
 * 	@note							把当前 SPI 与 ABI 机械角记为电角度 0(转子对齐后用)。
 *
 * */
void vMotorCaptureEncoderZero(void);

/*	@brief 						设置编码器方向
 * 	@param		i8Dir			+1=正向(默认); -1=反向(编码器机械角取反)
 * 	@note							上电调机方向确认发现反向时自动置 -1。
 *
 * */
void vMotorSetEncoderDir(int8_t i8Dir);
/*	@brief 						获取编码器方向
 * 	@retval						+1 或 -1
 *
 * */
int8_t i8MotorGetEncoderDir(void);

/* ==================== 运行控制 / 开环(实现于 App/app_motor.c) ==================== */
void vMotorSetRun(uint8_t bRun);
uint8_t u8MotorGetRun(void);
void vMotorOpenLoopSetVd(float fVd);
void vMotorOpenLoopSetVq(float fVq);
float fMotorGetOpenLoopVd(void);
float fMotorGetOpenLoopVq(void);
void vMotorOpenLoopRun(float fUdc);

#endif /* __MOTOR_H */

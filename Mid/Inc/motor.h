/**
  ******************************************************************************
  * @file    motor.h
  * @brief   电机电角度获取来源中间层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 电角度获取来源设置,支持三种来源运行时切换:
  *   - 自动生成(频率): 按设定的电频率匀速积分扫描,适合开环对相/扫频调试;
  *   - 固定值:         手动给定一个恒定电角度,适合定位/对相验证;
  *   - 编码器:         读取AS5047P机械角 × 极对数得电角度,供闭环使用。
  *
  * 使用流程:
  *   1. 上电调用 vMotorAngleInit(u16PolePairs) 初始化;
  *   2. 周期(主循环/电流环)调用 vMotorAngleUpdate() 按所选来源刷新电角度;
  *   3. 用 fMotorGetElecAngleRad() 读取当前电角度;
  *   4. 需要时用 vMotorSetAngleSource() 运行时切换来源,并设置对应参数。
  ******************************************************************************
  */

#ifndef __MOTOR_H
#define __MOTOR_H

#include "top_config.h"
#include <stdint.h>

/**	@brief 			电角度获取来源枚举
 * 	@note
 *
 * */
typedef enum
{
	emMotorAngleSrc_Auto = 0,	//自动生成(按电频率匀速积分)
	emMotorAngleSrc_Manual,		//固定值(手动给定)
	emMotorAngleSrc_Encoder,	//编码器(机械角 × 极对数)
}
enumMotorAngleSrcTdf;

/*	@brief 						电角度模块初始化
 * 	@param		u16PolePairs	电机极对数(机械角→电角度)
 * 	@note						默认来源为自动生成,频率初始为0(需 vMotorSetAutoFreqHz 设定),
 * 								时间基准、角度、零位偏移均复位。
 *
 * */
void vMotorAngleInit(uint16_t u16PolePairs);

/*	@brief 						刷新当前电角度(按所选来源),需周期调用
 * 	@note						- Auto:    用调用时间差 × 电频率积分推进角度;
 * 								- Manual:  使用手动设定的固定值;
 * 								- Encoder: 读 AS5047P 机械角 × 极对数(减零位偏移)。
 * 								编码器来源要求主循环已周期调用 vAS5047PUpdate(AS5047P)。
 *
 * */
void vMotorAngleUpdate(void);

/*	@brief 						获取当前电角度(弧度)
 * 	@retval						电角度,归一化 [0, 2π)
 *
 * */
float fMotorGetElecAngleRad(void);

/*	@brief 						获取当前电角度(度)
 * 	@retval						电角度 [0, 360)
 *
 * */
float fMotorGetElecAngleDeg(void);

/*	@brief 						设置电角度获取来源(运行时切换)
 * 	@param		emSrc			来源: Auto / Manual / Encoder
 * 	@note						切换不改变各来源已保存的参数。
 *
 * */
void vMotorSetAngleSource(enumMotorAngleSrcTdf emSrc);

/*	@brief 						获取当前来源
 * 	@retval						当前来源枚举
 *
 * */
enumMotorAngleSrcTdf emMotorGetAngleSource(void);

/* ==================== 自动生成(频率)参数 ==================== */
/*	@brief 						设置自动生成电频率
 * 	@param		fFreqHz			电频率(Hz),正=正转,负=反转
 *
 * */
void vMotorSetAutoFreqHz(float fFreqHz);

/*	@brief 						获取自动生成电频率
 * 	@retval						当前电频率(Hz)
 *
 * */
float fMotorGetAutoFreqHz(void);

/*	@brief 						设置自动生成电角速度
 * 	@param		fOmega			电角速度(rad/s),正=正转,负=反转
 * 	@note						等价于频率:f = ω / 2π
 *
 * */
void vMotorSetAutoSpeedRadS(float fOmega);

/*	@brief 						获取自动生成电角速度
 * 	@retval						电角速度(rad/s)
 *
 * */
float fMotorGetAutoSpeedRadS(void);

/*	@brief 						自动生成角度清零(从 0 rad 重新开始扫描)
 *
 * */
void vMotorResetAutoAngle(void);

/* ==================== 固定值(手动)参数 ==================== */
/*	@brief 						设置手动固定电角度
 * 	@param		fElecRad		固定电角度(rad),内部自动归一化 [0, 2π)
 *
 * */
void vMotorSetManualElecRad(float fElecRad);

/*	@brief 						设置手动固定电角度(度)
 * 	@param		fElecDeg		固定电角度(度)
 *
 * */
void vMotorSetManualElecDeg(float fElecDeg);

/*	@brief 						获取手动设定的固定电角度
 * 	@retval						手动电角度(rad,归一化 [0, 2π))
 *
 * */
float fMotorGetManualElecRad(void);

/* ==================== 编码器参数 ==================== */
/*	@brief 						设置编码器机械零位偏移
 * 	@param		fZeroRad		机械零位偏移(rad)
 * 	@note						电角度 = (θmech - θzero) × 极对数
 *
 * */
void vMotorSetEncoderZeroOffsetRad(float fZeroRad);

/*	@brief 						获取编码器机械零位偏移
 * 	@retval						机械零位偏移(rad)
 *
 * */
float fMotorGetEncoderZeroOffsetRad(void);

/*	@brief 						以当前 AS5047P 机械角捕获为零位
 * 	@note							常用于上电对齐/对相完成后记录机械零位,
 * 								使此刻电角度为 0。
 *
 * */
void vMotorCaptureEncoderZero(void);

/* ==================== 开环运行(启停) ==================== */
/*	@brief 						设置运行状态(开环启动/停止)
 * 	@param		bRun			1=启动, 0=停止
 * 	@note						启动瞬间自动生成角度清零(磁场从0 rad起),桥臂安全由
 * 								上层配合 vDRV8313Enable/Disable 控制。
 *
 * */
void vMotorSetRun(uint8_t bRun);

/*	@brief 						获取运行状态
 * 	@retval						1=运行, 0=停止
 *
 * */
uint8_t u8MotorGetRun(void);

/*	@brief 						设置开环 d 轴电压指令
 * 	@param		fVd			d轴电压指令(V)
 *
 * */
void vMotorOpenLoopSetVd(float fVd);

/*	@brief 						设置开环 q 轴电压指令(决定电流/转矩)
 * 	@param		fVq			q轴电压指令(V)
 *
 * */
void vMotorOpenLoopSetVq(float fVq);

/*	@brief 						获取开环 d 轴电压指令
 * 	@retval					d轴电压指令(V)
 *
 * */
float fMotorGetOpenLoopVd(void);

/*	@brief 						获取开环 q 轴电压指令
 * 	@retval					q轴电压指令(V)
 *
 * */
float fMotorGetOpenLoopVq(void);

/*	@brief 						开环运行一步输出(周期调用)
 * 	@param		fUdc			母线电压(V),供SVPWM归一化
 * 	@note						运行中: dq电压指令按当前电角度逆Park→SVPWM→三相占空比;
 * 								停止: 三相占空比回0.5。调用前需先 vMotorAngleUpdate()。
 *
 * */
void vMotorOpenLoopRun(float fUdc);

#endif

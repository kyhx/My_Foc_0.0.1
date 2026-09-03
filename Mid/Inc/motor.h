/**
  ******************************************************************************
  * @file    motor.h
  * @brief   电机中间层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 电机初始化状态机: "电流零偏校正 + 转子零位对齐"。
  *		- 电流校正: DRV8313 输出关断(零电流)期间,由 bsp_DRV8313 注入组自动低通累加三相零偏,
  *		  本模块等待其完成(状态=2)。
  *		- 零位对齐: 使能DRV8313,向A相注入占空比(θelec=0方向),转子锁定后读取机械角作为零位偏移。
  *		- 完成后提供补偿零位的电角度: θelec = (θmech - θoffset) × 极对数。
  *		配合 BSP: bsp_AS5047(编码器)/bsp_DRV8313(电流)/bsp_pwm(三相占空比)。
  ******************************************************************************
  */

#ifndef __MOTOR_H
#define __MOTOR_H

#include "top_config.h"
#include "bsp_AS5047.h"
#include "bsp_DRV8313.h"
#include "bsp_pwm.h"

/**	@brief 			MOTOR设备号枚举
 * 	@note
 *
 **/
typedef enum
{
	emMotorDeviceNum0 = 0,	//MOTOR设备0
}
enumMotorDeviceNumTdf;

/**	@brief 			MOTOR初始化状态枚举
 * 	@note
 *
 **/
typedef enum
{
	emMotorState_NotInit = 0,	//未开始
	emMotorState_Calibrating,	//电流零偏校正中(等待DRV8313校准完成)
	emMotorState_Aligning,		//零位对齐中(注入A相,等待转子锁定)
	emMotorState_Ready,			//初始化完成(可运行FOC)
	emMotorState_Error,			//初始化出错
}
enumMotorStateTdf;

/**	@brief 			电角度来源枚举
 * 	@note
 *
 **/
typedef enum
{
	emMotorAngleSrc_Encoder = 0,	//编码器读取(闭环FOC): θelec=(θmech-θoffset)×极对数
	emMotorAngleSrc_Manual,			//给定电角度(手动/定位): 由vMotorSetManualElecRad设置
	emMotorAngleSrc_Auto,			//自动生成(开环): 以电角度速度fAutoSpeedRadS匀速扫描
}
enumMotorAngleSrcTdf;

/**	@brief 			MOTOR静态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	float 						fAlignDuty;		//零位对齐注入占空比(0~1),注入A相产生θelec=0磁场
	uint32_t 					u32LockMs;		//转子锁定稳定等待时长(ms)
	uint32_t 					u32CalTimeoutMs;	//电流零偏校准超时(ms): 超时未完成转Error
	uint8_t 					u8Enable;		//1=执行零位对齐+电流校正; 0=跳过(直接Ready)
	float 						fAutoSpeedRadS;	//开环自动生成电角度速度(rad/s)
	enumMotorAngleSrcTdf 		emAngleSrc;		//默认电角度来源
	float 						fOlUdV;			//开环d轴电压指令(V)
	float 						fOlUqV;			//开环q轴电压指令(V)
}
stMotorStaticParameTdf;

/**	@brief 			MOTOR动态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	enumMotorStateTdf 			emState;		//初始化状态
	uint32_t 					u32Tick;		//状态计时(ms)
	float 						fZeroOffsetRad;	//零位偏移(机械角 rad): 对齐锁定处 θelec=0
	float 						fElecAngleRad;	//当前电角度(rad,按所选来源更新)
	enumMotorAngleSrcTdf 		emAngleSrc;		//当前电角度来源
	float 						fAutoAngleRad;	//自动生成累计电角度(rad)
	float 						fManualElecRad;	//给定电角度(rad)
	float 						fAutoSpeedRadS;	//自动生成电角度速度(rad/s)
	uint32_t 					u32LastTick;	//上次角度更新时间(ms)
	uint8_t 					u8Run;			//开环运行标志: 1=转动 0=停止
}
stMotorDynamicParameTdf;

/**	@brief 			MOTOR设备结构体定义
 * 	@note
 *
 **/
typedef struct
{
	stMotorStaticParameTdf 	MotorStaticParame;
	stMotorDynamicParameTdf MotorDynamicParame;
}
stMotorDeviceParameTdf;

/**	@brief 				函数外部声明
 * 	@note
 *
 **/
void 		vMotorInit(stMotorStaticParameTdf *pstInit, enumMotorDeviceNumTdf emDeviceNum);
const 		stMotorDeviceParameTdf *c_pstGetMotorDeviceParame(enumMotorDeviceNumTdf emDeviceNum);

/** 初始化状态机: 主循环周期调用 */
void 		vMotorInitTask(enumMotorDeviceNumTdf emDeviceNum);

/* 电角度来源设置 */
void 				vMotorSetAngleSource(enumMotorDeviceNumTdf emDeviceNum, enumMotorAngleSrcTdf emSrc);
void 				vMotorSetManualElecRad(enumMotorDeviceNumTdf emDeviceNum, float fAngleRad);
void 				vMotorSetAutoSpeed(enumMotorDeviceNumTdf emDeviceNum, float fSpeedRadS);
void 				vMotorAngleUpdate(enumMotorDeviceNumTdf emDeviceNum);

/* 开环驱动: 用当前电角度来源驱动三相PWM */
void 				vMotorOpenLoopRun(enumMotorDeviceNumTdf emDeviceNum);
void 				vMotorSetRun(enumMotorDeviceNumTdf emDeviceNum, uint8_t u8Run);
uint8_t 			u8MotorGetRun(enumMotorDeviceNumTdf emDeviceNum);
float 				fMotorGetAutoSpeed(enumMotorDeviceNumTdf emDeviceNum);

/* 通用读取接口 */
enumMotorStateTdf 	emMotorGetState(enumMotorDeviceNumTdf emDeviceNum);
uint8_t 			u8MotorIsReady(enumMotorDeviceNumTdf emDeviceNum);
uint8_t 			u8MotorIsError(enumMotorDeviceNumTdf emDeviceNum);	//1=初始化出错(校准超时/对齐故障)
float 				fMotorGetZeroOffsetRad(enumMotorDeviceNumTdf emDeviceNum);
float 				fMotorGetElecAngleRad(enumMotorDeviceNumTdf emDeviceNum);
enumMotorAngleSrcTdf emMotorGetAngleSource(enumMotorDeviceNumTdf emDeviceNum);

#endif

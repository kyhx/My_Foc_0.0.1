/**
  ******************************************************************************
  * @file    bsp_pwm.h
  * @brief   PWMBSP层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 本文件提供定时器PWM硬件抽象层接口,供中间件和应用层(FOC/电机)调用。
  *		- 支持最多 PWM_MAX_CHANNEL_NUM 路输出通道(默认3路,映射A/B/C三相)。
  *		- 统一以占空比(0~1)为接口,内部换算为定时器比较值 CCR = duty × ARR。
  *		- 兼容中心对齐/边沿对齐PWM,周期在初始化时从TIM读取或指定。
  *		- 典型应用: TIM1 CH1/CH2/CH3 输出三相PWM,配合DRV8313驱动电机。
  ******************************************************************************
  */

#ifndef __BSP_PWM__H
#define __BSP_PWM__H

#include "bsp_config.h"
#include "main.h"

/**	@brief 			PWM最大通道数
 * 	@note
 *
 **/
#define PWM_MAX_CHANNEL_NUM		3		//最多支持的PWM输出通道数

/**	@brief 			PWM设备号枚举
 * 	@note
 *
 **/
typedef enum
{
	emPwmDeviceNum0 = 0,	//PWM设备0
}
enumPwmDeviceNumTdf;

/**	@brief 			PWM通道号枚举(对应输出索引)
 * 	@note			索引经静态参数 aemChannel[] 映射到具体TIM通道。
 * 					默认索引0/1/2 → TIM_CHANNEL_1/2/3。
 *
 **/
typedef enum
{
	emPwmChannel1 = 0,		//通道索引0(默认TIM_CH1)
	emPwmChannel2,			//通道索引1(默认TIM_CH2)
	emPwmChannel3,			//通道索引2(默认TIM_CH3)
}
enumPwmChannelTdf;

/**	@brief 			PWM静态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	TIM_HandleTypeDef 			*pstTimHandle;		//HAL库定时器句柄(htim1)
	uint16_t 					u16Period;			//PWM周期(ARR),0则取定时器当前ARR
	enumPwmChannelTdf 			aemChannel[PWM_MAX_CHANNEL_NUM];	//通道映射表
	uint8_t 					u8ChannelNum;		//有效通道数(≤PWM_MAX_CHANNEL_NUM)
}
stPwmStaticParameTdf;

/**	@brief 			PWM动态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	uint16_t 					u16Period;								//当前周期(ARR)
	float 						afDuty[PWM_MAX_CHANNEL_NUM];			//各通道当前占空比(0~1)
	uint32_t 					au32Compare[PWM_MAX_CHANNEL_NUM];		//各通道当前比较值(CCR)
}
stPwmDynamicParameTdf;

/**	@brief 			PWM设备结构体定义
 * 	@note
 *
 **/
typedef struct
{
	stPwmStaticParameTdf 	PwmStaticParame;
	stPwmDynamicParameTdf 	PwmDynamicParame;
}
stPwmDeviceParameTdf;

/**	@brief 				函数外部声明
 * 	@note
 *
 **/
void 		vPwmDeviceInit(stPwmStaticParameTdf *pstInit, enumPwmDeviceNumTdf emDeviceNum);
const 		stPwmDeviceParameTdf *c_pstGetPwmDeviceParame(enumPwmDeviceNumTdf emDeviceNum);

/* 启停 */
void 		vPwmStart(enumPwmDeviceNumTdf emDeviceNum);		//启动所有有效通道PWM输出
void 		vPwmStop(enumPwmDeviceNumTdf emDeviceNum);			//停止所有有效通道PWM输出

/* 占空比设置(0~1) */
void 		vPwmSetDuty(enumPwmDeviceNumTdf emDeviceNum, enumPwmChannelTdf emChannel, float fDuty);
void 		vPwmSetDutyAll(enumPwmDeviceNumTdf emDeviceNum, float fDutyA, float fDutyB, float fDutyC);
void 		vPwmSetDutyArray(enumPwmDeviceNumTdf emDeviceNum, const float *pfDuty, uint8_t u8Num);

/* 比较值设置(CCR) */
void 		vPwmSetCompare(enumPwmDeviceNumTdf emDeviceNum, enumPwmChannelTdf emChannel, uint32_t u32Compare);

/* 通用读取接口 */
float 		fPwmGetDuty(enumPwmDeviceNumTdf emDeviceNum, enumPwmChannelTdf emChannel);
uint32_t 	u32PwmGetCompare(enumPwmDeviceNumTdf emDeviceNum, enumPwmChannelTdf emChannel);
uint16_t 	u16PwmGetPeriod(enumPwmDeviceNumTdf emDeviceNum);

#endif

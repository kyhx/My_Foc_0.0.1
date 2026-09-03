/**
  ******************************************************************************
  * @file    bsp_pwm.c
  * @brief   PWMBSP层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 	本文件实现定时器PWM硬件抽象层,封装HAL库操作
  *		占空比(0~1) ↔ 比较值(CCR = duty × ARR) 统一换算。
  *		典型应用: TIM1 CH1/CH2/CH3 三相PWM,配合DRV8313驱动FOC电机。
  ******************************************************************************
  */

#include	"bsp_pwm.h"
#include	"bsp_config.h"

//【0】PWM设备。
stPwmDeviceParameTdf arrystPwmDeviceparam[PWM_DEVICE_NUM];

/**
 * @brief 										通道索引 → HAL TIM通道常量
 * @param		emChannel		 	PWM通道索引
 * @retval									HAL TIM通道常量(TIM_CHANNEL_x)
 * @note											索引经静态参数映射表转换;越界默认返回CH1。
 *
 * */
static uint32_t u32MapHALChannel(enumPwmChannelTdf emChannel)
{
	switch (emChannel)
	{
		case emPwmChannel1: return TIM_CHANNEL_1;
		case emPwmChannel2: return TIM_CHANNEL_2;
		case emPwmChannel3: return TIM_CHANNEL_3;
		default:             return TIM_CHANNEL_1;
	}
}
/**
 * @brief 										PWM设备初始化
 * @param		pstInit					 	PWM静态参数结构体地址
 * @param		emDeviceNum		 		PWM设备号
 * @note											u16Period=0时读取定时器当前ARR;
 * 											aemChannel映射表未填时默认 CH1/CH2/CH3。
 * 											初始化仅记录配置并复位动态参数,不启动PWM(由vPwmStart启动)。
 *
 * */
void vPwmDeviceInit(stPwmStaticParameTdf *pstInit, enumPwmDeviceNumTdf emDeviceNum)
{
	stPwmDeviceParameTdf 	*pstDev = &arrystPwmDeviceparam[emDeviceNum];
	uint8_t 				u8Idx;

	/** 静态参数 */
	pstDev->PwmStaticParame.pstTimHandle = pstInit->pstTimHandle;
	pstDev->PwmStaticParame.u8ChannelNum = pstInit->u8ChannelNum;
	if (pstDev->PwmStaticParame.u8ChannelNum > PWM_MAX_CHANNEL_NUM)
	{
		pstDev->PwmStaticParame.u8ChannelNum = PWM_MAX_CHANNEL_NUM;
	}
	if (pstDev->PwmStaticParame.u8ChannelNum == 0)
	{
		pstDev->PwmStaticParame.u8ChannelNum = 3;
	}

	/** 周期: 0则取定时器当前ARR */
	if (pstInit->u16Period == 0)
	{
		pstDev->PwmStaticParame.u16Period = (uint16_t)__HAL_TIM_GET_AUTORELOAD(pstInit->pstTimHandle);
	}
	else
	{
		pstDev->PwmStaticParame.u16Period = pstInit->u16Period;
	}

	/** 通道映射表: 未填则默认 CH1/CH2/CH3 */
	for (u8Idx = 0; u8Idx < PWM_MAX_CHANNEL_NUM; u8Idx++)
	{
		if (u8Idx < pstInit->u8ChannelNum)
		{
			pstDev->PwmStaticParame.aemChannel[u8Idx] = pstInit->aemChannel[u8Idx];
		}
		else
		{
			pstDev->PwmStaticParame.aemChannel[u8Idx] = (enumPwmChannelTdf)u8Idx;
		}
	}

	/** 动态参数复位 */
	pstDev->PwmDynamicParame.u16Period = pstDev->PwmStaticParame.u16Period;
	for (u8Idx = 0; u8Idx < PWM_MAX_CHANNEL_NUM; u8Idx++)
	{
		pstDev->PwmDynamicParame.afDuty[u8Idx]      = 0.0f;
		pstDev->PwmDynamicParame.au32Compare[u8Idx] = 0;
	}
}
/**
 * @brief 										取得PWM设备参数
 * @param		emDeviceNum		 	PWM设备号
 *
 * */
const stPwmDeviceParameTdf *c_pstGetPwmDeviceParame(enumPwmDeviceNumTdf emDeviceNum)
{
	return &arrystPwmDeviceparam[emDeviceNum];
}
/**
 * @brief 										启动PWM输出(所有有效通道)
 * @param		emDeviceNum		 	PWM设备号
 * @note											按静态参数通道映射表依次 HAL_TIM_PWM_Start。
 *
 * */
void vPwmStart(enumPwmDeviceNumTdf emDeviceNum)
{
	stPwmDeviceParameTdf 	*pstDev = &arrystPwmDeviceparam[emDeviceNum];
	TIM_HandleTypeDef 		*pstTim = pstDev->PwmStaticParame.pstTimHandle;
	uint8_t 				u8Idx;

	for (u8Idx = 0; u8Idx < pstDev->PwmStaticParame.u8ChannelNum; u8Idx++)
	{
		HAL_TIM_PWM_Start(pstTim, u32MapHALChannel(pstDev->PwmStaticParame.aemChannel[u8Idx]));
		HAL_TIMEx_PWMN_Start(pstTim, u32MapHALChannel(pstDev->PwmStaticParame.aemChannel[u8Idx]));
	}
}
/**
 * @brief 										停止PWM输出(所有有效通道)
 * @param		emDeviceNum		 	PWM设备号
 *
 * */
void vPwmStop(enumPwmDeviceNumTdf emDeviceNum)
{
	stPwmDeviceParameTdf 	*pstDev = &arrystPwmDeviceparam[emDeviceNum];
	TIM_HandleTypeDef 		*pstTim = pstDev->PwmStaticParame.pstTimHandle;
	uint8_t 				u8Idx;

	for (u8Idx = 0; u8Idx < pstDev->PwmStaticParame.u8ChannelNum; u8Idx++)
	{
		HAL_TIM_PWM_Stop(pstTim, u32MapHALChannel(pstDev->PwmStaticParame.aemChannel[u8Idx]));
		HAL_TIMEx_PWMN_Stop(pstTim, u32MapHALChannel(pstDev->PwmStaticParame.aemChannel[u8Idx]));

	}
}
/**
 * @brief 										设置单通道占空比(0~1)
 * @param		emDeviceNum		 	PWM设备号
 * @param		emChannel		 	PWM通道索引
 * @param		fDuty				占空比(0~1)
 * @note											换算 CCR = duty × ARR 并写入定时器比较寄存器,
 * 											同时更新动态参数缓存。
 *
 * */
void vPwmSetDuty(enumPwmDeviceNumTdf emDeviceNum, enumPwmChannelTdf emChannel, float fDuty)
{
	stPwmDeviceParameTdf 	*pstDev = &arrystPwmDeviceparam[emDeviceNum];
	uint8_t 				u8Idx;
	uint32_t 				u32Compare;

	if ((uint8_t)emChannel >= PWM_MAX_CHANNEL_NUM)
	{
		return;
	}

	/** 占空比限幅 */
	if (fDuty < 0.0f) fDuty = 0.0f;
	else if (fDuty > 1.0f) fDuty = 1.0f;

	u32Compare = (uint32_t)(fDuty * (float)pstDev->PwmDynamicParame.u16Period);

	pstDev->PwmDynamicParame.afDuty[emChannel]      = fDuty;
	pstDev->PwmDynamicParame.au32Compare[emChannel] = u32Compare;

	/** 按映射表定位实际TIM通道 */
	u8Idx = (uint8_t)pstDev->PwmStaticParame.aemChannel[emChannel];
	__HAL_TIM_SET_COMPARE(pstDev->PwmStaticParame.pstTimHandle,
							u32MapHALChannel((enumPwmChannelTdf)u8Idx), u32Compare);
}
/**
 * @brief 										设置三相占空比(A/B/C)
 * @param		emDeviceNum		 	PWM设备号
 * @param		fDutyA				 A相占空比(0~1)
 * @param		fDutyB				 B相占空比(0~1)
 * @param		fDutyC				 C相占空比(0~1)
 * @note											批量调用vPwmSetDuty(通道0/1/2)。
 *
 * */
void vPwmSetDutyAll(enumPwmDeviceNumTdf emDeviceNum, float fDutyA, float fDutyB, float fDutyC)
{
	vPwmSetDuty(emDeviceNum, emPwmChannel1, fDutyA);
	vPwmSetDuty(emDeviceNum, emPwmChannel2, fDutyB);
	vPwmSetDuty(emDeviceNum, emPwmChannel3, fDutyC);
}
/**
 * @brief 										批量设置占空比数组
 * @param		emDeviceNum		 	PWM设备号
 * @param		pfDuty				占空比数组(0~1)
 * @param		u8Num				通道数(≤PWM_MAX_CHANNEL_NUM)
 *
 * */
void vPwmSetDutyArray(enumPwmDeviceNumTdf emDeviceNum, const float *pfDuty, uint8_t u8Num)
{
	uint8_t u8Idx;

	if (u8Num > PWM_MAX_CHANNEL_NUM)
	{
		u8Num = PWM_MAX_CHANNEL_NUM;
	}
	for (u8Idx = 0; u8Idx < u8Num; u8Idx++)
	{
		vPwmSetDuty(emDeviceNum, (enumPwmChannelTdf)u8Idx, pfDuty[u8Idx]);
	}
}
/**
 * @brief 										设置单通道比较值(CCR)
 * @param		emDeviceNum		 	PWM设备号
 * @param		emChannel		 	PWM通道索引
 * @param		u32Compare			比较值(≤ARR)
 * @note											同时换算并缓存对应占空比。
 *
 * */
void vPwmSetCompare(enumPwmDeviceNumTdf emDeviceNum, enumPwmChannelTdf emChannel, uint32_t u32Compare)
{
	stPwmDeviceParameTdf 	*pstDev = &arrystPwmDeviceparam[emDeviceNum];
	uint8_t 				u8Idx;
	float 					fDuty;

	if ((uint8_t)emChannel >= PWM_MAX_CHANNEL_NUM)
	{
		return;
	}
	if (u32Compare > pstDev->PwmDynamicParame.u16Period)
	{
		u32Compare = pstDev->PwmDynamicParame.u16Period;
	}

	fDuty = (float)u32Compare / (float)pstDev->PwmDynamicParame.u16Period;

	pstDev->PwmDynamicParame.afDuty[emChannel]      = fDuty;
	pstDev->PwmDynamicParame.au32Compare[emChannel] = u32Compare;

	u8Idx = (uint8_t)pstDev->PwmStaticParame.aemChannel[emChannel];
	__HAL_TIM_SET_COMPARE(pstDev->PwmStaticParame.pstTimHandle,
							u32MapHALChannel((enumPwmChannelTdf)u8Idx), u32Compare);
}
/**
 * @brief 										获取单通道占空比
 * @param		emDeviceNum		 	PWM设备号
 * @param		emChannel		 	PWM通道索引
 * @retval										占空比(0~1)
 *
 * */
float fPwmGetDuty(enumPwmDeviceNumTdf emDeviceNum, enumPwmChannelTdf emChannel)
{
	if ((uint8_t)emChannel >= PWM_MAX_CHANNEL_NUM)
	{
		return 0.0f;
	}
	return arrystPwmDeviceparam[emDeviceNum].PwmDynamicParame.afDuty[emChannel];
}
/**
 * @brief 										获取单通道比较值
 * @param		emDeviceNum		 	PWM设备号
 * @param		emChannel		 	PWM通道索引
 * @retval										比较值(CCR)
 *
 * */
uint32_t u32PwmGetCompare(enumPwmDeviceNumTdf emDeviceNum, enumPwmChannelTdf emChannel)
{
	if ((uint8_t)emChannel >= PWM_MAX_CHANNEL_NUM)
	{
		return 0;
	}
	return arrystPwmDeviceparam[emDeviceNum].PwmDynamicParame.au32Compare[emChannel];
}
/**
 * @brief 										获取当前周期
 * @param		emDeviceNum		 	PWM设备号
 * @retval										周期(ARR)
 *
 * */
uint16_t u16PwmGetPeriod(enumPwmDeviceNumTdf emDeviceNum)
{
	return arrystPwmDeviceparam[emDeviceNum].PwmDynamicParame.u16Period;
}

/**
  ******************************************************************************
  * @file    bsp_led.c
  * @brief   LEDBSP层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 本文件实现LED硬件抽象层，封装HAL库操作
  ******************************************************************************
  */
#include	"bsp_led.h"
#include	"bsp_config.h"


//【0】LED启动指示灯。【1】是PWNEN使能指示灯。
stLedDeviceParameTdf arrystLedDeviceparam[LED_DEVICE_NUM];

/**
 * @brief 										led参数结构体初始化
 * @param		pstInit					 	led参数结构体地址
 * @param		emDeviceNum		 		led设备号
 *
 **/
void vLedDeviceInit(stLedStaticParameTdf *pstInit, enumLedDeviceNumTdf emDeviceNum)
{
	arrystLedDeviceparam[emDeviceNum].LedStaticParame.pstGPIOBase = pstInit->pstGPIOBase;
	arrystLedDeviceparam[emDeviceNum].LedStaticParame.u16GPIOPin = pstInit->u16GPIOPin;
	arrystLedDeviceparam[emDeviceNum].LedStaticParame.emOnLevel = pstInit->emOnLevel;
	/* 动态参数复位: 默认熄灭 */
	arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus = emLedstatus_Off;
}
/**
 * @brief 										取得led设备参数
 * @param		emenDeviceNum		 	led设备号
 *
 **/
const stLedDeviceParameTdf *c_pstGetLedDeviceParame(enumLedDeviceNumTdf emDeviceNum)
{
	return &arrystLedDeviceparam[emDeviceNum];

}
/**
 * 	@brief 											更新Led引脚电平,将动态状态落到处决策
 *  @param		emDeviceNum	 		led设备号
 * 	@note											根据点亮电平(emOnLevel)与期望状态(emLedstatus)推导
 * 											实际输出的GPIO电平: 点亮电平=高则点亮输出高,否则输出低。
 *
 **/
void vLedUpdatePinLevel(enumLedDeviceNumTdf emDeviceNum)
{
	stLedDeviceParameTdf	*pstDev = &arrystLedDeviceparam[emDeviceNum];
	GPIO_PinState 			enOutput;

	/* 期望点亮且点亮电平==高 → 输出高; 其余输出低 */
	enOutput = ((pstDev->LedDynamicParame.emLedstatus == emLedstatus_On)
				&& (pstDev->LedStaticParame.emOnLevel == emLedOnLevel_High))
					? GPIO_PIN_SET : GPIO_PIN_RESET;

	HAL_GPIO_WritePin(pstDev->LedStaticParame.pstGPIOBase,
			pstDev->LedStaticParame.u16GPIOPin, enOutput);
}

/**
 * 	@brief 											led点亮
 * 	@param		emDeviceNum	 		led设备号
 **/
void vLedOn(enumLedDeviceNumTdf emDeviceNum)
{
	arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus = emLedstatus_On;
	vLedUpdatePinLevel(emDeviceNum);
}
/**
 * 	@brief 											led熄灭
 * 	@param		emDeviceNum			led设备号
 **/
void vLedOff(enumLedDeviceNumTdf emDeviceNum)
{
	arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus = emLedstatus_Off;
	vLedUpdatePinLevel(emDeviceNum);
}
/**
 * 	@brief 											led翻转
 * 	@param		emDeviceNum			led设备号
 **/
void vLedToggle(enumLedDeviceNumTdf emDeviceNum)
{
	arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus =
		(enumLedstatusTdf)!(arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus);
	vLedUpdatePinLevel(emDeviceNum);
}



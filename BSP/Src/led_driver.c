/**
 * @file			led_driver.c
 * @author 		可以航行
 * @version 	0.1
 * @date 			2026/8/19
 * @brief 		led驱动代码
 *
 **/

#include	"led_driver.h"
#include	"string.h"
#include	"top_config.h"


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
//		memcpy(&arrystLedDeviceparam[emDeviceNum].LedStaticParame,pstInit,sizeof(stLedDeviceParameTdf)/sizeof(uint8_t));
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
 * 	@brief 											更新Led状态，分离led参数与执行
 *  @param		enenDeviceNum	 		led设备号
 *	@param		emOnLevel	 				点亮电平
 *
 **/
void vLedUpdataPinLevel(enumLedDeviceNumTdf emDeviceNum)
{
	uint8_t OutputLevel;
	OutputLevel =	! (arrystLedDeviceparam[emDeviceNum].LedStaticParame.emOnLevel
		^ arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus);
	//根据LED状态表得到

	HAL_GPIO_WritePin(arrystLedDeviceparam[emDeviceNum].LedStaticParame.pstGPIOBase,
			arrystLedDeviceparam[emDeviceNum].LedStaticParame.u16GPIOPin, (GPIO_PinState)OutputLevel);
}

/**
 * 	@brief 											led点亮
 * 	@param		enenDeviceNum	 		led设备号
 *	@param		emLedstatus				led状态
 **/
void vLedOn(enumLedDeviceNumTdf emDeviceNum)
{
	arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus = emLedstatus_On;
	vLedUpdataPinLevel(emDeviceNum);
	
}
/**
 * 	@brief 											led熄灭
 * 	@param		enenDeviceNum			led设备号
 *	@param		emLedstatus				led状态
 **/
void vLedOff(enumLedDeviceNumTdf emDeviceNum)
{

	arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus = emLedstatus_Off;
	vLedUpdataPinLevel(emDeviceNum);
	
}
/**
 * 	@brief 											led翻转
 * 	@param		enenDeviceNum			led设备号
 *	@param		emLedstatus				led状态
 **/
void vLedToggle(enumLedDeviceNumTdf emDeviceNum)
{

	arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus = 
	(enumLedstatusTdf)!arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedstatus;
	vLedUpdataPinLevel(emDeviceNum);
	
}



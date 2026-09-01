/**
  ******************************************************************************
  * @file    bsp_led.h
  * @brief   LEDBSP层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 本文件提供LED硬件抽象层接口，供中间件和应用层调用
  ******************************************************************************
  */
  
#ifndef __BSP_LED__H
#define __BSP_LED__H

#include "bsp_config.h"

#include "main.h"

/*	@brief 				LED设备号枚举
 * 	@note
 *
 * */
typedef enum   
{

	emLedDeviceNum0		 	= 		0,//Led设备号
	emLedDeviceNum1,				//1
	emLedDeviceNum2,				//2
	emLedDeviceNum3,
	
}
enumLedDeviceNumTdf;


/*	@brief 				LED状态枚举
 * 	@note
 *
 * */
typedef enum   
{
	emLedstatus_Off		 = 0,
	emLedstatus_On	,
}
enumLedstatusTdf;


/*	@brief 				LED点亮电平枚举
 * 	@note
 *
 * */
typedef enum   
{

	emLedOnLevel_Low		 = 0,
	emLedOnLevel_High,
	
}
enumLedOnLevelTdf;

/*	@brief 				LED静态参数结构体定义
 * 	@note
 *
 * */
typedef struct  
{

	GPIO_TypeDef 					*pstGPIOBase;				//使用什么GPIOx
	uint16_t 							u16GPIOPin;					//使用的GPIOPinx
	enumLedOnLevelTdf			emOnLevel;					//LED点亮的电平
	
}
stLedStaticParameTdf;

/*	@brief 				LED动态参数结构体定义
 * 	@note
 *
 * */
typedef struct  
{
	enumLedstatusTdf			emLedstatus;				//LEd当前状态
	
}
stLedDynamicParameTdf;


/*	@brief 				LED参数结构体定义
 * 	@note
 *
 * */
typedef struct  
{
	stLedStaticParameTdf			LedStaticParame;
	stLedDynamicParameTdf 		LedDynamicParame;
	
}
stLedDeviceParameTdf;

void vLedDeviceInit(stLedStaticParameTdf *pstInit, enumLedDeviceNumTdf emDeviceNum);
const stLedDeviceParameTdf *c_pstGetLedDeviceParame(enumLedDeviceNumTdf emDeviceNum);
void vLedOn(enumLedDeviceNumTdf emDeviceNum);
void vLedOff(enumLedDeviceNumTdf emDeviceNum);
void vLedToggle(enumLedDeviceNumTdf emDeviceNum);


#endif

/**
  ******************************************************************************
  * @file    bsp_key.h
  * @brief   KeyBSP层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 本文件提供Key硬件抽象层接口，供中间件和应用层调用
  ******************************************************************************
  */
  
#ifndef __BSP_KEY__H
#define __BSP_KEY__H

#include "bsp_config.h"

#include "main.h"

/**	@brief 				Key设备号枚举
 * 	@note
 *
 **/
typedef enum   
{

	emKeyDeviceNum0		 	= 		0,//Key设备号
	emKeyDeviceNum1,				//1
	emKeyDeviceNum2,				//2
	emKeyDeviceNum3,
	
}
enumKeyDeviceNumTdf;


/**	@brief 				Key状态枚举
 * 	@note
 *
 **/
typedef enum   
{
	emKeystatusOff		 = 0,
	emKeystatusOn	,
}
enumKeystatusTdf;


/**	@brief 				Key按下电平枚举
 * 	@note
 *
 **/
typedef enum   
{

	emKeyOnLevelLow		 = 0,
	emKeyOnLevelHigh,
	
}
enumKeyOnLevelTdf;

/**	@brief 				Key标志电平枚举
 * 	@note
 *
 **/
typedef enum   
{

	emKeyFlag_Reset		 = 0,	//标志复位(0)
	emKeyFlag_Set,				//标志置位(1)
	
}
enumKeyFlagTdf;

/**	@brief 				Key静态参数结构体定义
 * 	@note
 *
 **/
typedef struct  
{

	GPIO_TypeDef 					*pstGPIOBase;				//使用什么GPIOx
	uint16_t 							u16GPIOPin;					//使用的GPIOPinx
	enumKeyOnLevelTdf			emKeyLevel;					//Key按下的电平
	
}
stKeyStaticParameTdf;

/**	@brief 				Key动态参数结构体定义
 * 	@note
 *
 **/
typedef struct  
{
	enumKeystatusTdf			emKeystatus;			//Key当前稳定状态(消抖后)
	enumKeyFlagTdf				emDebouncePending;	//消抖等待标志
	uint32_t					u32DebounceTick;		//消抖计时起点(ms)
	enumKeyFlagTdf				emPressEvent;			//按下事件标志
	enumKeyFlagTdf				emReleaseEvent;			//释放事件标志
	
}
stKeyDynamicParameTdf;


/**	@brief 				Key参数结构体定义
 * 	@note
 *
 **/
typedef struct  
{
	stKeyStaticParameTdf			KeyStaticParame;
	stKeyDynamicParameTdf 		KeyDynamicParame;
	
}
stKeyDeviceParameTdf;

/**	@brief 				函数外部声明
 * 	@note
 *
 **/

void vKeyDeviceInit(stKeyStaticParameTdf *pstInit, enumKeyDeviceNumTdf emDeviceNum);
const stKeyDeviceParameTdf *c_pstGetKeyDeviceParame(enumKeyDeviceNumTdf emDeviceNum);

void vKeyScan(void);
uint8_t u8KeyIsPressed(enumKeyDeviceNumTdf emDeviceNum);
uint8_t u8KeyGetPressEvent(enumKeyDeviceNumTdf emDeviceNum);
uint8_t u8KeyGetReleaseEvent(enumKeyDeviceNumTdf emDeviceNum);


#endif

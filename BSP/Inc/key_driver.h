/**
 * @file					key_driver.h
 * @author 					可以航行
 * @version 				0.1
 * @date 					2026/8/20
 * @brief 					Key驱动，使用HAL库
 *
 **/

#ifndef __KEY_DRIVER_H
#define __KEY_DRIVER_H

#include "top_config.h"

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
	enumKeystatusTdf			emKeystatus;				//Key当前稳定状态(消抖后)
	uint8_t							u8DebouncePending;		//消抖等待标志
	uint32_t						u32DebounceTick;		//消抖计时起点(ms)
	uint8_t							u8PressEvent;			//按下事件标志
	uint8_t							u8ReleaseEvent;			//释放事件标志
	
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

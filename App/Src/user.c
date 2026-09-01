/**
 * @file				user.c
 * @author 				可以航行
 * @version 			0.0.1
 * @date 				2026/8/19
 * @brief 				用户代码执行
 * */

#include "led_driver.h"
#include "key_driver.h"
#include "user.h"
#include "top_config.h"



void vLedInit()
{
	stLedStaticParameTdf stInit;
	stInit.	pstGPIOBase  =  LED_GPIO_Port;
	stInit. u16GPIOPin   =  LED_Pin;
	stInit. emOnLevel    =  emLedOnLevel_Low;
	 vLedDeviceInit( &stInit,emLedDeviceNum0);
	
	stInit.	pstGPIOBase  =  PWNEN_GPIO_Port;
	stInit. u16GPIOPin   =  PWNEN_Pin;
	stInit. emOnLevel    =  emLedOnLevel_High;
	 vLedDeviceInit( &stInit,emLedDeviceNum1);
}

void vKeyInit()
{
	stKeyStaticParameTdf stInit;
	stInit.	pstGPIOBase  =  KEY_GPIO_Port;
	stInit. u16GPIOPin   =  KEY_Pin;
	stInit. emKeyLevel   =  emKeyOnLevel_Low;	//KEY按下为低电平(内部上拉)
	 vKeyDeviceInit( &stInit,emKeyDeviceNum0);
}


void vUserInit()
{

	/* 1. 各外设驱动注册与初始化 */
	vLedInit();
	vKeyInit();
	


}






void vUserExecute()
{
	static uint32_t u32LastTick  = 0;

	/* 1. 轮询任务: 按键扫描、串口帧处理 */
	vKeyScan();


	/* 2. 500ms状态指示: LED闪烁指示系统运行 */
	if ((HAL_GetTick() - u32LastTick) >= 500)
	{
		u32LastTick = HAL_GetTick();
		vLedToggle(LED);
	}
}

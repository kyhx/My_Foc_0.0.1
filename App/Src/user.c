/***
 * @file				user.c
 * @author 				可以航行
 * @version 			0.0.1
 * @date 				2026/8/19
 * @brief 				用户代码执行
 * */

#include "motor.h"
#include "user.h"
#include "top_config.h"
#include "bsp_uart.h"
#include "oscilloscope.h"


void vMotorSetup()
{
	stMotorStaticParameTdf stInit;
	stInit. fAlignDuty     = (float)ZA_DUTY_PERMILLE / 1000.0f;	//零位对齐注入占空比(0~1)
	stInit. u32LockMs      = ZA_LOCK_MS;							//转子锁定稳定等待(ms)
	stInit. u32CalTimeoutMs = ZA_CAL_TIMEOUT_MS;					//电流零偏校准超时(ms)
	stInit. u8Enable       = FOC_OL_ENABLE;							//1=执行零位对齐+电流校正
	stInit. fAutoSpeedRadS = OL_ELEC_SPEED_RAD_S;					//开环自动生成电角度速度(rad/s)
	stInit. emAngleSrc     = emMotorAngleSrc_Auto;					//默认电角度来源: 开环自动生成
	stInit. fOlUdV         = OL_UD_V;								//开环d轴电压指令(V)
	stInit. fOlUqV         = OL_UQ_V;								//开环q轴电压指令(V)
	vMotorInit(&stInit, MOTOR);
}







void vUserInit()
{

	/** 1. 各外设驱动注册与初始化 */
	vLedInit();
	vKeyInit();
	vAS5047PInit();
	vDRV8313Init();
	vPwmInit();
	vUartInit();
	
	/** 示波器应用初始化 */
	OSc_Init();

	// /** 电机初始化: 电流零偏校正 + 转子零位对齐(状态机由vMotorInitTask推进) */
	// vMotorSetup();



}








void vUserExecute()
{
	static uint32_t u32LastTick  = 0;

	/** 3. 500ms状态指示: LED闪烁指示系统运行 */
	if ((HAL_GetTick() - u32LastTick) >= 500)
	{
		u32LastTick = HAL_GetTick();
		vLedToggle(LED);
	}
}

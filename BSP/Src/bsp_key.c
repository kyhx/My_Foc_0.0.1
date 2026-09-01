/**
  ******************************************************************************
  * @file    bsp_key.c
  * @brief   KeyBSP层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 本文件实现Key硬件抽象层，封装HAL库操作
  ******************************************************************************
  */

#include	"bsp_key.h"
#include	"bsp_config.h"


//【0】Key设备。
stKeyDeviceParameTdf arrystKeyDeviceparam[KEY_DEVICE_NUM];

/**
 * @brief 								Key参数结构体初始化
 * @param		pstInit					 Key参数结构体地址
 * @param		emDeviceNum		 		Key设备号
 *
 **/
void vKeyDeviceInit(stKeyStaticParameTdf *pstInit, enumKeyDeviceNumTdf emDeviceNum)
{
	arrystKeyDeviceparam[emDeviceNum].KeyStaticParame.pstGPIOBase = pstInit->pstGPIOBase;
	arrystKeyDeviceparam[emDeviceNum].KeyStaticParame.u16GPIOPin = pstInit->u16GPIOPin;
	arrystKeyDeviceparam[emDeviceNum].KeyStaticParame.emKeyLevel = pstInit->emKeyLevel;
	/* 动态参数复位 */
	arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emKeystatus = emKeystatusOff;
	arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emDebouncePending = emKeyFlag_Reset;
	arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.u32DebounceTick = 0;
	arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emPressEvent = emKeyFlag_Reset;
	arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emReleaseEvent = emKeyFlag_Reset;
}
/**
 * @brief 								取得Key设备参数
 * @param		emenDeviceNum		 	Key设备号
 *
 **/
const stKeyDeviceParameTdf *c_pstGetKeyDeviceParame(enumKeyDeviceNumTdf emDeviceNum)
{
	return &arrystKeyDeviceparam[emDeviceNum];

}

/**
 * 	@brief 									按键扫描
 * 	@note									需在程序主循环中周期调用。
 * 											内部采用时间戳方式消抖,不阻塞程序,
 * 											消抖时间由顶层参数KEY_DEBOUNCE_TIME配置。
 * 											状态稳定切换时置起按下/释放事件标志。
 **/
void vKeyScan(void)
{
	stKeyDeviceParameTdf *pstDev;
	uint32_t u32NowTick;
	uint8_t u8RawPressed;
	uint8_t u8StablePressed;
	uint8_t u8Index;

	u32NowTick = HAL_GetTick();

	for (u8Index = 0; u8Index < KEY_DEVICE_NUM; u8Index++)
	{
		pstDev = &arrystKeyDeviceparam[u8Index];

		/* 读取引脚原始电平,并换算为是否按下(按下电平可配置) */
		u8RawPressed = (uint8_t)(HAL_GPIO_ReadPin(
						pstDev->KeyStaticParame.pstGPIOBase,
						pstDev->KeyStaticParame.u16GPIOPin)
						== (GPIO_PinState)pstDev->KeyStaticParame.emKeyLevel);

		/* 当前稳定状态换算为是否按下 */
		u8StablePressed = (uint8_t)(pstDev->KeyDynamicParame.emKeystatus == emKeystatusOn);

		if (u8RawPressed == u8StablePressed)
		{
			/* 原始状态与稳定状态一致,清除消抖等待 */
			pstDev->KeyDynamicParame.emDebouncePending = emKeyFlag_Reset;
		}
		else
		{
			/* 原始状态与稳定状态不一致,进入消抖等待 */
			if (pstDev->KeyDynamicParame.emDebouncePending == emKeyFlag_Reset)
			{
				pstDev->KeyDynamicParame.emDebouncePending = emKeyFlag_Set;
				pstDev->KeyDynamicParame.u32DebounceTick = u32NowTick;
			}
			else if ((u32NowTick - pstDev->KeyDynamicParame.u32DebounceTick)
						>= KEY_DEBOUNCE_TIME)
			{
				/* 消抖时间到,确认状态切换,并置起对应事件标志 */
				pstDev->KeyDynamicParame.emDebouncePending = emKeyFlag_Reset;
				if (u8RawPressed != 0)
				{
					pstDev->KeyDynamicParame.emKeystatus = emKeystatusOn;
					pstDev->KeyDynamicParame.emPressEvent = emKeyFlag_Set;
				}
				else
				{
					pstDev->KeyDynamicParame.emKeystatus = emKeystatusOff;
					pstDev->KeyDynamicParame.emReleaseEvent = emKeyFlag_Set;
				}
			}
		}
	}
}
/**
 * 	@brief 								查询Key当前是否按下
 * 	@param		emDeviceNum	 			Key设备号
 * 	@retval								1-按下,0-未按下(消抖后稳定状态)
 *
 **/
uint8_t u8KeyIsPressed(enumKeyDeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= KEY_DEVICE_NUM)
	{
		return 0;
	}
	return (uint8_t)(arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emKeystatus == emKeystatusOn);
}
/**
 * 	@brief 								查询Key按下事件
 * 	@param		emDeviceNum	 			Key设备号
 * 	@retval								1-发生过按下事件,0-无
 * 	@note								查询后自动清除事件标志
 *
 **/
uint8_t u8KeyGetPressEvent(enumKeyDeviceNumTdf emDeviceNum)
{
	uint8_t u8Event;

	if ((uint8_t)emDeviceNum >= KEY_DEVICE_NUM)
	{
		return 0;
	}
	u8Event = (uint8_t)arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emPressEvent;
	arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emPressEvent = emKeyFlag_Reset;
	return u8Event;
}
/** 
 * 	@brief 								查询Key释放事件
 * 	@param		emDeviceNum	 			Key设备号
 * 	@retval								1-发生过释放事件,0-无
 * 	@note								查询后自动清除事件标志
 *
 **/
uint8_t u8KeyGetReleaseEvent(enumKeyDeviceNumTdf emDeviceNum)
{
	uint8_t u8Event;

	if ((uint8_t)emDeviceNum >= KEY_DEVICE_NUM)
	{
		return 0;
	}
	u8Event = (uint8_t)arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emReleaseEvent;
	arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.emReleaseEvent = emKeyFlag_Reset;
	return u8Event;
}

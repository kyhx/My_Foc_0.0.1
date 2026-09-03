/**
  ******************************************************************************
  * @file    bsp_DRV8313.h
  * @brief   DRV8313BSP层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 本文件提供DRV8313硬件抽象层接口，供中间件和应用层调用
  *		- PB11读取DRV8313错误状态(FAULT,低有效)。
  *  	- PA11控制DRV8313输出使能(高有效)。
  *  	- ADC1 CH1~CH3经运放采样三相电流(10mΩ采样电阻,增益16.5,参考1.65V),
  *    由注入组在PWM峰/谷(TIM1 TRGO)同步触发,用于电流环。
  *  	- ADC1 CH4采样母线电压,由普通组+DMA循环传输读取。
  ******************************************************************************
  */
  
#ifndef __BSP_DRV8313__H
#define __BSP_DRV8313__H

#include "bsp_config.h"
#include "main.h"

/**	@brief 			DRV8313常量
 * 	@note
 **/
#define DRV8313_REGULAR_CH_NUM		4	//普通组转换通道数(CH1~CH4)
#define DRV8313_BUSV_DMA_IDX		3	//母线电压(CH4)在普通组DMA缓冲中的下标(rank4)
#define DRV8313_ADC_MAX				4095	//ADC 12位满量程
#define DRV8313_CAL_NUM				5000	//电流零偏校准采样次数(约0.5s@10kHz)
#define DRV8313_CAL_ALPHA			0.998f	//校准低通滤波系数(历史权重)
#define DRV8313_CAL_ALPHA_INV		0.002f	//校准低通滤波系数(新样本权重)

/**	@brief 				DRV8313设备号枚举
 * 	@note
 *
 **/
typedef enum
{
	emDRV8313DeviceNum0 = 0,	//DRV8313设备0
}
enumDRV8313DeviceNumTdf;

/**	@brief 				DRV8313，PWNEN使能状态枚举
 * 	@note				PA11高有效: 1=使能, 0=关断
 *
 **/
typedef enum
{
	emPwnenOff = 0,		//关断
	emPwnenOn,			//使能
}
enumPwnenStatusTdf;

/**	@brief 				DRV8313，错误状态枚举
 * 	@note				FAULT(PB11)低有效: 过温、过流、欠压等故障时拉低
 *
 **/
typedef enum
{
	emErrorTrue = 0,	//有故障
	emErrorFalse,		//正常
}
enumErrorStatusTdf;

/**	@brief 				DRV8313，标志电平枚举
 * 	@note
 *
 **/
typedef enum
{
	emDRV8313Flag_Reset = 0,	//标志复位(0)
	emDRV8313Flag_Set,			//标志置位(1)
}
enumDRV8313FlagTdf;

/**	@brief 				DRV8313，电流零偏校准状态枚举
 * 	@note
 *
 **/
typedef enum
{
	emDRV8313Cal_NotStart = 0,	//未开始
	emDRV8313Cal_InProgress,	//校准中
	emDRV8313Cal_Done,			//完成
}
enumDRV8313CalStateTdf;

/**	@brief 				DRV8313静态参数结构体定义
 * 	@note
 *
 **/
typedef struct
{
	ADC_HandleTypeDef 			*pstADCHandle;		//HAL库ADC句柄(hadc1)
	GPIO_TypeDef 				*pstPwnenGpio;		//使能引脚GPIOx(PA11,高有效)
	uint16_t 					u16PwnenPin;		//使能引脚对应的GPIOPin
	GPIO_TypeDef 				*pstErrorGpio;		//错误引脚GPIOx(PB11,低有效)
	uint16_t 					u16ErrorPin;		//错误引脚对应的GPIOPin
}
stDRV8313StaticParameTdf;

/**	@brief 				DRV8313动态参数结构体定义
 * 	@note
 *
 **/
typedef struct
{
	uint16_t 					au16CurrentRaw[3];	//三相电流原始值(注入组JLDR1~3: Ia/Ib/Ic)
	volatile float 				fCurrentA[3];		//三相电流(A) Ia/Ib/Ic(ISR写入/主循环读)
	uint16_t 					u16BusVoltageRaw;	//母线电压原始值(普通组DMA,CH4)
	volatile float 				fBusVoltage;		//母线电压(V)(主循环更新/读取)
	enumPwnenStatusTdf 			emPwnen;			//使能状态
	enumErrorStatusTdf 			emError;			//错误状态
	volatile enumDRV8313FlagTdf 	emOcFlag;		//过流标志(任一相|I|超限)
	volatile enumDRV8313FlagTdf 	emInjDone;		//注入组转换完成标志(中断方式)
	volatile enumDRV8313CalStateTdf emCalState;		//电流零偏校准状态
	uint32_t 					u32CalCount;		//校准采样计数
	volatile float 				fCalOffsetA[3];		//三相电流零偏补偿(A)(ISR写入/主循环读)
}
stDRV8313DynamicParameTdf;

/**	@brief 				DRV8313设备结构体定义
 * 	@note
 *
 **/
typedef struct
{
	stDRV8313StaticParameTdf 	DRV8313StaticParame;
	stDRV8313DynamicParameTdf 	DRV8313DynamicParame;
	uint16_t 					au16RegDmaBuf[DRV8313_REGULAR_CH_NUM];	//普通组DMA循环缓冲(CH1~CH4)
}
stDRV8313DeviceParameTdf;

/**	@brief 				函数外部声明
 * 	@note
 *
 **/
void 	vDRV8313DeviceInit(stDRV8313StaticParameTdf *pstInit, enumDRV8313DeviceNumTdf emDeviceNum);
const 	stDRV8313DeviceParameTdf *c_pstGetDRV8313DeviceParame(enumDRV8313DeviceNumTdf emDeviceNum);

/* 使能与错误 */
void 	vDRV8313SetEnable(enumDRV8313DeviceNumTdf emDeviceNum, enumPwnenStatusTdf emStatus);
void 	vDRV8313Enable(enumDRV8313DeviceNumTdf emDeviceNum);
void 	vDRV8313Disable(enumDRV8313DeviceNumTdf emDeviceNum);
uint8_t u8DRV8313GetFault(enumDRV8313DeviceNumTdf emDeviceNum);		//返回1=有故障(FAULT低有效)

/* 电流(注入组,PWM同步触发) */
void 	vDRV8313StartInjected(enumDRV8313DeviceNumTdf emDeviceNum);	//启动注入组(中断方式)
void 	vDRV8313ReadCurrent(enumDRV8313DeviceNumTdf emDeviceNum);	//读取并换算三相电流
void 	vDRV8313CalibrateOffset(enumDRV8313DeviceNumTdf emDeviceNum);	//电流零偏校准(每采样调用)
uint8_t u8DRV8313GetCalState(enumDRV8313DeviceNumTdf emDeviceNum);	//校准状态: 2=完成
void 	HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc);	//注入组转换完成回调

/* PWM同步ISR钩子: 注入组每次转换完成(电流已更新)后调用注册的应用层回调(如FOC电流环) */
typedef void (*fpDRV8313IsrCb)(void);
void 	vDRV8313RegisterIsrCb(fpDRV8313IsrCb fpCb);	//注册PWM同步ISR钩子(NULL=取消)

/* 母线电压(普通组+DMA) */
void 	vDRV8313StartDMA(enumDRV8313DeviceNumTdf emDeviceNum);		//启动普通组DMA
void 	vDRV8313UpdateBusVoltage(enumDRV8313DeviceNumTdf emDeviceNum);	//读取母线电压

/* 通用读取接口 */
float 	fDRV8313GetCurrentA(enumDRV8313DeviceNumTdf emDeviceNum, uint8_t u8Phase);	//0=Ia 1=Ib 2=Ic
float 	fDRV8313GetBusVoltage(enumDRV8313DeviceNumTdf emDeviceNum);
uint8_t u8DRV8313GetOcFlag(enumDRV8313DeviceNumTdf emDeviceNum);
void 	vDRV8313ClearOcFlag(enumDRV8313DeviceNumTdf emDeviceNum);		//清除过流标志(锁存复位)

#endif


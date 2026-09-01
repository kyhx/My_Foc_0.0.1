/**
  ******************************************************************************
  * @file    bsp_DRV8313.c
  * @brief   DRV8313BSP层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 	本文件实现DRV8313硬件抽象层，封装HAL库操作
  *		电流换算: 三相PGND经10mΩ采样电阻+运放(增益16.5,参考1.65V)送ADC1 CH1~CH3。
  *  	 V = raw/4095*3.3V, I = (V-1.65)*ADC_CUR_GAIN, ADC_CUR_GAIN=1/(0.01*16.5)。
  * 	母线电压: ADC1 CH4分压采样, Vbus = raw/4095*3.3*ADC_UDC_GAIN。
  ******************************************************************************
  */

#include	"bsp_DRV8313.h"
#include	"bsp_config.h"

//【0】DRV8313设备。
stDRV8313DeviceParameTdf arrystDRV8313Deviceparam[DRV8313_DEVICE_NUM];

/**
 * @brief 										DRV8313设备初始化
 * @param		pstInit					 	DRV8313静态参数结构体地址
 * @param		emDeviceNum		 		DRV8313设备号
 * @note											PB11=FAULT(低有效), PA11=使能(高有效)。
 *
 **/
void vDRV8313DeviceInit(stDRV8313StaticParameTdf *pstInit, enumDRV8313DeviceNumTdf emDeviceNum)
{
	stDRV8313DeviceParameTdf 	*pstDev = &arrystDRV8313Deviceparam[emDeviceNum];
	uint8_t 					u8Idx;

	pstDev->DRV8313StaticParame.pstADCHandle = pstInit->pstADCHandle;
	pstDev->DRV8313StaticParame.pstPwnenGpio = pstInit->pstPwnenGpio;
	pstDev->DRV8313StaticParame.u16PwnenPin  = pstInit->u16PwnenPin;
	pstDev->DRV8313StaticParame.pstErrorGpio = pstInit->pstErrorGpio;
	pstDev->DRV8313StaticParame.u16ErrorPin  = pstInit->u16ErrorPin;

	/** 动态参数复位 */
	pstDev->DRV8313DynamicParame.emPwnen      = emPwnenOff;
	pstDev->DRV8313DynamicParame.emError      = emErrorFalse;
	pstDev->DRV8313DynamicParame.emOcFlag     = emDRV8313Flag_Reset;
	pstDev->DRV8313DynamicParame.emInjDone    = emDRV8313Flag_Reset;
	pstDev->DRV8313DynamicParame.emCalState   = emDRV8313Cal_NotStart;
	pstDev->DRV8313DynamicParame.u32CalCount  = 0;
	pstDev->DRV8313DynamicParame.u16BusVoltageRaw = 0;
	pstDev->DRV8313DynamicParame.fBusVoltage  = 0.0f;
	for (u8Idx = 0; u8Idx < 3; u8Idx++)
	{
		pstDev->DRV8313DynamicParame.au16CurrentRaw[u8Idx] = 0;
		pstDev->DRV8313DynamicParame.fCurrentA[u8Idx]      = 0.0f;
		pstDev->DRV8313DynamicParame.fCalOffsetA[u8Idx]    = 0.0f;
	}

	/** 普通组DMA缓冲清零 */
	for (u8Idx = 0; u8Idx < DRV8313_REGULAR_CH_NUM; u8Idx++)
	{
		pstDev->au16RegDmaBuf[u8Idx] = 0;
	}
}
/**
 * @brief 										取得DRV8313设备参数
 * @param		emDeviceNum		 	DRV8313设备号
 *
 **/
const stDRV8313DeviceParameTdf *c_pstGetDRV8313DeviceParame(enumDRV8313DeviceNumTdf emDeviceNum)
{
	return &arrystDRV8313Deviceparam[emDeviceNum];
}
/**
 * 	@brief 										设置DRV8313输出使能
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@param		emStatus		 	emPwnenOn使能 / emPwnenOff关断
 * 	@note											PA11高有效。使能前请确保PWM已配置。
 *
 **/
void vDRV8313SetEnable(enumDRV8313DeviceNumTdf emDeviceNum, enumPwnenStatusTdf emStatus)
{
	stDRV8313DeviceParameTdf 	*pstDev = &arrystDRV8313Deviceparam[emDeviceNum];

	if (emStatus == emPwnenOn)
	{
		HAL_GPIO_WritePin(pstDev->DRV8313StaticParame.pstPwnenGpio,
							pstDev->DRV8313StaticParame.u16PwnenPin, GPIO_PIN_SET);
	}
	else
	{
		HAL_GPIO_WritePin(pstDev->DRV8313StaticParame.pstPwnenGpio,
							pstDev->DRV8313StaticParame.u16PwnenPin, GPIO_PIN_RESET);
	}
	pstDev->DRV8313DynamicParame.emPwnen = emStatus;
}
/**
 * 	@brief 										使能DRV8313输出
 * 	@param		emDeviceNum		 	DRV8313设备号
 *
 **/
void vDRV8313Enable(enumDRV8313DeviceNumTdf emDeviceNum)
{
	vDRV8313SetEnable(emDeviceNum, emPwnenOn);
}
/**
 * 	@brief 										关断DRV8313输出
 * 	@param		emDeviceNum		 	DRV8313设备号
 *
 **/
void vDRV8313Disable(enumDRV8313DeviceNumTdf emDeviceNum)
{
	vDRV8313SetEnable(emDeviceNum, emPwnenOff);
}
/**
 * 	@brief 										读取DRV8313错误状态
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@retval										1=有故障, 0=正常
 * 	@note											FAULT(PB11)低有效: 过温/过流/欠压等故障时拉低。
 * 											同时更新动态参数emError。
 *
 **/
uint8_t u8DRV8313GetFault(enumDRV8313DeviceNumTdf emDeviceNum)
{
	stDRV8313DeviceParameTdf 	*pstDev = &arrystDRV8313Deviceparam[emDeviceNum];
	uint8_t 					u8Fault;

	u8Fault = (HAL_GPIO_ReadPin(pstDev->DRV8313StaticParame.pstErrorGpio,
								pstDev->DRV8313StaticParame.u16ErrorPin) == GPIO_PIN_RESET) ? 1 : 0;
	pstDev->DRV8313DynamicParame.emError = (u8Fault != 0) ? emErrorTrue : emErrorFalse;
	return u8Fault;
}
/**
 * 	@brief 										启动注入组转换(中断方式)
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@note											注入组由TIM1 TRGO在PWM峰/谷同步触发(CH1~CH4),
 * 											转换完成进入HAL_ADCEx_InjectedConvCpltCallback自动读取电流。
 * 											需保证TIM1 PWM已启动。
 *
 **/
void vDRV8313StartInjected(enumDRV8313DeviceNumTdf emDeviceNum)
{
	stDRV8313DeviceParameTdf 	*pstDev = &arrystDRV8313Deviceparam[emDeviceNum];

	HAL_ADCEx_InjectedStart_IT(pstDev->DRV8313StaticParame.pstADCHandle);
}
/**
 * 	@brief 										读取并换算三相电流
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@note											读取注入组JLDR1~3(CH1~CH3),按运放增益换算为电流(A),
 * 											并做过流判断。可在注入组中断回调或主循环调用。
 *
 **/
void vDRV8313ReadCurrent(enumDRV8313DeviceNumTdf emDeviceNum)
{
	stDRV8313DeviceParameTdf 	*pstDev = &arrystDRV8313Deviceparam[emDeviceNum];
	ADC_HandleTypeDef 			*pstAdc = pstDev->DRV8313StaticParame.pstADCHandle;
	static const uint32_t 		kau32InjRank[3] = {ADC_INJECTED_RANK_1, ADC_INJECTED_RANK_2, ADC_INJECTED_RANK_3};
	uint8_t 					u8Phase;
	uint16_t 					u16Raw;
	float 						fVolt;
	float 						fCur;

	for (u8Phase = 0; u8Phase < 3; u8Phase++)
	{
		u16Raw = (uint16_t)HAL_ADCEx_InjectedGetValue(pstAdc, kau32InjRank[u8Phase]);
		pstDev->DRV8313DynamicParame.au16CurrentRaw[u8Phase] = u16Raw;

		/** 采样电压→电流: V=(raw/4095)*3.3, I=(V-1.65)*增益 */
		fVolt = (float)u16Raw * ADC_ADC_REF / (float)DRV8313_ADC_MAX;
		fCur  = (fVolt - ADC_REFER) * ADC_CUR_GAIN;
		pstDev->DRV8313DynamicParame.fCurrentA[u8Phase] = fCur;

		/** 过流判断 */
		if ((fCur > ADC_OC_CURRENT_A) || (fCur < -ADC_OC_CURRENT_A))
		{
			pstDev->DRV8313DynamicParame.emOcFlag = emDRV8313Flag_Set;
		}
	}

	/** 电流零偏校准(每次采样自动累加) */
	vDRV8313CalibrateOffset(emDeviceNum);
}
/**
 * 	@brief 										电流零偏校准
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@note											上电后桥臂关断(零电流)期间,对三相电流做低通滤波累加,
 * 											得到运放/ADC的零偏,用于补偿。状态: 0=未开始 1=校准中 2=完成。
 * 											仅在校准未完成且输出关断时累加(避免把真实电流当零偏)。
 * 											GetCurrent返回的电流已自动扣除该零偏。
 *
 **/
void vDRV8313CalibrateOffset(enumDRV8313DeviceNumTdf emDeviceNum)
{
	stDRV8313DeviceParameTdf 	*pstDev = &arrystDRV8313Deviceparam[emDeviceNum];
	uint8_t 					u8Phase;

	/** 校准未完成 且 输出关断(保证零电流采样) 才累加 */
	if (pstDev->DRV8313DynamicParame.emCalState == emDRV8313Cal_Done)
	{
		return;
	}
	if (pstDev->DRV8313DynamicParame.emPwnen != emPwnenOff)
	{
		return;
	}

	if (pstDev->DRV8313DynamicParame.u32CalCount < DRV8313_CAL_NUM)
	{
		/** 校准中: 低通滤波累加零偏 offset = offset*0.998 + I*0.002 */
		pstDev->DRV8313DynamicParame.emCalState = emDRV8313Cal_InProgress;
		pstDev->DRV8313DynamicParame.u32CalCount++;
		for (u8Phase = 0; u8Phase < 3; u8Phase++)
		{
			pstDev->DRV8313DynamicParame.fCalOffsetA[u8Phase] =
					pstDev->DRV8313DynamicParame.fCalOffsetA[u8Phase] * DRV8313_CAL_ALPHA
					+ pstDev->DRV8313DynamicParame.fCurrentA[u8Phase] * DRV8313_CAL_ALPHA_INV;
		}
	}
	else
	{
		/** 采样次数到,校准完成 */
		pstDev->DRV8313DynamicParame.emCalState  = emDRV8313Cal_Done;
		pstDev->DRV8313DynamicParame.u32CalCount = 0;
	}
}
/**
 * 	@brief 										获取电流零偏校准状态
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@retval										0=未开始 1=校准中 2=完成
 *
 **/
uint8_t u8DRV8313GetCalState(enumDRV8313DeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= DRV8313_DEVICE_NUM)
	{
		return 0;
	}
	return (uint8_t)arrystDRV8313Deviceparam[emDeviceNum].DRV8313DynamicParame.emCalState;
}
/**
 * 	@brief 										注入组转换完成回调(HAL库调用)
 * 	@param		hadc					 	完成的ADC句柄
 * 	@note											每个PWM周期注入组转换完成时触发,自动读取三相电流。
 *
 **/
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	enumDRV8313DeviceNumTdf 	emDev;
	stDRV8313DeviceParameTdf 	*pstDev;

	/** 遍历所有设备,按 ADC 句柄匹配,支持多实例 */
	for (emDev = (enumDRV8313DeviceNumTdf)0; (uint8_t)emDev < DRV8313_DEVICE_NUM; emDev = (enumDRV8313DeviceNumTdf)((uint8_t)emDev + 1u))
	{
		pstDev = &arrystDRV8313Deviceparam[emDev];
		if (hadc == pstDev->DRV8313StaticParame.pstADCHandle)
		{
			pstDev->DRV8313DynamicParame.emInjDone = emDRV8313Flag_Set;
			vDRV8313ReadCurrent(emDev);
			break;
		}
	}
}
/**
 * 	@brief 										启动普通组DMA转换
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@note											普通组(CH1~CH4)连续转换+DMA循环传输,
 * 											母线电压(CH4)在缓冲下标DRV8313_BUSV_DMA_IDX。
 *
 **/
void vDRV8313StartDMA(enumDRV8313DeviceNumTdf emDeviceNum)
{
	stDRV8313DeviceParameTdf 	*pstDev = &arrystDRV8313Deviceparam[emDeviceNum];

	HAL_ADC_Start_DMA(pstDev->DRV8313StaticParame.pstADCHandle,
						(uint32_t *)pstDev->au16RegDmaBuf, DRV8313_REGULAR_CH_NUM);
}
/**
 * 	@brief 										读取母线电压
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@note											从普通组DMA循环缓冲取CH4原始值,按分压比换算为母线电压(V)。
 *
 **/
void vDRV8313UpdateBusVoltage(enumDRV8313DeviceNumTdf emDeviceNum)
{
	stDRV8313DeviceParameTdf 	*pstDev = &arrystDRV8313Deviceparam[emDeviceNum];
	uint16_t 					u16Raw;

	u16Raw = pstDev->au16RegDmaBuf[DRV8313_BUSV_DMA_IDX];
	pstDev->DRV8313DynamicParame.u16BusVoltageRaw = u16Raw;
	pstDev->DRV8313DynamicParame.fBusVoltage = (float)u16Raw * ADC_ADC_REF / (float)DRV8313_ADC_MAX * ADC_UDC_GAIN;
}
/**
 * 	@brief 										获取某相电流
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@param		u8Phase			 	0=Ia, 1=Ib, 2=Ic
 * 	@retval										相电流(A)
 *
 **/
float fDRV8313GetCurrentA(enumDRV8313DeviceNumTdf emDeviceNum, uint8_t u8Phase)
{
	stDRV8313DeviceParameTdf 	*pstDev;

	if (((uint8_t)emDeviceNum >= DRV8313_DEVICE_NUM) || (u8Phase > 2))
	{
		return 0.0f;
	}
	pstDev = &arrystDRV8313Deviceparam[emDeviceNum];
	/** 返回扣除零偏补偿后的电流 */
	return pstDev->DRV8313DynamicParame.fCurrentA[u8Phase]
			- pstDev->DRV8313DynamicParame.fCalOffsetA[u8Phase];
}
/**
 * 	@brief 										获取母线电压
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@retval										母线电压(V)
 *
 **/
float fDRV8313GetBusVoltage(enumDRV8313DeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= DRV8313_DEVICE_NUM)
	{
		return 0.0f;
	}
	return arrystDRV8313Deviceparam[emDeviceNum].DRV8313DynamicParame.fBusVoltage;
}
/**
 * 	@brief 										获取过流标志
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@retval										1=发生过流, 0=正常
 *
 **/
uint8_t u8DRV8313GetOcFlag(enumDRV8313DeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= DRV8313_DEVICE_NUM)
	{
		return 0;
	}
	return (uint8_t)arrystDRV8313Deviceparam[emDeviceNum].DRV8313DynamicParame.emOcFlag;
}
/**
 * 	@brief 										清除过流标志
 * 	@param		emDeviceNum		 	DRV8313设备号
 * 	@note											过流标志为锁存型,需检测后主动复位,下次未再触发才为0。
 *
 * */
void vDRV8313ClearOcFlag(enumDRV8313DeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum < DRV8313_DEVICE_NUM)
	{
		arrystDRV8313Deviceparam[emDeviceNum].DRV8313DynamicParame.emOcFlag = emDRV8313Flag_Reset;
	}
}



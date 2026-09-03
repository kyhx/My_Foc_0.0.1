/**
  ******************************************************************************
  * @file    bsp_AS5047.c
  * @brief   AS5047BSP层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 	本文件实现AS5047硬件抽象层，封装HAL库操作
  *		AS5047P驱动代码(普通阻塞模式 + DMA非阻塞模式)
  ******************************************************************************
  */

#include	"bsp_AS5047.h"
#include	"bsp_config.h"


//【0】AS5047P设备。
stAS5047PDeviceParameTdf arrystAS5047PDeviceparam[AS5047P_DEVICE_NUM];


/**
 * @brief 										计算AS5047命令帧偶校验位
 * @param		u16CommandFrame	 	待计算的命令15位数据(低15位)
 * @retval										带偶校验位的完整16位命令帧
 * @note											AS5047要求命令帧偶校验:bit15=1使得1的总数为偶数。
 *
 * */
static uint16_t u16SpiCalcEvenParity(uint16_t u16CommandFrame)
{
	uint8_t 	u8Count = 0;
	uint16_t	u16Value = u16CommandFrame;

	/** 统计1的个数 */
	while (u16Value != 0)
	{
		u8Count ^= (uint8_t)(u16Value & 1U);
		u16Value >>= 1;
	}
	/** 若低15位1的个数为奇数,置bit15为1使其为偶数 */
	if (u8Count != 0)
	{
		u16CommandFrame |= SPI_AS5047P_PARITY_BIT;
	}
	return u16CommandFrame;
}
/**
 * 	@brief 										片选使能(拉低CS,低有效)
 * 	@param		emDeviceNum		 	AS5047P设备号
 *
 * */
static void vSpiCsLow(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	HAL_GPIO_WritePin(pstDev->AS5047PStaticParame.pstCsGpioBase,
						pstDev->AS5047PStaticParame.u16CsPin, GPIO_PIN_RESET);
}
/**
 * 	@brief 										片选释放(拉高CS,低有效)
 * 	@param		emDeviceNum		 	AS5047P设备号
 *
 * */
static void vSpiCsHigh(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	HAL_GPIO_WritePin(pstDev->AS5047PStaticParame.pstCsGpioBase,
						pstDev->AS5047PStaticParame.u16CsPin, GPIO_PIN_SET);
}
/**
 * 	@brief 										SPI单帧发送并接收(16位,普通阻塞模式)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@param		u16Tx				发送命令帧
 * 	@param		pu16Rx				返回接收数据(可空)
 * 	@retval									HAL状态
 * 	@note											带超时保护,超时置错误标志。
 *
 * */
static HAL_StatusTypeDef enSpiTransfer(enumAS5047PDeviceNumTdf emDeviceNum, uint16_t u16Tx, uint16_t *pu16Rx)
{
	HAL_StatusTypeDef 	enStatus;
	uint16_t 			u16RxBuf = 0;
	stAS5047PDeviceParameTdf *pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	enStatus = HAL_SPI_TransmitReceive(pstDev->AS5047PStaticParame.pstSpiHandle,
							(uint8_t*)&u16Tx, (uint8_t*)&u16RxBuf, 1,
                            pstDev->AS5047PStaticParame.u16Timeout);

	if (enStatus != HAL_OK)
	{
		pstDev->AS5047PDynamicParame.emError = emAS5047PFlag_Set;
	}
	if (pu16Rx != NULL)
	{
		*pu16Rx = u16RxBuf;
	}
	return enStatus;
}


/**
 * @brief 										AS5047P设备初始化
 * @param		pstInit					 	SPI静态参数结构体地址
 * @param		emDeviceNum		 		AS5047P设备号
 * @note											emTransferMode选择传输方式:
 * 											- Polling: 普通阻塞模式,调用vAS5047PUpdate即可。
 * 											- DMA:     非阻塞模式,需配合HAL_SPI_TxRxCpltCallback。
 *
 * */
void vAS5047PDeviceInit(stAS5047PStaticParameTdf *pstInit, enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	pstDev->AS5047PStaticParame.pstSpiHandle   = pstInit->pstSpiHandle;
	pstDev->AS5047PStaticParame.pstCsGpioBase  = pstInit->pstCsGpioBase;
	pstDev->AS5047PStaticParame.u16CsPin       = pstInit->u16CsPin;
	pstDev->AS5047PStaticParame.emTransferMode = pstInit->emTransferMode;
	if (pstInit->u16Timeout == 0)
	{
		pstDev->AS5047PStaticParame.u16Timeout = 10;
	}
	else
	{
		pstDev->AS5047PStaticParame.u16Timeout = pstInit->u16Timeout;
	}
	/** 每圈码值(缺省16384) */
	if (pstInit->u32MaxCount == 0)
	{
		pstDev->AS5047PStaticParame.u32MaxCount = SPI_AS5047P_MAX_COUNT;
	}
	else
	{
		pstDev->AS5047PStaticParame.u32MaxCount = pstInit->u32MaxCount;
	}
	

	/** 动态参数复位 */
	pstDev->AS5047PDynamicParame.u16Angle     = 0;
	pstDev->AS5047PDynamicParame.u32AngleRaw  = 0;
	pstDev->AS5047PDynamicParame.emError      = emAS5047PFlag_Reset;
	pstDev->AS5047PDynamicParame.emBusy       = emAS5047PFlag_Reset;
	pstDev->AS5047PDynamicParame.emDmaDone    = emAS5047PFlag_Reset;
	pstDev->AS5047PDynamicParame.u16TxBuf     = 0;
	pstDev->AS5047PDynamicParame.u16RxBuf     = 0;
}
/**
 * @brief 										取得AS5047P设备参数
 * @param		emDeviceNum		 	AS5047P设备号
 *
 * */
const stAS5047PDeviceParameTdf *c_pstGetAS5047PDeviceParame(enumAS5047PDeviceNumTdf emDeviceNum)
{
	return &arrystAS5047PDeviceparam[emDeviceNum];
}
/**
 *  @brief      通用读AS5047寄存器(普通阻塞模式)
 *  @param      emDeviceNum     AS5047P设备号
 *  @param      u16RegAddr      寄存器地址(14位,如0x3FFF角度/0x3FFC诊断)
 *  @retval                     寄存器数据(14位)
 *  @note                       AS5047流水线:需要连续读两次才能获取当前值。
 *                              内部自动处理流水线延迟，返回有效的当前值。
 */
uint16_t u16AS5047PReadRegister(enumAS5047PDeviceNumTdf emDeviceNum, uint16_t u16RegAddr)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];
	uint16_t 					u16Cmd;
	uint16_t 					u16Rx1 = 0;
	uint16_t 					u16Rx2 = 0;
	uint8_t  					u8Error = 0;
	uint16_t 					u16Data;

	/** 构造读命令: bit14=R/W, bit13:0=地址, bit15=偶校验 */
	u16Cmd = (uint16_t)(SPI_AS5047P_READ_BIT | (u16RegAddr & SPI_AS5047P_ADDR_MASK));
	u16Cmd = u16SpiCalcEvenParity(u16Cmd);

	/** 帧1: 独立CS周期发送读命令,读取的是上次命令的残留数据。
	 * AS5047为流水线输出,每一帧都需独立的CS拉低→传输→拉高周期,
	 * 否则传感器内部状态机不推进,后续读到的仍是旧值(表现为只刷新一次)。 */
	vSpiCsLow(emDeviceNum);
	if (enSpiTransfer(emDeviceNum, u16Cmd, &u16Rx1) != HAL_OK)
	{
		vSpiCsHigh(emDeviceNum);
		pstDev->AS5047PDynamicParame.emError = emAS5047PFlag_Set;
		/** 传输失败: 返回缓存角度, 避免突变 */
		if (u16RegAddr == SPI_AS5047P_REG_ANGLE)
		{
			return pstDev->AS5047PDynamicParame.u16Angle;
		}
		return 0;
	}
	vSpiCsHigh(emDeviceNum);

	/** 帧2: 再次发相同命令(独立CS周期),此帧MISO返回帧1命令的有效响应 */
	vSpiCsLow(emDeviceNum);
	if (enSpiTransfer(emDeviceNum, u16Cmd, &u16Rx2) != HAL_OK)
	{
		vSpiCsHigh(emDeviceNum);
		pstDev->AS5047PDynamicParame.emError = emAS5047PFlag_Set;
		if (u16RegAddr == SPI_AS5047P_REG_ANGLE)
		{
			return pstDev->AS5047PDynamicParame.u16Angle;
		}
		return 0;
	}
	vSpiCsHigh(emDeviceNum);

	/** 使用第二次传输的有效数据 */

	/** 检查错误标志 (bit14: EF - 命令帧错误) */
	if (u16Rx2 & SPI_AS5047P_ERRFLAG_BIT)
	{
		u8Error = 1;
	}

	/** 提取有效数据 (bit13:0) */
	u16Data = u16Rx2 & SPI_AS5047P_ANGLE_MASK;

	/** 如果是角度寄存器，保存到缓存 */
	if (u16RegAddr == SPI_AS5047P_REG_ANGLE)
	{
		pstDev->AS5047PDynamicParame.u16Angle    = u16Data;
		pstDev->AS5047PDynamicParame.u32AngleRaw = u16Rx2;
	}

	/** 更新错误标志 */
	if (u8Error)
	{
		pstDev->AS5047PDynamicParame.emError = emAS5047PFlag_Set;
	}

	return u16Data;
}
/**
 * 	@brief 										读取AS5047角度(普通阻塞模式,双帧流水线)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										14位角度值(0~0x3FFF),对应0~360度
 *
 * */
uint16_t u16AS5047PReadAngle(enumAS5047PDeviceNumTdf emDeviceNum)
{
	return u16AS5047PReadRegister(emDeviceNum, SPI_AS5047P_REG_ANGLE);
}
/**
 * 	@brief 										启动DMA读角度(非阻塞,单帧)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@note											AS5047为流水线输出:本次发读角度命令,返回上一次命令的结果。
 * 											主循环/电流环周期调用 vAS5047PUpdate 或 vAS5047PDmaTask 即可持续更新。
 * 											传输完成后由 HAL_SPI_TxRxCpltCallback 解析角度、释放片选。
 * 											单帧16位,不阻塞CPU;若已有传输在进行则本次跳过。
 *
 * */
void vAS5047PReadAngleDMA(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];
	uint16_t 					u16Cmd;

	/** 已有传输进行中,跳过本次 */
	if (pstDev->AS5047PDynamicParame.emBusy != emAS5047PFlag_Reset)
	{
		return;
	}

	/** 构造读角度命令帧 */
	u16Cmd = (uint16_t)(SPI_AS5047P_READ_BIT | (SPI_AS5047P_REG_ANGLE & SPI_AS5047P_ADDR_MASK));
	u16Cmd = u16SpiCalcEvenParity(u16Cmd);

	pstDev->AS5047PDynamicParame.u16TxBuf   = u16Cmd;
	pstDev->AS5047PDynamicParame.u16RxBuf   = 0;
	pstDev->AS5047PDynamicParame.emDmaDone  = emAS5047PFlag_Reset;
	pstDev->AS5047PDynamicParame.emBusy     = emAS5047PFlag_Set;

	/** 拉低CS,启动DMA单帧收发(16位) */
	vSpiCsLow(emDeviceNum);
	if (HAL_SPI_TransmitReceive_DMA(pstDev->AS5047PStaticParame.pstSpiHandle,
									(uint8_t *)&pstDev->AS5047PDynamicParame.u16TxBuf,
									(uint8_t *)&pstDev->AS5047PDynamicParame.u16RxBuf, 1) != HAL_OK)
	{
		vSpiCsHigh(emDeviceNum);
		pstDev->AS5047PDynamicParame.emBusy  = emAS5047PFlag_Reset;
		pstDev->AS5047PDynamicParame.emError = emAS5047PFlag_Set;
	}
}
/**
 * 	@brief 										DMA传输完成回调(HAL库调用)
 * 	@param		hspi					 	完成的SPI句柄
 * 	@note											在DMA模式下由中断触发。释放片选并解析DMA接收到的角度帧。
 * 											本驱动仅使用单路AS5047P(设备0),直接匹配其SPI句柄。
 *
 * */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	enumAS5047PDeviceNumTdf 	emDev;
	stAS5047PDeviceParameTdf 	*pstDev;
	uint16_t 					u16Rx;

	/** 遍历所有设备,按 SPI 句柄匹配,支持多实例 */
	for (emDev = (enumAS5047PDeviceNumTdf)0; (uint8_t)emDev < AS5047P_DEVICE_NUM; emDev = (enumAS5047PDeviceNumTdf)((uint8_t)emDev + 1u))
	{
		pstDev = &arrystAS5047PDeviceparam[emDev];
		if (hspi != pstDev->AS5047PStaticParame.pstSpiHandle)
		{
			continue;
		}

		/** 传输完成,释放片选 */
		vSpiCsHigh(emDev);

		/** 解析DMA接收到的返回帧 */
		u16Rx = pstDev->AS5047PDynamicParame.u16RxBuf;
		if (u16Rx & SPI_AS5047P_ERRFLAG_BIT)
		{
			pstDev->AS5047PDynamicParame.emError = emAS5047PFlag_Set;
		}
		pstDev->AS5047PDynamicParame.u16Angle    = u16Rx & SPI_AS5047P_ANGLE_MASK;
		pstDev->AS5047PDynamicParame.u32AngleRaw = u16Rx;

		pstDev->AS5047PDynamicParame.emBusy    = emAS5047PFlag_Reset;
		pstDev->AS5047PDynamicParame.emDmaDone = emAS5047PFlag_Set;
		break;
	}
}
/**
 * 	@brief 										DMA任务(主循环周期调用)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@note											上一帧完成后(回调已解析),立即发出下一帧,维持流水线连续更新。
 * 											调用频率即编码器采样频率,通常放主循环或电流环。
 *
 * */
void vAS5047PDmaTask(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	if (pstDev->AS5047PDynamicParame.emBusy == emAS5047PFlag_Reset)
	{
		vAS5047PReadAngleDMA(emDeviceNum);
	}
}
/**
 * 	@brief 										更新AS5047角度(按配置的传输模式)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@note											DMA模式: 调用vAS5047PDmaTask非阻塞发帧;
 * 											普通模式: 阻塞双帧读取并缓存角度。传输失败保持旧角度。
 *
 * */
void vAS5047PUpdate(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];
	uint16_t 					u16Angle;
	
	if (pstDev->AS5047PStaticParame.emTransferMode == emAS5047PTransferMode_DMA)
	{
		vAS5047PDmaTask(emDeviceNum);
	}
	else
	{
		u16Angle = u16AS5047PReadRegister(emDeviceNum, SPI_AS5047P_REG_ANGLE);
		pstDev->AS5047PDynamicParame.u16Angle = u16Angle;
	}


	pstDev->AS5047PDynamicParame.fAngleRad = (pstDev->AS5047PDynamicParame.u16Angle & SPI_AS5047P_ANGLE_MASK) 
							* 6.2831855f / (float)pstDev->AS5047PStaticParame.u32MaxCount;
	pstDev->AS5047PDynamicParame.fAngleDeg = (pstDev->AS5047PDynamicParame.u16Angle & SPI_AS5047P_ANGLE_MASK)
							* 360.0f / (float)pstDev->AS5047PStaticParame.u32MaxCount;

	
}
/**
 * 	@brief 										获取最近一次读取的角度
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										14位角度值
 * 	@note											返回缓存的最近一次读取结果,不发起SPI通信。
 *
 * */
uint16_t u16AS5047PGetAngle(enumAS5047PDeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0;
	}
	return arrystAS5047PDeviceparam[emDeviceNum].AS5047PDynamicParame.u16Angle;
}
/**
 * 	@brief 										获取角度(弧度)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										角度值(弧度,0~2π)
 * 	@note											基于最近一次读取的14位角度换算。单圈14bit分辨率。
 *
 * */
float fAS5047PGetAngleRad(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev;

	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0.0f;
	}
		pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	return pstDev->AS5047PDynamicParame.fAngleRad;
}
/**
 * 	@brief 										获取角度(度)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										角度值(度,0~360)
 * 	@note											基于最近一次读取的14位角度换算。单圈14bit分辨率。
 *
 * */
float fAS5047PGetAngleDeg(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev;

	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0.0f;
	}
	pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	return pstDev->AS5047PDynamicParame.fAngleDeg;
}

/**
 * 	@brief 										获取错误标志
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										1-存在超时/命令帧错误,0-正常
 *
 * */
uint8_t u8AS5047PGetError(enumAS5047PDeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0;
	}
	return (uint8_t)arrystAS5047PDeviceparam[emDeviceNum].AS5047PDynamicParame.emError;
}
/**
 * 	@brief 										获取DMA传输忙标志
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										1-传输进行中,0-空闲
 *
 * */
uint8_t u8AS5047PIsBusy(enumAS5047PDeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0;
	}
	return (uint8_t)arrystAS5047PDeviceparam[emDeviceNum].AS5047PDynamicParame.emBusy;
}

/*******************************************************ENC编码器模式*****************************************************/
/*******************************************************ENC编码器模式*****************************************************/
/*******************************************************ENC编码器模式*****************************************************/

/**
 * @brief 										正交编码器(ABI)接口初始化
 * @param		pstInit					 	编码器静态参数结构体地址
 * @param		emDeviceNum		 		AS5047P设备号
 * @note											pstTimHandle指向编码器定时器(htim3,TIM_ENCODERMODE_TI1),
 * 											u32Cpr为每圈计数(ABI倍频后)。
 *
 * */

void vAS5047PEncDeviceInit(stAS5047PENCStaticParameTdf *pstInit, enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	pstDev->AS5047PENCStaticParame.pstTimHandle = pstInit->pstTimHandle;
	pstDev->AS5047PENCStaticParame.u32Cpr       = pstInit->u32Cpr;

	/** 动态参数复位 */
	pstDev->AS5047PENCDynamicParame.i32Count    = 0;
	pstDev->AS5047PENCDynamicParame.u16RawCount = 0;
	pstDev->AS5047PENCDynamicParame.i16Delta    = 0;
	pstDev->AS5047PENCDynamicParame.fAngleRad   = 0.0f;
	pstDev->AS5047PENCDynamicParame.fAngleDeg   = 0.0f;
	pstDev->AS5047PENCDynamicParame.emStarted   = emAS5047PFlag_Reset;
}
/**
 * 	@brief 										启动正交编码器接口
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@note											调用HAL_TIM_Encoder_Start启动TIM3编码器计数,
 * 											记录初始计数值。重复调用无效。
 *
 * */
void vAS5047PEncStart(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];
	TIM_HandleTypeDef 			*pstTim = pstDev->AS5047PENCStaticParame.pstTimHandle;

	if (pstDev->AS5047PENCDynamicParame.emStarted != emAS5047PFlag_Reset)
	{
		return;
	}

	HAL_TIM_Encoder_Start(pstTim, TIM_CHANNEL_1);

	pstDev->AS5047PENCDynamicParame.emStarted   = emAS5047PFlag_Set;
	pstDev->AS5047PENCDynamicParame.u16RawCount = (uint16_t)__HAL_TIM_GET_COUNTER(pstTim);
	pstDev->AS5047PENCDynamicParame.i32Count    = 0;
}
/**
 * 	@brief 										停止正交编码器接口
 * 	@param		emDeviceNum		 	AS5047P设备号
 *
 * */
void vAS5047PEncStop(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];

	HAL_TIM_Encoder_Stop(pstDev->AS5047PENCStaticParame.pstTimHandle, TIM_CHANNEL_1);
	pstDev->AS5047PENCDynamicParame.emStarted = emAS5047PFlag_Reset;
}
/**
 * 	@brief 										更新正交编码器角度
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@note											在主循环/电流环周期调用。读取TIM3当前16位计数,
 * 											与上次调用之差得本次增量,半步回绕修正(兼容正反转与16位回绕),
 * 											累加后换算单圈角度,并缓存本次增量(i16Delta)供上层测速。
 *
 * */
void vAS5047PEncUpdate(enumAS5047PDeviceNumTdf emDeviceNum)
{
	stAS5047PDeviceParameTdf 	*pstDev = &arrystAS5047PDeviceparam[emDeviceNum];
	TIM_HandleTypeDef 			*pstTim = pstDev->AS5047PENCStaticParame.pstTimHandle;
	uint16_t 					u16Count;
	uint16_t 					u16RawLast;			//上次调用时的16位原始计数(增量基准)
	int32_t 					i32Delta;
	int32_t 					i32AngleCount;
	float 						fAngle;
	uint32_t 					u32Cpr = pstDev->AS5047PENCStaticParame.u32Cpr;

	if (pstDev->AS5047PENCDynamicParame.emStarted == emAS5047PFlag_Reset)
	{
		return;
	}

	/** 读取当前16位计数,与上次调用之差得本次增量(±32768半步回绕修正,覆盖16位计数回绕)。
	 * 相邻差只反映本次实际位移,跨圈/跨回绕均无累计误差。 */
	u16RawLast = pstDev->AS5047PENCDynamicParame.u16RawCount;	//覆盖前先取旧值
	u16Count   = (uint16_t)__HAL_TIM_GET_COUNTER(pstTim);
	pstDev->AS5047PENCDynamicParame.u16RawCount = u16Count;

	i32Delta = (int32_t)u16Count - (int32_t)u16RawLast;
	if (i32Delta > 32768)
	{
		i32Delta -= 65536;
	}
	else if (i32Delta < -32768)
	{
		i32Delta += 65536;
	}
	pstDev->AS5047PENCDynamicParame.i16Delta = (int16_t)i32Delta;
	/** 累计计数(每拍只加一次真实增量) */
	pstDev->AS5047PENCDynamicParame.i32Count += i32Delta;

	/** 单圈角度 count % CPR */
	i32AngleCount = pstDev->AS5047PENCDynamicParame.i32Count % (int32_t)u32Cpr;
	if (i32AngleCount < 0)
	{
		i32AngleCount += (int32_t)u32Cpr;
	}

	fAngle = (float)i32AngleCount / (float)u32Cpr;		//3141592653589793238462643
	pstDev->AS5047PENCDynamicParame.fAngleRad = fAngle *  6.283185307179586f;
	pstDev->AS5047PENCDynamicParame.fAngleDeg = fAngle * 360.0f;
}
/**
 * 	@brief 										获取编码器累计计数
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										有符号累计计数(可多次旋转累加)
 *
 * */
int32_t i32AS5047PEncGetCount(enumAS5047PDeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0;
	}
	return arrystAS5047PDeviceparam[emDeviceNum].AS5047PENCDynamicParame.i32Count;
}
/**
 * 	@brief 										获取本次编码器增量(供上层测速)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										本次vAS5047PEncUpdate的计数增量(±CPR/2内)
 *
 * */
int16_t i16AS5047PEncGetDelta(enumAS5047PDeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0;
	}
	return arrystAS5047PDeviceparam[emDeviceNum].AS5047PENCDynamicParame.i16Delta;
}
/**
 * 	@brief 										获取编码器角度(弧度)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										单圈角度(弧度,0~2π)
 *
 * */
float fAS5047PEncGetAngleRad(enumAS5047PDeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0.0f;
	}
	return arrystAS5047PDeviceparam[emDeviceNum].AS5047PENCDynamicParame.fAngleRad;
}
/**
 * 	@brief 										获取编码器角度(度)
 * 	@param		emDeviceNum		 	AS5047P设备号
 * 	@retval										单圈角度(度,0~360)
 *
 * */
float fAS5047PEncGetAngleDeg(enumAS5047PDeviceNumTdf emDeviceNum)
{
	if ((uint8_t)emDeviceNum >= AS5047P_DEVICE_NUM)
	{
		return 0.0f;
	}
	return arrystAS5047PDeviceparam[emDeviceNum].AS5047PENCDynamicParame.fAngleDeg;
}


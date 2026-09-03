/**
  ******************************************************************************
  * @file    bsp_AS5047.h
  * @brief   AS5047PBSP层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * AS5047P驱动(普通阻塞模式 + DMA非阻塞模式),HAL库,STM32G431
  *
  * 支持两种SPI传输方式,用于FOC转子角度获取:
  *  - emAS5047PTransferMode_Polling: 普通阻塞模式,调用HAL_SPI_TransmitReceive,简单可靠,单帧约6us。
  *  - emAS5047PTransferMode_DMA:     DMA非阻塞模式,调用HAL_SPI_TransmitReceive_DMA,不占CPU,
  *                                    配合主循环/电流环周期调用完成连续角度更新。
  ******************************************************************************
  */
  
#ifndef __BSP_AS5047P__H
#define __BSP_AS5047P__H

#include "bsp_config.h"
#include "main.h"

/**	@brief 				AS5047P设备号枚举
 * 	@note
 *
 **/
typedef enum
{
	emAS5047PDeviceNum0 = 0,	//AS5047P设备0
}
enumAS5047PDeviceNumTdf;

/**	@brief 				SPI传输模式枚举
 * 	@note
 *
 **/
typedef enum
{
	emAS5047PTransferMode_Polling = 0,	//普通阻塞模式(HAL_SPI_TransmitReceive)
	emAS5047PTransferMode_DMA,			//DMA非阻塞模式(HAL_SPI_TransmitReceive_DMA)
}
enumAS5047PTransferModeTdf;

/**	@brief 				AS5047P标志电平枚举
 * 	@note
 *
 **/
typedef enum
{
	emAS5047PFlag_Reset = 0,	//标志复位(0)
	emAS5047PFlag_Set,			//标志置位(1)
}
enumAS5047PFlagTdf;

/**	@brief 				SPI静态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	SPI_HandleTypeDef 			*pstSpiHandle;		//HAL库SPI句柄(hspi1)
	GPIO_TypeDef 				*pstCsGpioBase;		//片选CS的GPIOx
	uint16_t 					u16CsPin;			//片选CS对应的GPIOPin
	uint16_t 					u16Timeout;			//普通模式单帧传输超时(ms),0则用默认10
	uint32_t 					u32MaxCount;		//编码器每圈码值(默认16384,可配置)
	enumAS5047PTransferModeTdf 	emTransferMode;		//传输模式: Polling / DMA
}
stAS5047PStaticParameTdf;

/**	@brief 				SPI动态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	uint16_t 					u16Angle;			//最近一次角度(0~0x3FFF)
	uint32_t 					u32AngleRaw;		//最近一次角度原始帧值(含状态位)
	
	float 						fAngleRad;			//单圈角度(弧度)
	float 						fAngleDeg;			//单圈角度(度)

	volatile enumAS5047PFlagTdf emError;			//错误标志(上次传输超时/命令帧错误)
	volatile enumAS5047PFlagTdf emBusy;				//DMA传输进行中标志
	volatile enumAS5047PFlagTdf emDmaDone;			//DMA传输完成标志
	uint16_t 					u16TxBuf;			//DMA发送缓冲(命令帧)
	uint16_t 					u16RxBuf;			//DMA接收缓冲(返回帧)
}
stAS5047PDynamicParameTdf;

/**	@brief 				正交编码器(ABI)静态参数结构体定义
 * 	@note				AS5047P ABI差分输出接TIM编码器模式(如TIM3 CH1/CH2)。
 * 						u32Cpr为每圈计数(ABI经倍频后)。
 *
 * */
typedef struct
{
	TIM_HandleTypeDef 			*pstTimHandle;		//编码器定时器句柄(htim3)
	uint32_t 					u32Cpr;				//每圈计数(ABI倍频后,如1024)
}
stAS5047PENCStaticParameTdf;

/**	@brief 				正交编码器(ABI)动态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	int32_t 					i32Count;			//累计计数(带符号,可正反转,跨圈累加)
	uint16_t 					u16RawCount;		//定时器当前原始16位计数
	int16_t 					i16Delta;			//本次增量(±CPR/2内),供上层测速
	float 						fAngleRad;			//单圈角度(弧度)
	float 						fAngleDeg;			//单圈角度(度)
	enumAS5047PFlagTdf 			emStarted;			//是否已启动
}
stAS5047PENCDynamicParameTdf;

/**	@brief 				AS5047P设备结构体定义
 * 	@note
 *
 **/
typedef struct
{
	stAS5047PStaticParameTdf 	AS5047PStaticParame;
	stAS5047PDynamicParameTdf 	AS5047PDynamicParame;
	stAS5047PENCStaticParameTdf AS5047PENCStaticParame;
	stAS5047PENCDynamicParameTdf AS5047PENCDynamicParame;
}
stAS5047PDeviceParameTdf;

/**	@brief 			AS5047P SPI协议常量
 * 	@note			16位命令帧: bit15=偶校验P(由下15位计算),bit14=R/W(1读0写),bit13:0=14位地址/数据。
 * 					读角度寄存器ANGLECOM(地址0x3FFF): bit14=1, bit13:0=0x3FFF。
 * 					AS5047为流水线输出(返回上一次命令帧结果),每次传送返回上一命令数据。
 * 					返回帧: bit15=PARD偶校验,bit14=EF错误,bit13:0=14位数据。
 * */
#define SPI_AS5047P_ADDR_MASK       0x3FFF  /** 地址掩码 (14位) */
#define SPI_AS5047P_REG_ANGLE		0x3FFF		//角度寄存器ANGLECOM地址(14位)
#define SPI_AS5047P_REG_ANGLEUNC	0x3FFE		//无动态补偿角度寄存器ANGLEUNC
#define SPI_AS5047P_REG_DIAAGC		0x3FFC		//诊断/AGC寄存器
#define SPI_AS5047P_READ_BIT		(1U << 14)	//读/写标志位(R/W=1读)
#define SPI_AS5047P_PARITY_BIT		(1U << 15)	//偶校验位
#define SPI_AS5047P_ANGLE_MASK		0x3FFF		//14位角度掩码
#define SPI_AS5047P_ERRFLAG_BIT		(1U << 14)	//返回帧EF错误标志(数据位bit14)
#define SPI_AS5047P_MAX_COUNT		16384		//编码器每圈总码值(14位)

/**	@brief 				函数外部声明
 * 	@note
 *
 **/
void 	vAS5047PDeviceInit(stAS5047PStaticParameTdf *pstInit, enumAS5047PDeviceNumTdf emDeviceNum);
const 	stAS5047PDeviceParameTdf *c_pstGetAS5047PDeviceParame(enumAS5047PDeviceNumTdf emDeviceNum);

/** 普通(阻塞)模式接口 */
uint16_t u16AS5047PReadRegister(enumAS5047PDeviceNumTdf emDeviceNum, uint16_t u16RegAddr);
uint16_t u16AS5047PReadAngle(enumAS5047PDeviceNumTdf emDeviceNum);
void 	 vAS5047PUpdate(enumAS5047PDeviceNumTdf emDeviceNum);		//阻塞单帧角度更新

/** DMA(非阻塞)模式接口 */
void 	 vAS5047PReadAngleDMA(enumAS5047PDeviceNumTdf emDeviceNum);	//启动一帧DMA读
void 	 vAS5047PDmaTask(enumAS5047PDeviceNumTdf emDeviceNum);		//主循环调用:处理完成并续传
void 	 HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);		//DMA传输完成回调(由HAL调用)

/** 正交编码器(ABI)模式接口: 通过TIM编码器模式读取AS5047P ABI差分输出 */
void 	 vAS5047PEncDeviceInit(stAS5047PENCStaticParameTdf *pstInit, enumAS5047PDeviceNumTdf emDeviceNum);
void 	 vAS5047PEncStart(enumAS5047PDeviceNumTdf emDeviceNum);
void 	 vAS5047PEncStop(enumAS5047PDeviceNumTdf emDeviceNum);
void 	 vAS5047PEncUpdate(enumAS5047PDeviceNumTdf emDeviceNum);
int32_t  i32AS5047PEncGetCount(enumAS5047PDeviceNumTdf emDeviceNum);
int16_t  i16AS5047PEncGetDelta(enumAS5047PDeviceNumTdf emDeviceNum);
float 	 fAS5047PEncGetAngleRad(enumAS5047PDeviceNumTdf emDeviceNum);
float 	 fAS5047PEncGetAngleDeg(enumAS5047PDeviceNumTdf emDeviceNum);

/** 通用读取接口 */
uint16_t u16AS5047PGetAngle(enumAS5047PDeviceNumTdf emDeviceNum);
float 	 fAS5047PGetAngleRad(enumAS5047PDeviceNumTdf emDeviceNum);
float 	 fAS5047PGetAngleDeg(enumAS5047PDeviceNumTdf emDeviceNum);
uint8_t  u8AS5047PGetError(enumAS5047PDeviceNumTdf emDeviceNum);
uint8_t  u8AS5047PIsBusy(enumAS5047PDeviceNumTdf emDeviceNum);

#endif

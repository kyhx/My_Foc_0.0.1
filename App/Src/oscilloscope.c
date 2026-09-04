/**
  ******************************************************************************
  * @file    oscilloscope.c
  * @brief   App层示波器应用源文件
  * @author  可以航行
  * @version V2.0.0
  * @date    2026-09-04
  ******************************************************************************
  * @attention
  * 以固定周期(默认10ms)采集三相电流(Ia/Ib/Ic)与三路电角度
  * (θAuto 自动生成 / θSpi 编码器SPI / θAbi 编码器ABI),通过 VOFA+ JustFloat
  * 协议上报,用于测试电角度获取代码。
  * 通道顺序: 0=Ia 1=Ib 2=Ic 3=θAuto 4=θSpi 5=θAbi
  ******************************************************************************
  */

#include "oscilloscope.h"
#include "vofa.h"
#include "bsp_DRV8313.h"
#include "motor.h"
#include "top_config.h"

static VOFA_Handle_t 	stOSc;			//VOFA句柄
static uint32_t 		u32LastTick = 0;	//上次采样时刻(ms)

/**
 * @brief 										App层示波器应用初始化
 * @param		无
 * @note											配置VOFA通道数
 *
 * */
void OSc_Init(void)
{
	VOFA_Init(&stOSc, VOFA_TX_DMA);
	VOFA_ConfigChannels(&stOSc, OSC_CHANNEL_NUM);
	u32LastTick = HAL_GetTick();
}

/**
 * @brief 										立即采集并上报一帧
 * @param		无
 * @note											通道: Ia Ib Ic θAuto θSpi θAbi
 *
 * */
void OSc_ReportOnce(void)
{
	float fIa       = fDRV8313GetCurrentA(DRV8313, 0);	//A相电流(A)
	float fIb       = fDRV8313GetCurrentA(DRV8313, 1);	//B相电流(A)
	float fIc       = fDRV8313GetCurrentA(DRV8313, 2);	//C相电流(A)
	float fThetaAuto= fMotorGetAutoElecRad();			//自动生成电角度(rad)
	float fThetaSpi = fMotorGetEncoderSpiElecRad();		//编码器SPI电角度(rad)
	float fThetaAbi = fMotorGetEncoderAbiElecRad();		//编码器ABI电角度(rad)

	VOFA_SetData(&stOSc, 0, fIa);
	VOFA_SetData(&stOSc, 1, fIb);
	VOFA_SetData(&stOSc, 2, fIc);
	VOFA_SetData(&stOSc, 3, fThetaAuto);
	VOFA_SetData(&stOSc, 4, fThetaSpi);
	VOFA_SetData(&stOSc, 5, fThetaAbi);

	VOFA_Send(&stOSc);
}

/**
 * @brief 										示波器周期任务
 * @param		无
 * @note											主循环周期调用,到采样周期即上报一帧
 *
 * */
void OSc_Task(void)
{
	if ((HAL_GetTick() - u32LastTick) >= OSC_SAMPLE_PERIOD_MS)
	{
		u32LastTick = HAL_GetTick();
		OSc_ReportOnce();
	}
}

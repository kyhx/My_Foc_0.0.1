/**
  ******************************************************************************
  * @file    oscilloscope.h
  * @brief   App层示波器应用头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 周期采集三相电流与三路电角度,通过 VOFA+ JustFloat 协议上报,用于测试电角度获取。
  * 通道: 0=Ia 1=Ib 2=Ic 3=θAuto(自动生成电角度) 4=θSpi(编码器SPI电角度) 5=θAbi(编码器ABI电角度)
  ******************************************************************************
  */

#ifndef __OSCILLOSCOPE_H
#define __OSCILLOSCOPE_H

#include "top_config.h"

#define OSC_CHANNEL_NUM 		6		//示波器通道数(JustFloat)
#define OSC_SAMPLE_PERIOD_MS 	10		//采样/上报周期(ms), 10ms≈100Hz

/*	@brief 								示波器应用初始化
 * 	@param		无
 * 	@note											配置VOFA通道数,初始化测速基准
 *
 * */
void OSc_Init(void);

/*	@brief 								示波器周期任务
 * 	@param		无
 * 	@note											主循环周期调用,到采样周期即采集并上报一帧
 *
 * */
void OSc_Task(void);

/*	@brief 								立即采集并上报一帧
 * 	@param		无
 * 	@note											手动触发或内部周期调用
 *
 * */
void OSc_ReportOnce(void);

#endif

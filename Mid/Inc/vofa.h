/**
  ******************************************************************************
  * @file    vofa.h
  * @brief   VOFA+ 串口上位机中间层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 支持 VOFA+ 的 JustFloat(浮点波形) 与 String(字符串) 两种协议,
  * 底层通过 BSP_UART 层发送,支持 阻塞/DMA/中断 三种方式。
  ******************************************************************************
  */

#ifndef __VOFA_H
#define __VOFA_H

#include "top_config.h"
#include "bsp_uart.h"

#define VOFA_MAX_CHANNELS 			8		//最大通道数
#define VOFA_STRING_BUF_SIZE 		128		//String协议发送缓冲(字节)

/* JustFloat/String 共用帧尾: 0x00 0x00 0x80 0x7F */
#define VOFA_FRAME_TAIL_B0 			0x00
#define VOFA_FRAME_TAIL_B1 			0x00
#define VOFA_FRAME_TAIL_B2 			0x80
#define VOFA_FRAME_TAIL_B3 			0x7F

/* 发送模式 */
#define VOFA_TX_BLOCKING 			0		//阻塞发送(HAL_UART_Transmit)
#define VOFA_TX_DMA 				1		//DMA非阻塞发送(HAL_UART_Transmit_DMA)
#define VOFA_TX_IT 					2		//中断发送(HAL_UART_Transmit_IT)

/*	@brief 				VOFA句柄结构体
 * 	@note
 *
 * */
typedef struct
{
	uint8_t 	u8Channels;			//当前配置的通道数(JustFloat)
	uint8_t 	u8TxMode;			//发送模式: VOFA_TX_BLOCKING / DMA / IT
	float 		afData[VOFA_MAX_CHANNELS];	//各通道待发送数据
}
VOFA_Handle_t;

/*	@brief 										VOFA初始化
 * 	@param		ph						VOFA句柄
 * 	@param		u8TxMode				发送模式(VOFA_TX_BLOCKING/DMA/IT)
 *
 * */
void VOFA_Init(VOFA_Handle_t *ph, uint8_t u8TxMode);

/*	@brief 										配置JustFloat通道数
 * 	@param		ph						VOFA句柄
 * 	@param		u8Ch					通道数(1~VOFA_MAX_CHANNELS)
 *
 * */
void VOFA_ConfigChannels(VOFA_Handle_t *ph, uint8_t u8Ch);

/*	@brief 										设置单通道数据
 * 	@param		ph						VOFA句柄
 * 	@param		u8Idx					通道索引
 * 	@param		fData					数据(float)
 *
 * */
void VOFA_SetData(VOFA_Handle_t *ph, uint8_t u8Idx, float fData);

/*	@brief 										批量设置通道数据
 * 	@param		ph						VOFA句柄
 * 	@param		pfArr					数据数组
 * 	@param		u8Num					数据个数
 *
 * */
void VOFA_SetDataArray(VOFA_Handle_t *ph, float *pfArr, uint8_t u8Num);

/*	@brief 										JustFloat发送一帧
 * 	@param		ph						VOFA句柄
 * 	@note											帧 = N×float(小端) + 帧尾
 *
 * */
void VOFA_Send(VOFA_Handle_t *ph);

/*	@brief 										String协议发送
 * 	@param		ph						VOFA句柄
 * 	@param		pStr					待发送字符串
 *
 * */
void VOFA_SendString(VOFA_Handle_t *ph, const char *pStr);

#endif

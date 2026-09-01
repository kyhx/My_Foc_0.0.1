/**
  ******************************************************************************
  * @file    vofa.c
  * @brief   VOFA+ 串口上位机中间层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 实现 VOFA+ JustFloat / String 协议,底层走 BSP_UART 层。
  * JustFloat 帧 = N×float(小端) + 帧尾 {0x00,0x00,0x80,0x7F}。
  * String 帧 = 字符串 + 帧尾 {0x00,0x00,0x80,0x7F}。
  ******************************************************************************
  */

#include "vofa.h"
#include "top_config.h"
#include <string.h>

/**
 * @brief 										按发送模式发送原始字节
 * @param		ph							VOFA句柄
 * @param		pu8Data					发送缓冲(发送期间须保持有效)
 * @param		u16Len					长度
 * @note											DMA/IT模式要求缓冲生命周期覆盖发送,故本模块用static缓冲。
 *
 * */
static void vVofaSendBytes(VOFA_Handle_t *ph, uint8_t *pu8Data, uint16_t u16Len)
{
	if (ph->u8TxMode == VOFA_TX_DMA)
	{
		enUartSendDMA(emUartDeviceNum0, pu8Data, u16Len);
	}
	else if (ph->u8TxMode == VOFA_TX_IT)
	{
		HAL_UART_Transmit_IT(&huart1, pu8Data, u16Len);
	}
	else
	{
		enUartSendBlocking(emUartDeviceNum0, pu8Data, u16Len);
	}
}

/**
 * @brief 										VOFA初始化
 * @param		ph							VOFA句柄
 * @param		u8TxMode				发送模式
 *
 * */
void VOFA_Init(VOFA_Handle_t *ph, uint8_t u8TxMode)
{
	ph->u8TxMode   = u8TxMode;
	ph->u8Channels = 0;
}

/**
 * @brief 										配置JustFloat通道数
 * @param		ph							VOFA句柄
 * @param		u8Ch						通道数
 *
 * */
void VOFA_ConfigChannels(VOFA_Handle_t *ph, uint8_t u8Ch)
{
	if (u8Ch > VOFA_MAX_CHANNELS)
	{
		u8Ch = VOFA_MAX_CHANNELS;
	}
	ph->u8Channels = u8Ch;
}

/**
 * @brief 										设置单通道数据
 * @param		ph							VOFA句柄
 * @param		u8Idx						通道索引
 * @param		fData						数据(float)
 *
 * */
void VOFA_SetData(VOFA_Handle_t *ph, uint8_t u8Idx, float fData)
{
	if (u8Idx < VOFA_MAX_CHANNELS)
	{
		ph->afData[u8Idx] = fData;
	}
}

/**
 * @brief 										批量设置通道数据
 * @param		ph							VOFA句柄
 * @param		pfArr						数据数组
 * @param		u8Num						数据个数
 *
 * */
void VOFA_SetDataArray(VOFA_Handle_t *ph, float *pfArr, uint8_t u8Num)
{
	uint8_t u8Idx;

	if (u8Num > VOFA_MAX_CHANNELS)
	{
		u8Num = VOFA_MAX_CHANNELS;
	}
	for (u8Idx = 0; u8Idx < u8Num; u8Idx++)
	{
		ph->afData[u8Idx] = pfArr[u8Idx];
	}
}

/**
 * @brief 										JustFloat 发送
 * @param		ph							VOFA句柄
 * @note											将各通道float按小端拼装成字节流,追加帧尾后发送。
 *
 * */
void VOFA_Send(VOFA_Handle_t *ph)
{
	static uint8_t 	au8Buf[VOFA_MAX_CHANNELS * 4 + 4];
	uint16_t 		u16Len = 0;
	uint8_t 		u8Idx;

	for (u8Idx = 0; u8Idx < ph->u8Channels; u8Idx++)
	{
		/* STM32小端,直接拷贝float内存即得小端字节序 */
		memcpy(&au8Buf[u16Len], &ph->afData[u8Idx], 4);
		u16Len += 4;
	}
	au8Buf[u16Len++] = VOFA_FRAME_TAIL_B0;
	au8Buf[u16Len++] = VOFA_FRAME_TAIL_B1;
	au8Buf[u16Len++] = VOFA_FRAME_TAIL_B2;
	au8Buf[u16Len++] = VOFA_FRAME_TAIL_B3;

	vVofaSendBytes(ph, au8Buf, u16Len);
}

/**
 * @brief 										String 协议发送
 * @param		ph							VOFA句柄
 * @param		pStr						待发送字符串
 * @note											超长自动截断,追加帧尾后发送。
 *
 * */
void VOFA_SendString(VOFA_Handle_t *ph, const char *pStr)
{
	static uint8_t 	au8Buf[VOFA_STRING_BUF_SIZE];
	uint16_t 		u16Len = (uint16_t)strlen(pStr);

	if (u16Len > (VOFA_STRING_BUF_SIZE - 4))
	{
		u16Len = VOFA_STRING_BUF_SIZE - 4;
	}
	memcpy(au8Buf, pStr, u16Len);
	au8Buf[u16Len++] = VOFA_FRAME_TAIL_B0;
	au8Buf[u16Len++] = VOFA_FRAME_TAIL_B1;
	au8Buf[u16Len++] = VOFA_FRAME_TAIL_B2;
	au8Buf[u16Len++] = VOFA_FRAME_TAIL_B3;

	vVofaSendBytes(ph, au8Buf, u16Len);
}

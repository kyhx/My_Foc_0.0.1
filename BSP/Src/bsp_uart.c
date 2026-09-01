/**
  ******************************************************************************
  * @file    bsp_uart.c
  * @brief   串口BSP层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 本文件实现串口硬件抽象层，封装HAL库操作
  *		USART1(460800) 阻塞/DMA发送 + 中断单字节接收, 供VOFA上报与命令解析使用
  ******************************************************************************
  */

#include "bsp_uart.h"
#include "bsp_config.h"


//【0】USART1设备。
stUartDeviceParameTdf arrystUartDeviceparam[UART_DEVICE_NUM];


/**
 * @brief 										UART设备初始化
 * @param		pstInit					 	UART静态参数结构体地址
 * @param		emDeviceNum		 		UART设备号
 *
 * */
void vUartDeviceInit(stUartStaticParameTdf *pstInit, enumUartDeviceNumTdf emDeviceNum)
{
	stUartDeviceParameTdf *pstDev = &arrystUartDeviceparam[emDeviceNum];

	pstDev->UartStaticParame.pstUartHandle = pstInit->pstUartHandle;
	pstDev->UartStaticParame.u16Timeout    = (pstInit->u16Timeout == 0) ?
												UART_DEFAULT_TIMEOUT : pstInit->u16Timeout;
	pstDev->UartStaticParame.emTransferMode = pstInit->emTransferMode;

	/* 动态参数复位 */
	pstDev->UartDynamicParame.emError  = emUartFlag_Reset;
	pstDev->UartDynamicParame.emBusy   = emUartFlag_Reset;
	pstDev->UartDynamicParame.emTxDone = emUartFlag_Reset;
	pstDev->UartDynamicParame.emRxReady= emUartFlag_Reset;
	pstDev->UartDynamicParame.emRxNew  = emUartFlag_Reset;
	pstDev->UartDynamicParame.u8RxByte = 0;
}

/**
 * @brief 										BSP层UART整体初始化
 * @note											注册huart1,并启动中断单字节接收(供命令解析)
 *
 * */
void BSP_UART_Init(void)
{
	stUartStaticParameTdf stInit;

	stInit.pstUartHandle  = &huart1;
	stInit.u16Timeout     = UART_DEFAULT_TIMEOUT;
	stInit.emTransferMode = emUartTransferMode_DMA;
	vUartDeviceInit(&stInit, emUartDeviceNum0);

	/* 启动中断单字节接收 */
	enUartReceiveByteIT(emUartDeviceNum0);
}

/**
 * @brief 										获取UART设备参数
 * @param		emDeviceNum		 		UART设备号
 * @retval									设备参数结构体指针(只读)
 *
 * */
const stUartDeviceParameTdf *c_pstGetUartDeviceParame(enumUartDeviceNumTdf emDeviceNum)
{
	return &arrystUartDeviceparam[emDeviceNum];
}

/**
 * @brief 										UART阻塞发送
 * @param		emDeviceNum		 		UART设备号
 * @param		pu8Data				发送数据缓冲
 * @param		u16Len				发送长度(字节)
 * @retval									HAL状态
 *
 * */
HAL_StatusTypeDef enUartSendBlocking(enumUartDeviceNumTdf emDeviceNum, uint8_t *pu8Data, uint16_t u16Len)
{
	stUartDeviceParameTdf *pstDev = &arrystUartDeviceparam[emDeviceNum];
	HAL_StatusTypeDef enStatus;

	if ((pstDev->UartStaticParame.pstUartHandle == NULL) || (pu8Data == NULL))
	{
		return HAL_ERROR;
	}

	enStatus = HAL_UART_Transmit(pstDev->UartStaticParame.pstUartHandle,
									pu8Data, u16Len,
									pstDev->UartStaticParame.u16Timeout);
	if (enStatus != HAL_OK)
	{
		pstDev->UartDynamicParame.emError = emUartFlag_Set;
	}
	return enStatus;
}

/**
 * @brief 										UART DMA非阻塞发送
 * @param		emDeviceNum		 		UART设备号
 * @param		pu8Data				发送数据缓冲(需保持有效直到发送完成)
 * @param		u16Len				发送长度(字节)
 * @retval									HAL状态
 * @note											完成后HAL_UART_TxCpltCallback置emTxDone
 *
 * */
HAL_StatusTypeDef enUartSendDMA(enumUartDeviceNumTdf emDeviceNum, uint8_t *pu8Data, uint16_t u16Len)
{
	stUartDeviceParameTdf *pstDev = &arrystUartDeviceparam[emDeviceNum];
	HAL_StatusTypeDef enStatus;

	if ((pstDev->UartStaticParame.pstUartHandle == NULL) || (pu8Data == NULL))
	{
		return HAL_ERROR;
	}

	pstDev->UartDynamicParame.emBusy   = emUartFlag_Set;
	pstDev->UartDynamicParame.emTxDone = emUartFlag_Reset;

	enStatus = HAL_UART_Transmit_DMA(pstDev->UartStaticParame.pstUartHandle,
										pu8Data, u16Len);
	if (enStatus != HAL_OK)
	{
		pstDev->UartDynamicParame.emBusy  = emUartFlag_Reset;
		pstDev->UartDynamicParame.emError = emUartFlag_Set;
	}
	return enStatus;
}

/**
 * @brief 										UART阻塞接收
 * @param		emDeviceNum		 		UART设备号
 * @param		pu8Buf				接收数据缓冲
 * @param		u16Len				期望接收长度(字节)
 * @retval									HAL状态
 *
 * */
HAL_StatusTypeDef enUartReceiveBlocking(enumUartDeviceNumTdf emDeviceNum, uint8_t *pu8Buf, uint16_t u16Len)
{
	stUartDeviceParameTdf *pstDev = &arrystUartDeviceparam[emDeviceNum];
	HAL_StatusTypeDef enStatus;

	if ((pstDev->UartStaticParame.pstUartHandle == NULL) || (pu8Buf == NULL))
	{
		return HAL_ERROR;
	}

	enStatus = HAL_UART_Receive(pstDev->UartStaticParame.pstUartHandle,
									pu8Buf, u16Len,
									pstDev->UartStaticParame.u16Timeout);
	if (enStatus != HAL_OK)
	{
		pstDev->UartDynamicParame.emError = emUartFlag_Set;
	}
	return enStatus;
}

/**
 * @brief 										UART中断单字节接收(启动/重装)
 * @param		emDeviceNum		 		UART设备号
 * @retval									HAL状态
 * @note											每收到1字节进入HAL_UART_RxCpltCallback并自动重装
 *
 * */
HAL_StatusTypeDef enUartReceiveByteIT(enumUartDeviceNumTdf emDeviceNum)
{
	stUartDeviceParameTdf *pstDev = &arrystUartDeviceparam[emDeviceNum];

	if (pstDev->UartStaticParame.pstUartHandle == NULL)
	{
		return HAL_ERROR;
	}

	pstDev->UartDynamicParame.emRxReady = emUartFlag_Reset;
	pstDev->UartDynamicParame.emRxNew   = emUartFlag_Reset;

	return HAL_UART_Receive_IT(pstDev->UartStaticParame.pstUartHandle,
									(uint8_t*)&pstDev->UartDynamicParame.u8RxByte, 1);
}

/**
 * @brief 										UART查询是否有新接收字节
 * @param		emDeviceNum		 		UART设备号
 * @retval									1-有新字节待处理,0-无
 *
 * */
uint8_t u8UartIsRxNew(enumUartDeviceNumTdf emDeviceNum)
{
	stUartDeviceParameTdf *pstDev = &arrystUartDeviceparam[emDeviceNum];

	return (uint8_t)(pstDev->UartDynamicParame.emRxNew == emUartFlag_Set);
}

/**
 * @brief 										UART获取并清除最近接收字节
 * @param		emDeviceNum		 		UART设备号
 * @retval									最近接收的单字节数据(同时清除emRxNew)
 *
 * */
uint8_t u8UartGetRxByte(enumUartDeviceNumTdf emDeviceNum)
{
	stUartDeviceParameTdf *pstDev = &arrystUartDeviceparam[emDeviceNum];
	uint8_t u8Byte = 0;

	if (pstDev->UartDynamicParame.emRxNew == emUartFlag_Set)
	{
		pstDev->UartDynamicParame.emRxNew = emUartFlag_Reset;
		u8Byte = pstDev->UartDynamicParame.u8RxByte;
	}
	return u8Byte;
}

/**
 * @brief 										UART发送完成回调
 * @param		huart						HAL库UART句柄
 * @note										遍历设备表,匹配句柄置emTxDone并清emBusy
 *
 * */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t u8Idx;

	for (u8Idx = 0; u8Idx < UART_DEVICE_NUM; u8Idx++)
	{
		if (arrystUartDeviceparam[u8Idx].UartStaticParame.pstUartHandle == huart)
		{
			arrystUartDeviceparam[u8Idx].UartDynamicParame.emBusy   = emUartFlag_Reset;
			arrystUartDeviceparam[u8Idx].UartDynamicParame.emTxDone = emUartFlag_Set;
			break;
		}
	}
}

/**
 * @brief 										UART接收完成回调
 * @param		huart						HAL库UART句柄
 * @note										置emRxReady/emRxNew,并自动重装单字节中断接收
 *
 * */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t u8Idx;

	for (u8Idx = 0; u8Idx < UART_DEVICE_NUM; u8Idx++)
	{
		if (arrystUartDeviceparam[u8Idx].UartStaticParame.pstUartHandle == huart)
		{
			arrystUartDeviceparam[u8Idx].UartDynamicParame.emRxReady = emUartFlag_Set;
			arrystUartDeviceparam[u8Idx].UartDynamicParame.emRxNew   = emUartFlag_Set;
			/* 自动重装单字节接收,持续接收下一位 */
			HAL_UART_Receive_IT(huart,
								(uint8_t*)&arrystUartDeviceparam[u8Idx].UartDynamicParame.u8RxByte, 1);
			break;
		}
	}
}

  
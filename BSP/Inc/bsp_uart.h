/**
  ******************************************************************************
  * @file    bsp_uart.h
  * @brief   串口BSP层头文件
  * @author  
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 本文件提供串口硬件抽象层接口，供中间件和应用层调用
  ******************************************************************************
  */

#ifndef __BSP_UART__H
#define __BSP_UART__H
    
#include "bsp_config.h"
#include "main.h"
#include "usart.h"
#include <string.h>


/*	@brief 				UART设备号枚举
 * 	@note
 *
 * */
typedef enum
{
	emUartDeviceNum0 = 0,	//Uart设备0
}
enumUartDeviceNumTdf;


/*	@brief 				UART传输模式枚举
 * 	@note				VOFA上报/命令响应可选择阻塞或DMA方式
 *
 * */
typedef enum
{
	emUartTransferMode_Polling = 0,	//普通阻塞模式(HAL_UART_Transmit/Receive)
	emUartTransferMode_DMA,			//DMA非阻塞模式(HAL_UART_Transmit_DMA)
}
enumUartTransferModeTdf;


/*	@brief 				UART标志电平枚举
 * 	@note
 *
 * */
typedef enum
{
	emUartFlag_Reset = 0,	//标志复位(0)
	emUartFlag_Set,			//标志置位(1)
}
enumUartFlagTdf;


/*	@brief 				UART静态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	UART_HandleTypeDef 			*pstUartHandle;		//HAL库UART句柄(huart1)
	uint16_t 					u16Timeout;			//普通模式超时(ms),0则用默认UART_DEFAULT_TIMEOUT
	enumUartTransferModeTdf 	emTransferMode;		//传输模式: Polling / DMA
}
stUartStaticParameTdf;


/*	@brief 				UART动态参数结构体定义
 * 	@note
 *
 * */
typedef struct
{
	enumUartFlagTdf 			emError;			//错误标志(上次传输超时)
	enumUartFlagTdf 			emBusy;				//DMA发送进行中标志
	enumUartFlagTdf 			emTxDone;			//DMA发送完成标志
	enumUartFlagTdf 			emRxReady;			//接收完成标志(已收到u8RxByte)
	enumUartFlagTdf 			emRxNew;			//新单字节到达待处理标志(供命令中间件查询)
	volatile uint8_t 			u8RxByte;			//最近一次中断接收的单字节数据
}
stUartDynamicParameTdf;


/*	@brief 				UART设备结构体定义
 * 	@note
 *
 * */
typedef struct
{
	stUartStaticParameTdf 		UartStaticParame;
	stUartDynamicParameTdf 		UartDynamicParame;
}
stUartDeviceParameTdf;


/*	@brief 										UART设备初始化
 * 	@param		pstInit					 	UART静态参数结构体地址
 * 	@param		emDeviceNum		 		UART设备号
 * 	@note											BSP_UART_Init内部调用,也可单独注册某一路串口
 *
 * */
void vUartDeviceInit(stUartStaticParameTdf *pstInit, enumUartDeviceNumTdf emDeviceNum);

/*	@brief 										BSP层UART整体初始化(注册huart1并启动中断单字节接收)
 * 	@param		无
 * 	@note											供应用层/user.c调用,需在USART1外设初始化(MX_USART1_UART_Init)之后
 *
 * */
void BSP_UART_Init(void);

/*	@brief 										获取UART设备参数
 * 	@param		emDeviceNum		 		UART设备号
 * 	@retval									设备参数结构体指针(只读)
 *
 * */
const stUartDeviceParameTdf *c_pstGetUartDeviceParame(enumUartDeviceNumTdf emDeviceNum);

/*	@brief 										UART阻塞发送
 * 	@param		emDeviceNum		 		UART设备号
 * 	@param		pu8Data				发送数据缓冲
 * 	@param		u16Len				发送长度(字节)
 * 	@retval									HAL状态
 *
 * */
HAL_StatusTypeDef enUartSendBlocking(enumUartDeviceNumTdf emDeviceNum, uint8_t *pu8Data, uint16_t u16Len);

/*	@brief 										UART DMA非阻塞发送
 * 	@param		emDeviceNum		 		UART设备号
 * 	@param		pu8Data				发送数据缓冲(需保持有效直到发送完成)
 * 	@param		u16Len				发送长度(字节)
 * 	@retval									HAL状态
 * 	@note											完成后HAL_UART_TxCpltCallback置u8TxDone
 *
 * */
HAL_StatusTypeDef enUartSendDMA(enumUartDeviceNumTdf emDeviceNum, uint8_t *pu8Data, uint16_t u16Len);

/*	@brief 										UART阻塞接收
 * 	@param		emDeviceNum		 		UART设备号
 * 	@param		pu8Buf				接收数据缓冲
 * 	@param		u16Len				期望接收长度(字节)
 * 	@retval									HAL状态
 *
 * */
HAL_StatusTypeDef enUartReceiveBlocking(enumUartDeviceNumTdf emDeviceNum, uint8_t *pu8Buf, uint16_t u16Len);

/*	@brief 										UART中断单字节接收(启动/重装)
 * 	@param		emDeviceNum		 		UART设备号
 * 	@retval									HAL状态
 * 	@note											每收到1字节进入HAL_UART_RxCpltCallback并自动重装,
 * 											命令中间件轮询u8RxNew/u8RxByte解析上位机指令
 *
 * */
HAL_StatusTypeDef enUartReceiveByteIT(enumUartDeviceNumTdf emDeviceNum);

/*	@brief 										UART查询是否有新接收字节
 * 	@param		emDeviceNum		 		UART设备号
 * 	@retval									1-有新字节待处理,0-无
 *
 * */
uint8_t u8UartIsRxNew(enumUartDeviceNumTdf emDeviceNum);

/*	@brief 										UART获取并清除最近接收字节
 * 	@param		emDeviceNum		 		UART设备号
 * 	@retval									最近接收的单字节数据(同时清除emRxNew)
 *
 * */
uint8_t u8UartGetRxByte(enumUartDeviceNumTdf emDeviceNum);

/*	@brief 										UART发送完成回调
 * 	@param		huart						HAL库UART句柄
 * 	@note										置对应设备的u8TxDone并清u8Busy
 *
 * */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);

/*	@brief 										UART接收完成回调
 * 	@param		huart						HAL库UART句柄
 * 	@note										置u8RxReady/u8RxNew,并自动重装单字节中断接收
 *
 * */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* __BSP_UART_H */

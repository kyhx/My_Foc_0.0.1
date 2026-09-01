/**
  ******************************************************************************
  * @file    comm.c
  * @brief   串口命令通信中间层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 轮询 BSP_UART 层的中断单字节接收标志,按行(以 \r 或 \n 结束)解析上位机ASCII命令:
  *   help          - 打印命令帮助
  *   angle         - 打印电角度(rad)
  *   current       - 打印三相电流(A)
  *   report        - 查询连续VOFA上报使能
  *   report on/off - 开启/关闭连续VOFA上报
  *   vd <值>       - 设置d轴电压指令(V)
  *   vq <值>       - 设置q轴电压指令(V)
  *   start/stop    - 使能/关断DRV8313电机驱动
  * 回复使用 VOFA+ String 协议,便于上位机统一解析。
  ******************************************************************************
  */

#include "app_comm.h"
#include "bsp_uart.h"
#include "bsp_DRV8313.h"
#include "user.h"
#include "vofa.h"
#include "top_config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define COMM_RX_BUF_SIZE 	64		//接收行缓冲
#define COMM_TX_BUF_SIZE 	128		//回复缓冲

static uint8_t 		au8RxBuf[COMM_RX_BUF_SIZE];
static uint8_t 		u8RxIdx = 0;
static char 		acTxBuf[COMM_TX_BUF_SIZE];
static VOFA_Handle_t stCommVofa;	//用于String协议回复

/**
 * @brief 										以VOFA+ String协议发送一行回复
 * @param		pStr						回复字符串
 *
 * */
static void vCommReply(const char *pStr)
{
	VOFA_SendString(&stCommVofa, pStr);
}

/**
 * @brief 										解析并执行一行命令
 * @param		pLine						以'\0'结尾的命令行
 *
 * */
static void vCommProcessLine(char *pLine)
{
	if (strcmp(pLine, "help") == 0)
	{
		snprintf(acTxBuf, COMM_TX_BUF_SIZE,
			"cmds: help angle current report report on|off vd <v> vq <v> start stop\r\n");
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "angle") == 0)
	{
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "theta_elec=%.4f rad\r\n", fFocGetThetaElec());
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "current") == 0)
	{
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "Ia=%.3f Ib=%.3f Ic=%.3f A\r\n",
				fFocGetCurrentA(0), fFocGetCurrentA(1), fFocGetCurrentA(2));
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "report") == 0)
	{
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "report=%s\r\n",
				u8FocGetReportEnable() ? "on" : "off");
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "report on") == 0)
	{
		vFocSetReportEnable(1);
		vCommReply("report on\r\n");
	}
	else if (strcmp(pLine, "report off") == 0)
	{
		vFocSetReportEnable(0);
		vCommReply("report off\r\n");
	}
	else if (strncmp(pLine, "vd ", 3) == 0)
	{
		vFocSetVd((float)strtof(pLine + 3, NULL));
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "Vd=%.3f V\r\n", fFocGetVd());
		vCommReply(acTxBuf);
	}
	else if (strncmp(pLine, "vq ", 3) == 0)
	{
		vFocSetVq((float)strtof(pLine + 3, NULL));
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "Vq=%.3f V\r\n", fFocGetVq());
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "start") == 0)
	{
		vDRV8313Enable(DRV8313);
		vCommReply("motor start\r\n");
	}
	else if (strcmp(pLine, "stop") == 0)
	{
		vDRV8313Disable(DRV8313);
		vCommReply("motor stop\r\n");
	}
	else
	{
		vCommReply("unknown cmd, type 'help'\r\n");
	}
}

/**
 * @brief 										APP_COMM初始化
 * @note											清空接收缓冲,并确保串口中断单字节接收已启动
 *
 * */
void APP_COMM_Init(void)
{
	u8RxIdx = 0;
	VOFA_Init(&stCommVofa, VOFA_TX_BLOCKING);
	enUartReceiveByteIT(emUartDeviceNum0);
}

/**
 * @brief 										APP_COMM周期任务
 * @note											轮询BSP_UART接收标志,按行累积,行结束解析执行
 *
 * */
void APP_COMM_Execute(void)
{
	uint8_t u8Byte;

	if (u8UartIsRxNew(emUartDeviceNum0))
	{
		u8Byte = u8UartGetRxByte(emUartDeviceNum0);

		if ((u8Byte == '\r') || (u8Byte == '\n'))
		{
			au8RxBuf[u8RxIdx] = '\0';
			if (u8RxIdx > 0)
			{
				vCommProcessLine((char*)au8RxBuf);
			}
			u8RxIdx = 0;
		}
		else if (u8RxIdx < (COMM_RX_BUF_SIZE - 1))
		{
			au8RxBuf[u8RxIdx++] = u8Byte;
		}
		else
		{
			/* 行缓冲溢出,丢弃本行重新累积 */
			u8RxIdx = 0;
		}
	}
}

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
#include "motor.h"
#include "app_motor.h"
#include "user.h"
#include "vofa.h"
#include "top_config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define COMM_RX_BUF_SIZE 	64		//接收行缓冲
#define COMM_TX_BUF_SIZE 	200		//回复缓冲

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
			"help angle current vd vq freq iq id foc "
			"src(auto/manual/encspi/encabi/next/zero) mode open|current start stop state\r\n");
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "angle") == 0)
	{
		snprintf(acTxBuf, COMM_TX_BUF_SIZE,
				"src=%s theta=%.4f rad  auto=%.4f spi=%.4f abi=%.4f rad\r\n",
				pMotorGetAngleSourceName(), fMotorGetElecAngleRad(),
				fMotorGetAutoElecRad(), fMotorGetEncoderSpiElecRad(),
				fMotorGetEncoderAbiElecRad());
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "current") == 0)
	{
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "Ia=%.3f Ib=%.3f Ic=%.3f A\r\n",
				fDRV8313GetCurrentA(DRV8313, 0),
				fDRV8313GetCurrentA(DRV8313, 1),
				fDRV8313GetCurrentA(DRV8313, 2));
		vCommReply(acTxBuf);
	}
	else if (strncmp(pLine, "vd ", 3) == 0)
	{
		vMotorOpenLoopSetVd((float)strtof(pLine + 3, NULL));
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "Vd=%.3f V\r\n", fMotorGetOpenLoopVd());
		vCommReply(acTxBuf);
	}
	else if (strncmp(pLine, "vq ", 3) == 0)
	{
		vMotorOpenLoopSetVq((float)strtof(pLine + 3, NULL));
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "Vq=%.3f V\r\n", fMotorGetOpenLoopVq());
		vCommReply(acTxBuf);
	}
	else if (strncmp(pLine, "iq ", 3) == 0)
	{
		vAppMotorSetIqRef((float)strtof(pLine + 3, NULL));
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "Iq_ref=%.3f A\r\n", fAppMotorGetIqRef());
		vCommReply(acTxBuf);
	}
	else if (strncmp(pLine, "id ", 3) == 0)
	{
		vAppMotorSetIdRef((float)strtof(pLine + 3, NULL));
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "Id_ref=%.3f A\r\n", fAppMotorGetIdRef());
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "foc") == 0)
	{
		snprintf(acTxBuf, COMM_TX_BUF_SIZE,
			"mode=%d Id_ref=%.3f Iq_ref=%.3f Id=%.3f Iq=%.3f Vd=%.3f Vq=%.3f\r\n",
			(int)emAppMotorGetMode(), fAppMotorGetIdRef(), fAppMotorGetIqRef(),
			fAppMotorGetId(), fAppMotorGetIq(), fAppMotorGetVd(), fAppMotorGetVq());
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "mode open") == 0)
	{
		vAppMotorSetMode(emAppMotorMode_OpenLoop);
		vCommReply("mode open (开环)\r\n");
	}
	else if (strcmp(pLine, "mode current") == 0)
	{
		vAppMotorSetMode(emAppMotorMode_Current);
		vCommReply("mode current (FOC电流闭环)\r\n");
	}
	else if (strncmp(pLine, "freq ", 5) == 0)
	{
		vMotorSetAutoFreqHz((float)strtof(pLine + 5, NULL));
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "freq=%.3f Hz\r\n", fMotorGetAutoFreqHz());
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "src auto") == 0)
	{
		vUserMotorSetSource(emMotorAngleSrc_Auto);
		vCommReply("src auto\r\n");
	}
	else if (strcmp(pLine, "src manual") == 0)
	{
		vUserMotorSetSource(emMotorAngleSrc_Manual);
		vCommReply("src manual (固定值)\r\n");
	}
	else if (strcmp(pLine, "src encspi") == 0)
	{
		vUserMotorSetSource(emMotorAngleSrc_EncoderSpi);
		vCommReply("src encspi (编码器SPI)\r\n");
	}
	else if (strcmp(pLine, "src encabi") == 0)
	{
		vUserMotorSetSource(emMotorAngleSrc_EncoderAbi);
		vCommReply("src encabi (编码器ABI)\r\n");
	}
	else if (strcmp(pLine, "src next") == 0)
	{
		vUserMotorNextSource();
		snprintf(acTxBuf, COMM_TX_BUF_SIZE, "src next -> %s\r\n", pUserMotorGetSourceName());
		vCommReply(acTxBuf);
	}
	else if (strcmp(pLine, "zero") == 0)
	{
		vUserMotorCaptureEncoderZero();
		vCommReply("encoder zero captured\r\n");
	}
	else if (strcmp(pLine, "start") == 0)
	{
		uint8_t u8Ret = u8UserMotorStart();
		if (u8Ret == 0)
		{
			vCommReply("motor start\r\n");
		}
		else if (u8Ret == 1)
		{
			vCommReply("start rejected: fault latched (long-press key to clear)\r\n");
		}
		else
		{
			vCommReply("start rejected: current cal not done\r\n");
		}
	}
	else if (strcmp(pLine, "stop") == 0)
	{
		vUserMotorStop();
		vCommReply("motor stop\r\n");
	}
	else if (strcmp(pLine, "state") == 0)
	{
		snprintf(acTxBuf, COMM_TX_BUF_SIZE,
			"run=%d src=%s cal=%d fault=%d mode=%d Iq_ref=%.3f\r\n",
			u8MotorGetRun(), pUserMotorGetSourceName(),
			u8DRV8313GetCalState(DRV8313), u8UserMotorGetFault(),
			(int)emAppMotorGetMode(), fAppMotorGetIqRef());
		vCommReply(acTxBuf);
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

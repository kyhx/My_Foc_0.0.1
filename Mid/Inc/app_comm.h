/**
  ******************************************************************************
  * @file    app_comm.h
  * @brief   串口命令通信中间层头文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 通过串口接收上位机ASCII命令并执行。轮询 BSP_UART 层接收标志,
  * 不占用 HAL_UART_RxCpltCallback(该回调已由 bsp_uart 持有并自动重装)。
  ******************************************************************************
  */

#ifndef __APP_COMM_H
#define __APP_COMM_H

#include "top_config.h"

/*	@brief 										APP_COMM初始化
 * 	@param		无
 * 	@note											清空接收缓冲,并确保串口中断单字节接收已启动
 *
 * */
void APP_COMM_Init(void);

/*	@brief 										APP_COMM周期任务
 * 	@param		无
 * 	@note											主循环周期调用,轮询接收标志,按行解析执行上位机命令
 *
 * */
void APP_COMM_Execute(void);

#endif

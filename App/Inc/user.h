#ifndef __USER_H
#define __USER_H

#include "top_config.h"
#include "motor.h"

#include "main.h"


void vUserExecute(void);
void vUserInit(void);

/* ==================== 电角度来源控制(按键与串口共用) ==================== */
/**
  * @brief  设置电角度来源(auto/manual/encspi/encabi), 供按键/串口选择
  */
void vUserMotorSetSource(enumMotorAngleSrcTdf emSrc);
enumMotorAngleSrcTdf emUserMotorGetSource(void);

/**
  * @brief  启动电机(串口 start)
  * @retval 0=已启动/成功 1=故障锁存被拒 2=电流校准未完成被拒
  */
uint8_t u8UserMotorStart(void);

/**
  * @brief  停止电机并关断桥臂
  */
void vUserMotorStop(void);

uint8_t u8UserMotorIsRunning(void);
uint8_t u8UserMotorGetFault(void);

/**
  * @brief  启停切换(短按): 停止→启动 / 启动→停止
  * @retval 0=成功 1=故障锁存被拒 2=电流校准未完成被拒
  */
uint8_t u8UserMotorToggleRun(void);

/**
  * @brief  切换到下一个电角度来源(循环: Auto→Manual→EncSpi→EncAbi→Auto,长按)
  */
void vUserMotorNextSource(void);

/**
  * @brief  获取当前来源名称字符串("auto"/"manual"/"encspi"/"encabi")
  */
const char *pUserMotorGetSourceName(void);

/**
  * @brief  捕获编码器零位(把当前机械角记为电角度0,供 EncSPI/EncAbi 校准)
  */
void vUserMotorCaptureEncoderZero(void);

#endif

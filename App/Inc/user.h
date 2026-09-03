#ifndef __USER_H
#define __USER_H

#include "top_config.h"
#include "motor.h"

#include "main.h"


void vMotorSetup(void);
void vUserExecute(void);
void vUserInit(void);
//void vKeyControlMotor(void);

/* ==================== 电机运行控制(按键与串口共用) ==================== */
/**
  * @brief  设置电机启动来源(auto/manual/encoder), 供串口选择后启动
  */
void vUserMotorSetSource(enumMotorAngleSrcTdf emSrc);
enumMotorAngleSrcTdf emUserMotorGetSource(void);

/**
  * @brief  按当前选定来源启动电机(含编码器首次转子对齐+捕获零位)
  * @retval 0=已启动/成功 1=故障锁存被拒 2=电流校准未完成被拒
  */
uint8_t u8UserMotorStart(void);

/**
  * @brief  停止电机并关断桥臂
  */
void vUserMotorStop(void);

uint8_t u8UserMotorIsRunning(void);
uint8_t u8UserMotorIsAligning(void);
uint8_t u8UserMotorGetFault(void);

/* ==================== FOC 坐标变换与上报 ==================== */

/**
  * @brief  FOC 变换更新：采样三相电流，Clarke/Park 变换，逆 Park/逆 Clarke 电压指令
  * @param  无
  * @retval 无
  */
void vFocUpdate(void);

/**
  * @brief  通过 VOFA+ JustFloat 协议上报一次 (Ia Ib Ic Id Iq theta Udc)
  * @param  无
  * @retval 无
  */
void vFocReportOnce(void);

void  vFocSetVd(float fVd);      /** 设置 d 轴电压指令(V) */
void  vFocSetVq(float fVq);      /** 设置 q 轴电压指令(V) */
float fFocGetVd(void);           /** 获取 d 轴电压指令(V) */
float fFocGetVq(void);           /** 获取 q 轴电压指令(V) */
float fFocGetId(void);           /** 获取 d 轴电流(A) */
float fFocGetIq(void);           /** 获取 q 轴电流(A) */
float fFocGetCurrentA(uint8_t u8Phase);  /** 获取某相电流(A), 0/1/2 = Ia/Ib/Ic */
float fFocGetThetaElec(void);    /** 获取转子电角度(rad) */
void  vFocSetReportEnable(uint8_t bEnable);  /** 开启/关闭连续 vofa 上报 */
uint8_t u8FocGetReportEnable(void);          /** 查询连续上报使能状态 */

#endif

/**
  ******************************************************************************
  * @file    bsp_led.h
  * @brief   LED BSP层头文件
  * @author  可以航行
  * @version V1.1.0
  * @date    2026-09-03
  ******************************************************************************
  */
#ifndef __BSP_LED_H__
#define __BSP_LED_H__

#include "main.h"
#include <stdint.h>
#include <stdbool.h>


/*==================== 枚举定义 ====================*/

// LED点亮电平
typedef enum {
    emLedOnLevel_Low = 0,   // 低电平点亮
    emLedOnLevel_High = 1   // 高电平点亮
} emLedOnLevelTdf;

// LED状态
typedef enum {
    emLedStatusOff = 0,     // 熄灭
    emLedStatusOn = 1       // 点亮
} emLedStatusTdf;

// LED设备号
typedef enum {
    emLedDeviceNum0 = 0,         
    emLedDeviceNum1 = 1,         
    emLedDeviceNum2 = 2,         
    emLedDeviceNum3 = 3          
} enumLedDeviceNumTdf;

/*==================== 结构体定义 ====================*/

// 静态参数 (硬件配置)
typedef struct {
    GPIO_TypeDef *pstGPIOBase;   // GPIO端口
    uint16_t u16GPIOPin;         // GPIO引脚
    emLedOnLevelTdf emOnLevel;   // 点亮电平
} stLedStaticParameTdf;

// 动态参数 (运行状态)
typedef struct {
    emLedStatusTdf emLedStatus;  // 当前状态
    // 可扩展：闪烁参数等
} stLedDynamicParameTdf;

// 完整LED参数
typedef struct {
    stLedStaticParameTdf LedStaticParame;
    stLedDynamicParameTdf LedDynamicParame;
} stLedDeviceParameTdf;


/*==================== API函数 ====================*/

void vLedDeviceInit(stLedStaticParameTdf *pstInit, enumLedDeviceNumTdf emDeviceNum);
void vLedDeviceInitArray(stLedStaticParameTdf *pstInitArray, uint8_t u8Count);
const stLedDeviceParameTdf *c_pstGetLedDeviceParame(enumLedDeviceNumTdf emDeviceNum);

void vLedOn(enumLedDeviceNumTdf emDeviceNum);
void vLedOff(enumLedDeviceNumTdf emDeviceNum);
void vLedToggle(enumLedDeviceNumTdf emDeviceNum);
void vLedReset(enumLedDeviceNumTdf emDeviceNum);

bool bLedIsOn(enumLedDeviceNumTdf emDeviceNum);

// 闪烁功能
void vLedBlinkBlock(enumLedDeviceNumTdf emDeviceNum, 
                     uint32_t u32OnTime, 
                     uint32_t u32OffTime, 
                     uint32_t u32Count);

#endif /* __BSP_LED_H__ */
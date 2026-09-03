/**
  ******************************************************************************
  * @file    bsp_led.c
  * @brief   LED BSP层源文件
  * @author  可以航行
  * @version V1.1.0
  * @date    2026-09-03
  ******************************************************************************
  * @attention
  * 本文件实现LED硬件抽象层，封装HAL库操作
  * 支持高电平有效和低电平有效两种模式
  ******************************************************************************
  */
#include "bsp_led.h"
#include "bsp_config.h"
#include <stdbool.h>

/*==================== 全局变量 ====================*/
stLedDeviceParameTdf arrystLedDeviceparam[LED_DEVICE_NUM];

/*==================== 内部函数声明 ====================*/
static void vLedUpdatePinLevel(enumLedDeviceNumTdf emDeviceNum);

/*==================== 初始化 ====================*/

/**
 * @brief       LED参数结构体初始化
 * @param       pstInit         LED参数结构体地址
 * @param       emDeviceNum     LED设备号
 * @retval      None
 */
void vLedDeviceInit(stLedStaticParameTdf *pstInit, enumLedDeviceNumTdf emDeviceNum)
{
    if (pstInit == NULL || emDeviceNum >= LED_DEVICE_NUM) {
        return;
    }
    
    stLedDeviceParameTdf *pstDev = &arrystLedDeviceparam[emDeviceNum];
    
    // 复制静态参数
    pstDev->LedStaticParame.pstGPIOBase = pstInit->pstGPIOBase;
    pstDev->LedStaticParame.u16GPIOPin = pstInit->u16GPIOPin;
    pstDev->LedStaticParame.emOnLevel = pstInit->emOnLevel;
    
    // 动态参数复位：默认熄灭
    pstDev->LedDynamicParame.emLedStatus = emLedStatusOff;
    
    // 立即更新引脚状态
    vLedUpdatePinLevel(emDeviceNum);
}

/**
 * @brief       批量初始化LED
 * @param       pstInitArray    初始化参数数组
 * @param       u8Count         数组长度
 */
void vLedDeviceInitArray(stLedStaticParameTdf *pstInitArray, uint8_t u8Count)
{
    for (uint8_t i = 0; i < u8Count && i < LED_DEVICE_NUM; i++) {
        vLedDeviceInit(&pstInitArray[i], i);
    }
}

/**
 * @brief       取得LED设备参数（只读）
 * @param       emDeviceNum     LED设备号
 * @retval      LED设备参数指针
 */
const stLedDeviceParameTdf *c_pstGetLedDeviceParame(enumLedDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= LED_DEVICE_NUM) {
        return NULL;
    }
    return &arrystLedDeviceparam[emDeviceNum];
}

/*==================== 内部函数 ====================*/

/**
 * @brief       更新LED引脚电平，将动态状态落实到硬件
 * @param       emDeviceNum     LED设备号
 * @note        根据点亮电平(emOnLevel)与期望状态(emLedStatus)推导
 *              实际输出的GPIO电平：
 *              - 期望点亮 && 点亮电平=高 → 输出高
 *              - 期望点亮 && 点亮电平=低 → 输出低
 *              - 期望熄灭 && 点亮电平=高 → 输出低
 *              - 期望熄灭 && 点亮电平=低 → 输出高
 */
static void vLedUpdatePinLevel(enumLedDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= LED_DEVICE_NUM) {
        return;
    }
    
    stLedDeviceParameTdf *pstDev = &arrystLedDeviceparam[emDeviceNum];
    
    // 异或逻辑：状态==ON 与 电平==HIGH 相等时输出SET，否则RESET
    bool bWantOn = (pstDev->LedDynamicParame.emLedStatus == emLedStatusOn);
    bool bHighActive = (pstDev->LedStaticParame.emOnLevel == emLedOnLevel_High);
    
    GPIO_PinState enOutput = (bWantOn == bHighActive) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    
    HAL_GPIO_WritePin(pstDev->LedStaticParame.pstGPIOBase,
                      pstDev->LedStaticParame.u16GPIOPin,
                      enOutput);
}

/*==================== 对外API ====================*/

/**
 * @brief       LED点亮
 * @param       emDeviceNum     LED设备号
 */
void vLedOn(enumLedDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= LED_DEVICE_NUM) {
        return;
    }
    
    arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedStatus = emLedStatusOn;
    vLedUpdatePinLevel(emDeviceNum);
}

/**
 * @brief       LED熄灭
 * @param       emDeviceNum     LED设备号
 */
void vLedOff(enumLedDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= LED_DEVICE_NUM) {
        return;
    }
    
    arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedStatus = emLedStatusOff;
    vLedUpdatePinLevel(emDeviceNum);
}

/**
 * @brief       LED翻转
 * @param       emDeviceNum     LED设备号
 */
void vLedToggle(enumLedDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= LED_DEVICE_NUM) {
        return;
    }
    
    stLedDynamicParameTdf *pstDyn = &arrystLedDeviceparam[emDeviceNum].LedDynamicParame;
    
    // 安全翻转状态
    if (pstDyn->emLedStatus == emLedStatusOn) {
        pstDyn->emLedStatus = emLedStatusOff;
    } else {
        pstDyn->emLedStatus = emLedStatusOn;
    }
    
    vLedUpdatePinLevel(emDeviceNum);
}

/**
 * @brief       查询LED当前状态
 * @param       emDeviceNum     LED设备号
 * @retval      true=点亮, false=熄灭
 */
bool bLedIsOn(enumLedDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= LED_DEVICE_NUM) {
        return false;
    }
    return (arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedStatus == emLedStatusOn);
}

/**
 * @brief       LED闪烁（阻塞式）
 * @param       emDeviceNum     LED设备号
 * @param       u32OnTime       点亮时间(ms)
 * @param       u32OffTime      熄灭时间(ms)
 * @param       u32Count        闪烁次数（0=无限）
 * @note        阻塞式闪烁，会占用CPU
 */
void vLedBlinkBlock(enumLedDeviceNumTdf emDeviceNum, 
                     uint32_t u32OnTime, 
                     uint32_t u32OffTime, 
                     uint32_t u32Count)
{
    if (emDeviceNum >= LED_DEVICE_NUM || u32OnTime == 0 || u32OffTime == 0) {
        return;
    }
    
    uint32_t u32Loop = (u32Count == 0) ? 0xFFFFFFFF : (u32Count * 2);
    
    for (uint32_t i = 0; i < u32Loop; i++) {
        if (i % 2 == 0) {
            vLedOn(emDeviceNum);
            HAL_Delay(u32OnTime);
        } else {
            vLedOff(emDeviceNum);
            HAL_Delay(u32OffTime);
        }
    }
}

/**
 * @brief       LED闪烁（非阻塞式 - 需配合定时器）
 * @param       emDeviceNum     LED设备号
 * @param       u32Period       闪烁周期(ms)
 * @param       u32Duty         占空比(0-100)
 * @note        需要在定时器中断中调用 vLedBlinkUpdate()
 */
void vLedBlinkStart(enumLedDeviceNumTdf emDeviceNum, uint32_t u32Period, uint8_t u8Duty)
{
    if (emDeviceNum >= LED_DEVICE_NUM || u32Period == 0) {
        return;
    }
    
    // 需要在结构体中增加 blink 相关字段
    // 这里仅作示例
}

/**
 * @brief       复位LED到熄灭状态
 * @param       emDeviceNum     LED设备号
 */
void vLedReset(enumLedDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= LED_DEVICE_NUM) {
        return;
    }
    
    arrystLedDeviceparam[emDeviceNum].LedDynamicParame.emLedStatus = emLedStatusOff;
    vLedUpdatePinLevel(emDeviceNum);
}
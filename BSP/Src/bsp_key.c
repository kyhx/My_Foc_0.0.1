/**
 * @file			bsp_key.c
 * @author 		可以航行
 * @version 	1.0
 * @date 			2026/9/3
 * @brief 		Key驱动源码 - 完整功能版
 */

#include "bsp_key.h"
#include <string.h>

/*==================== 全局变量 ====================*/
stKeyDeviceParameTdf arrystKeyDeviceparam[KEY_DEVICE_NUM];

/*==================== 内部函数 ====================*/
static void vKeyProcessEvents(stKeyDeviceParameTdf *pstKey, 
                               uint8_t u8RawPressed, 
                               uint32_t u32NowTick);

/*==================== 初始化 ====================*/

void vKeyDeviceInit(stKeyStaticParameTdf *pstInit, enumKeyDeviceNumTdf emDeviceNum)
{
    if (pstInit == NULL || emDeviceNum >= KEY_DEVICE_NUM) {
        return;
    }
    
    stKeyDeviceParameTdf *pstKey = &arrystKeyDeviceparam[emDeviceNum];
    
    // 复制静态参数
    memcpy(&pstKey->KeyStaticParame, pstInit, sizeof(stKeyStaticParameTdf));
    
    // 初始化动态参数
    stKeyDynamicParameTdf *pstDyn = &pstKey->KeyDynamicParame;
    pstDyn->emKeystatus = emKeystatus_Off;
    pstDyn->u8DebouncePending = 0;
    pstDyn->u32DebounceTick = 0;
    pstDyn->u8PressEvent = 0;
    pstDyn->u8ReleaseEvent = 0;
    pstDyn->u8LongPressEvent = 0;
    pstDyn->u8RepeatEvent = 0;
    pstDyn->u32PressStartTick = 0;
    pstDyn->u8LongPressFlag = 0;
    pstDyn->u8RepeatFlag = 0;
    pstDyn->u32LastRepeatTick = 0;
    pstDyn->u8SwitchState = 0;
}

void vKeyDeviceInitArray(stKeyStaticParameTdf *pstInitArray, uint8_t u8Count)
{
    for (uint8_t i = 0; i < u8Count && i < KEY_DEVICE_NUM; i++)
    {
        vKeyDeviceInit(&pstInitArray[i], i);
    }
}

/*==================== 核心扫描 ====================*/

void vKeyScan(void)
{
    uint32_t u32NowTick = HAL_GetTick();
    
    for (uint8_t u8Index = 0; u8Index < KEY_DEVICE_NUM; u8Index++)
    {
        stKeyDeviceParameTdf *pstKey = &arrystKeyDeviceparam[u8Index];
        stKeyDynamicParameTdf *pstDyn = &pstKey->KeyDynamicParame;
        stKeyStaticParameTdf *pstStatic = &pstKey->KeyStaticParame;
        
        // 读取原始电平并换算为按下状态
        uint8_t u8RawPressed = (HAL_GPIO_ReadPin(
            pstStatic->pstGPIOBase,
            pstStatic->u16GPIOPin) 
            == (GPIO_PinState)pstStatic->emKeyLevel);
        
        // 当前稳定状态
        uint8_t u8StablePressed = (pstDyn->emKeystatus == emKeystatus_On);
        
        // 消抖处理
        if (u8RawPressed == u8StablePressed)
        {
            // 状态一致，清除消抖标志
            pstDyn->u8DebouncePending = 0;
            
            // 如果处于按下状态，处理长按/连发
            if (u8RawPressed)
            {
                vKeyProcessEvents(pstKey, u8RawPressed, u32NowTick);
            }
        }
        else
        {
            // 状态不一致，进入消抖
            if (pstDyn->u8DebouncePending == 0)
            {
                // 首次检测到变化
                pstDyn->u8DebouncePending = 1;
                pstDyn->u32DebounceTick = u32NowTick;
            }
            else if ((u32NowTick - pstDyn->u32DebounceTick) >= KEY_DEBOUNCE_TIME)
            {
                // 消抖完成，确认状态切换
                pstDyn->u8DebouncePending = 0;
                
                if (u8RawPressed)
                {
                    // 按下：记录时间，触发事件
                    pstDyn->emKeystatus = emKeystatus_On;
                    pstDyn->u32PressStartTick = u32NowTick;
                    pstDyn->u8LongPressFlag = 0;
                    pstDyn->u8RepeatFlag = 0;
                    
                    // 根据模式处理
                    if (pstStatic->emKeyMode == emKeyMode_Switch)
                    {
                        // 自锁模式：切换状态
                        pstDyn->u8SwitchState = !pstDyn->u8SwitchState;
                        if (pstDyn->u8SwitchState)
                        {
                            pstDyn->u8PressEvent = 1;
                        }
                        else
                        {
                            pstDyn->u8ReleaseEvent = 1;
                        }
                    }
                    else
                    {
                        
                    }
                }
                else
                {
                    // 释放：触发释放事件
                    pstDyn->emKeystatus = emKeystatus_Off;
                    
                    // 检查是否长按（长按模式下）
                    if (pstStatic->emKeyMode == emKeyMode_LongPress)
                    {
                        if (pstDyn->u8LongPressFlag)
                        {
                            // 已经触发了长按事件，不触发释放事件
                            pstDyn->u8LongPressFlag = 0;
                        }
                        else
                        {
                            // 短按：触发单击事件
                            pstDyn->u8PressEvent = 1;  // 复用PressEvent作为单击
                        }
                    }
                    else if (pstStatic->emKeyMode != emKeyMode_Switch)
                    {
                        // 非自锁模式：触发释放事件
                        pstDyn->u8ReleaseEvent = 1;
                    }
                    
                    // 重置连发标志
                    pstDyn->u8RepeatFlag = 0;
                }
            }
        }
    }
}

/*==================== 事件处理 ====================*/

static void vKeyProcessEvents(stKeyDeviceParameTdf *pstKey, 
                               uint8_t u8RawPressed, 
                               uint32_t u32NowTick)
{
    stKeyDynamicParameTdf *pstDyn = &pstKey->KeyDynamicParame;
    stKeyStaticParameTdf *pstStatic = &pstKey->KeyStaticParame;
    
    uint32_t u32PressDuration = u32NowTick - pstDyn->u32PressStartTick;
    
    // 获取长按时间（使用默认或自定义）
    uint32_t u32LongPressTime = (pstStatic->u16LongPressTime > 0) 
                                 ? pstStatic->u16LongPressTime 
                                 : KEY_LONG_PRESS_MS;
    
    // 长按检测
    if (pstStatic->emKeyMode == emKeyMode_LongPress || 
        pstStatic->emKeyMode == emKeyMode_Repeat)
    {
        if (u32PressDuration >= u32LongPressTime && 
            pstDyn->u8LongPressFlag == 0)
        {
            pstDyn->u8LongPressFlag = 1;
            pstDyn->u8LongPressEvent = 1;
            
            // 如果是长按模式，触发长按事件
            if (pstStatic->emKeyMode == emKeyMode_LongPress)
            {
                pstDyn->u8PressEvent = 0;  // 清除按下事件
            }
        }
    }
    
    // 连发检测 (只在长按模式下)
    if (pstStatic->emKeyMode == emKeyMode_Repeat && 
        pstDyn->u8LongPressFlag)
    {
        uint32_t u32RepeatDelay = (pstStatic->u16RepeatDelay > 0) 
                                   ? pstStatic->u16RepeatDelay 
                                   : KEY_REPEAT_DELAY;
        uint32_t u32RepeatTime = (pstStatic->u16RepeatTime > 0) 
                                  ? pstStatic->u16RepeatTime 
                                  : KEY_REPEAT_TIME;
        
        if (pstDyn->u8RepeatFlag == 0)
        {
            // 首次连发：需要延迟
            if (u32PressDuration >= u32RepeatDelay)
            {
                pstDyn->u8RepeatFlag = 1;
                pstDyn->u32LastRepeatTick = u32NowTick;
                pstDyn->u8RepeatEvent = 1;
            }
        }
        else
        {
            // 后续连发：按间隔触发
            if ((u32NowTick - pstDyn->u32LastRepeatTick) >= u32RepeatTime)
            {
                pstDyn->u32LastRepeatTick = u32NowTick;
                pstDyn->u8RepeatEvent = 1;
            }
        }
    }
}

/*==================== API函数 ====================*/

bool bKeyIsPressed(enumKeyDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= KEY_DEVICE_NUM) {
        return false;
    }
    
    stKeyDeviceParameTdf *pstKey = &arrystKeyDeviceparam[emDeviceNum];
    
    // 自锁模式返回切换状态
    if (pstKey->KeyStaticParame.emKeyMode == emKeyMode_Switch) {
        return (pstKey->KeyDynamicParame.u8SwitchState == 1);
    }
    
    // 其他模式返回物理状态
    return (pstKey->KeyDynamicParame.emKeystatus == emKeystatus_On);
}

emKeyEventTdf emKeyGetEvent(enumKeyDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= KEY_DEVICE_NUM) {
        return emKeyEvent_None;
    }
    
    stKeyDynamicParameTdf *pstDyn = &arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame;
    
    if (pstDyn->u8PressEvent) {
        pstDyn->u8PressEvent = 0;
        return emKeyEvent_Press;
    }
    if (pstDyn->u8ReleaseEvent) {
        pstDyn->u8ReleaseEvent = 0;
        return emKeyEvent_Release;
    }
    if (pstDyn->u8LongPressEvent) {
        pstDyn->u8LongPressEvent = 0;
        return emKeyEvent_LongPress;
    }
    if (pstDyn->u8RepeatEvent) {
        pstDyn->u8RepeatEvent = 0;
        return emKeyEvent_Repeat;
    }
    
    return emKeyEvent_None;
}

bool bKeyGetPressEvent(enumKeyDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= KEY_DEVICE_NUM) {
        return false;
    }
    
    stKeyDynamicParameTdf *pstDyn = &arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame;
    uint8_t u8Event = pstDyn->u8PressEvent;
    pstDyn->u8PressEvent = 0;
    return (u8Event != 0);
}

bool bKeyGetReleaseEvent(enumKeyDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= KEY_DEVICE_NUM) {
        return false;
    }
    
    stKeyDynamicParameTdf *pstDyn = &arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame;
    uint8_t u8Event = pstDyn->u8ReleaseEvent;
    pstDyn->u8ReleaseEvent = 0;
    return (u8Event != 0);
}

bool bKeyGetLongPressEvent(enumKeyDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= KEY_DEVICE_NUM) {
        return false;
    }
    
    stKeyDynamicParameTdf *pstDyn = &arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame;
    uint8_t u8Event = pstDyn->u8LongPressEvent;
    pstDyn->u8LongPressEvent = 0;
    return (u8Event != 0);
}

bool bKeyGetRepeatEvent(enumKeyDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= KEY_DEVICE_NUM) {
        return false;
    }
    
    stKeyDynamicParameTdf *pstDyn = &arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame;
    uint8_t u8Event = pstDyn->u8RepeatEvent;
    pstDyn->u8RepeatEvent = 0;
    return (u8Event != 0);
}

bool bKeyGetSwitchState(enumKeyDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= KEY_DEVICE_NUM) {
        return false;
    }
    return (arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame.u8SwitchState == 1);
}

void vKeyReset(enumKeyDeviceNumTdf emDeviceNum)
{
    if (emDeviceNum >= KEY_DEVICE_NUM) {
        return;
    }
    
    stKeyDynamicParameTdf *pstDyn = &arrystKeyDeviceparam[emDeviceNum].KeyDynamicParame;
    pstDyn->emKeystatus = emKeystatus_Off;
    pstDyn->u8DebouncePending = 0;
    pstDyn->u8PressEvent = 0;
    pstDyn->u8ReleaseEvent = 0;
    pstDyn->u8LongPressEvent = 0;
    pstDyn->u8RepeatEvent = 0;
    pstDyn->u8LongPressFlag = 0;
    pstDyn->u8RepeatFlag = 0;
    pstDyn->u8SwitchState = 0;
}
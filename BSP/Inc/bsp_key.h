/**
 * @file			bsp_key.h
 * @author 		可以航行
 * @version 	1.0
 * @date 			2026/9/3
 * @brief 		Key驱动头文件 - 完整功能版
 */

#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "bsp_config.h"


/*==================== 枚举定义 ====================*/

// 按键电平有效方式
typedef enum {
    emKeyLevel_Low = 0,          // 低电平有效
    emKeyLevel_High = 1          // 高电平有效
} emKeyLevelTdf;

// 按键状态
typedef enum {
    emKeystatus_Off = 0,         // 释放
    emKeystatus_On = 1           // 按下
} emKeystatusTdf;

// 按键工作模式
typedef enum {
    emKeyMode_Pulse = 0,         // 点动模式 (按下有效, 释放停止)
    emKeyMode_Switch = 1,        // 自锁模式 (按一下切换, 再按切换)
    emKeyMode_LongPress = 2,     // 长按模式 (短按/长按区分)
    emKeyMode_Repeat = 3         // 连发模式 (长按连续触发)
} emKeyModeTdf;

// 按键事件类型
typedef enum {
    emKeyEvent_None = 0,         // 无事件
    emKeyEvent_Press = 1,        // 按下事件
    emKeyEvent_Release = 2,      // 释放事件
    emKeyEvent_LongPress = 3,    // 长按事件
    emKeyEvent_Repeat = 4,       // 连发事件
    emKeyEvent_Click = 5,        // 单击事件 (短按)
    emKeyEvent_DoubleClick = 6   // 双击事件 (需扩展)
} emKeyEventTdf;

// 按键设备号
typedef enum {
    emKeyDeviceNum0 = 0,
    // emKeyDeviceNum1 = 1,
    // emKeyDeviceNum2 = 2,
    // emKeyDeviceNum3 = 3
} enumKeyDeviceNumTdf;

/*==================== 结构体定义 ====================*/

// 静态参数 (硬件配置)
typedef struct {
    GPIO_TypeDef *pstGPIOBase;       // GPIO端口
    uint16_t u16GPIOPin;             // GPIO引脚
    emKeyLevelTdf emKeyLevel;        // 有效电平
    emKeyModeTdf emKeyMode;          // 工作模式
    uint16_t u16LongPressTime;       // 长按时间 (ms, 0使用默认)
    uint16_t u16RepeatDelay;         // 连发延迟 (ms, 0使用默认)
    uint16_t u16RepeatTime;          // 连发间隔 (ms, 0使用默认)
} stKeyStaticParameTdf;

// 动态参数 (运行状态)
typedef struct {
    emKeystatusTdf emKeystatus;      // 当前稳定状态
    uint8_t u8DebouncePending;       // 消抖等待标志
    uint32_t u32DebounceTick;        // 消抖开始时间戳
    
    // 事件标志
    uint8_t u8PressEvent;            // 按下事件 (查询后清除)
    uint8_t u8ReleaseEvent;          // 释放事件 (查询后清除)
    uint8_t u8LongPressEvent;        // 长按事件 (查询后清除)
    uint8_t u8RepeatEvent;           // 连发事件 (查询后清除)
    
    // 长按/连发相关
    uint32_t u32PressStartTick;      // 按下开始时间
    uint8_t u8LongPressFlag;         // 长按标志
    uint8_t u8RepeatFlag;            // 连发标志
    uint32_t u32LastRepeatTick;      // 上次连发时间
    
    // 自锁模式状态
    uint8_t u8SwitchState;           // 自锁状态 (0=OFF, 1=ON)
} stKeyDynamicParameTdf;

// 完整按键参数
typedef struct {
    stKeyStaticParameTdf KeyStaticParame;
    stKeyDynamicParameTdf KeyDynamicParame;
} stKeyDeviceParameTdf;

/*==================== 外部变量 ====================*/

/*==================== API函数 ====================*/

/**
 * @brief 按键初始化
 * @param pstInit       静态参数指针
 * @param emDeviceNum   设备号
 */
void vKeyDeviceInit(stKeyStaticParameTdf *pstInit, enumKeyDeviceNumTdf emDeviceNum);

/**
 * @brief 批量初始化按键
 * @param pstInitArray  静态参数数组
 * @param u8Count       数组长度
 */
void vKeyDeviceInitArray(stKeyStaticParameTdf *pstInitArray, uint8_t u8Count);

/**
 * @brief 按键扫描 (在主循环中周期调用)
 */
void vKeyScan(void);

/**
 * @brief 查询按键稳定状态
 * @param emDeviceNum   设备号
 * @retval true=按下, false=释放
 */
bool bKeyIsPressed(enumKeyDeviceNumTdf emDeviceNum);

/**
 * @brief 查询按键事件 (自动清除)
 * @param emDeviceNum   设备号
 * @retval 事件类型
 */
emKeyEventTdf emKeyGetEvent(enumKeyDeviceNumTdf emDeviceNum);

/**
 * @brief 查询按键按下事件 (自动清除)
 * @param emDeviceNum   设备号
 * @retval true=有事件
 */
bool bKeyGetPressEvent(enumKeyDeviceNumTdf emDeviceNum);

/**
 * @brief 查询按键释放事件 (自动清除)
 * @param emDeviceNum   设备号
 * @retval true=有事件
 */
bool bKeyGetReleaseEvent(enumKeyDeviceNumTdf emDeviceNum);

/**
 * @brief 查询按键长按事件 (自动清除)
 * @param emDeviceNum   设备号
 * @retval true=有事件
 */
bool bKeyGetLongPressEvent(enumKeyDeviceNumTdf emDeviceNum);

/**
 * @brief 查询按键连发事件 (自动清除)
 * @param emDeviceNum   设备号
 * @retval true=有事件
 */
bool bKeyGetRepeatEvent(enumKeyDeviceNumTdf emDeviceNum);

/**
 * @brief 获取自锁状态
 * @param emDeviceNum   设备号
 * @retval true=ON, false=OFF
 */
bool bKeyGetSwitchState(enumKeyDeviceNumTdf emDeviceNum);

/**
 * @brief 重置按键状态
 * @param emDeviceNum   设备号
 */
void vKeyReset(enumKeyDeviceNumTdf emDeviceNum);

#endif /* __BSP_KEY_H__ */
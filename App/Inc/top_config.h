/*
 * @file					top_config.h
 * @author 				可以航行
 * @version 			0.1
 * @data 					2026/8/19
 * @brief 				工程配置文件，顶层参数表
 * */

#ifndef __TOP_CONFIG_H
#define __TOP_CONFIG_H


#include "main.h"


//LED设备
#define LED_DEVICE_NUM 		2		
#define LED 		emLedDeviceNum0		//LED指示ADC采样
#define PWNEN 	emLedDeviceNum1		//PWNEN指示电机启动

//KEY设备
#define KEY_DEVICE_NUM 		1		
#define KEY_DEBOUNCE_TIME 		20		//按键消抖时间(ms)
#define KEY_LONG_PRESS_MS 		800		//长按判定时间(ms): 短按切启停,长按调占空比
#define KEY_ADJ_INTERVAL_MS 		150		//长按调占空比步进间隔(ms)
#define KEY 			emKeyDeviceNum0		//KEY按键

//Comm设备
// 根据实际使用的串口修改
#define BSP_UART_HANDLE      huart1          // 对应CubeMX生成的句柄
#define BSP_UART_INSTANCE    UART1          // 串口实例
#define BSP_UART_IRQ         UART1_IRQn     // 中断号
#define BSP_UART_IRQHandler  UART1_IRQHandler // 中断服务函数
#define BSP_UART_TIMEOUT     1000
//PWM设备
#define PWM_DEVICE_NUM 		1		//PWM设备数量
#define PWM 		emPwmDeviceNum0		//PWM设备0

//ADC设备
#define ADC_DEVICE_NUM 		1		//ADC设备数量
#define ADC 		emAdcDeviceNum0		//ADC设备0
#define ADC_SAMPLE_AT_PWM_PEAK 		1		//ADC注入采样时刻: 1=峰顶(上溢)触发,适配低边采样电阻; 0=谷底(下溢),适配高边采样(与tim.c配合)
#define ADC_CAL_NUM 		10		//foc_current 偏置校准采样次数

//DRV8313设备
#define DRV8313_DEVICE_NUM 		1		//DRV8313设备数量
#define DRV8313 		emDRV8313DeviceNum0		//DRV8313设备0

//AS5047P设备
#define AS5047P_DEVICE_NUM 		1		//AS5047P设备数量
#define AS5047P 		emAS5047PDeviceNum0		//AS5047P设备0
#define AS5047P_ENC_CPR 			1024		//编码器每圈计数
#define AS5047P_SPEED_WINDOW_MS 		20		//测速窗口(ms): 累计该窗口内增量再算速度,消除主循环快慢影响


//电机参数
#define MOTOR_POLE_PAIRS 		7		//电机极对数(机械角→电角度)

//ADC电流/电压换算参数
/* 电流采样方案: 三相低边各1个采样电阻, 经LM324运放(3.3V供电)放大后送ADC1_IN1/2/3,
 * 母线电压经220K/10K分压送ADC1_IN4。LM324中点参考电压=3.3V/2=1.65V。
 * 换算: I = (raw - 中点偏置) × (ADC_ADC_REF/4096) × ADC_CUR_GAIN,
 *       中点偏置由 vFocCurrentCalibrate 在占空比0、电流0时实测均值得到。
 * 增益推导: V_sense = I×RSENSE×AMP → ADC_CUR_GAIN = 1/(RSENSE×AMP) (A/V)。
 * 当前 RSENSE=0.01Ω(10mΩ)×AMP=16.5, 增益≈6.0606 A/V,
 * 满量程 ±1.65V ↔ ±10A。 */

//零点对齐与开环驱动流程配置(user.c接入)
#define FOC_OL_ENABLE 			1		//1=上电后自动零点对齐并开环运行; 0=仅做电流/编码器采样
#define ZA_DUTY_PERMILLE 		300		//零点对齐注入占空比(千分比0~1000), 需防止过流
#define ZA_LOCK_MS 				800		//零点对齐转子锁定稳定等待时长(ms)
#define OL_UD_V 				0.0f	//开环d轴电压指令(V)
#define OL_UQ_V 				0.5f	//开环q轴电压指令(V)

#endif

/*
 * @file					bsp_config.h
 * @author 				可以航行
 * @version 			0.1
 * @data 					2026/8/19
 * @brief 				工程配置文件
 * */

#ifndef __BSP_CONFIG_H
#define __BSP_CONFIG_H


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

// 超时时间（ms）
#define UART_DEFAULT_TIMEOUT 		100		//普通模式发送/接收默认超时(ms)
#define UART_DEVICE_NUM 			1		//UART设备数量



//DRV8313设备
#define DRV8313_DEVICE_NUM 		1		//DRV8313设备数量
#define DRV8313 		emDRV8313DeviceNum0		//DRV8313设备0
#define ADC_REFER 			1.65f		//LM324中点参考电压(V), 3.3V供电分压
#define ADC_ADC_REF 			3.3f		//ADC满量程参考电压(V)
#define ADC_RSENSE 			0.01f		//低边采样电阻(Ω): 10mΩ
#define ADC_CUR_AMP 			16.5f		//LM324放大倍数
#define ADC_CUR_GAIN 			(1.0f/(ADC_RSENSE*ADC_CUR_AMP))	//电流增益(A/V) = 1/(0.01×16.5) ≈ 6.0606
#define ADC_UDC_GAIN 			23.0f		//母线电压增益(220K/10K=23)

//电流采样有效性与保护参数
#define ADC_RAW_MIN 			124		//LM324输出有效下限(raw): 0.1V≈124LSB, 低于此判饱和/断线
#define ADC_RAW_MAX 			3971	//LM324输出有效上限(raw): 3.2V≈3971LSB, 高于此判饱和
#define ADC_OFFSET_RAW_TOL 		200		//校准偏置与理论中点(1.65V≈2048LSB)允许偏差(raw), 超出判校准异常
#define ADC_OC_CURRENT_A 		9.0f	//过流阈值(A), 任一相|I|超此值置过流标志(满量程±10A的90%)

//AS5047P设备
#define AS5047P_DEVICE_NUM 		1		//AS5047P设备数量
#define AS5047P 		emAS5047PDeviceNum0		//AS5047P设备0
#define AS5047P_ENC_CPR 			1024		//编码器每圈计数
#define AS5047P_SPEED_WINDOW_MS 		20		//测速窗口(ms): 累计该窗口内增量再算速度,消除主循环快慢影响


#endif

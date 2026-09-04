/***
 * @file			user.c
 * @author 			可以航行
 * @version 		0.0.1
 * @date 			2026/9/4
 * @brief 			用户代码执行(电角度获取测试版)
 *
 * 电角度获取来源共 4 种(Auto自动生成 / Manual固定值 / EncoderSpi编码器SPI /
 * EncoderAbi编码器ABI),由长按按键循环切换;短按控制电机启停。
 * VOFA 上位机经 App/oscilloscope 上报 三相电流 + 三路电角度 便于测试比对。
 * */

#include "bsp_DRV8313.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "motor.h"
#include "user.h"
#include "top_config.h"
#include "bsp_config.h"
#include "oscilloscope.h"
#include "app_comm.h"
#include "motor_config.h"

/* 开环测试参数(可按电机实测调节) */
#define OL_START_FREQ_HZ	10.0f	/* 自动生成默认电频率(Hz) */
#define OL_VD_V				0.0f	/* 开环 d 轴电压(V) */
#define OL_VQ_V				0.5f	/* 开环 q 轴电压(V),幅值决定电流 */

/* 电机运行控制状态(按键与串口共用) */
static enumMotorAngleSrcTdf s_emSelSrc     = emMotorAngleSrc_Auto;	/* 当前选定来源(默认Auto) */
static uint8_t  s_u8FaultLatch  = 0;	/* 故障锁存(过流/硬件故障,长按复位) */
static uint8_t  s_u8EncZeroDone = 0;	/* 编码器零位是否已捕获 */

enumMotorAngleSrcTdf emUserMotorGetSource(void)
{
	return s_emSelSrc;
}

const char *pUserMotorGetSourceName(void)
{
	return pMotorGetAngleSourceName();
}

void vUserMotorSetSource(enumMotorAngleSrcTdf emSrc)
{
	if ((emSrc >= emMotorAngleSrc_Auto) && (emSrc < emMotorAngleSrc_Max))
	{
		s_emSelSrc     = emSrc;
		s_u8EncZeroDone = 0;
		vMotorSetAngleSource(emSrc);
	}
}

void vUserMotorNextSource(void)
{
	/* 循环: Auto→Manual→EncSpi→EncAbi→Auto */
	s_emSelSrc = (enumMotorAngleSrcTdf)(((int)s_emSelSrc + 1) % (int)emMotorAngleSrc_Max);
	s_u8EncZeroDone = 0;
	vMotorSetAngleSource(s_emSelSrc);
}

uint8_t u8UserMotorIsRunning(void)
{
	return u8MotorGetRun();
}

uint8_t u8UserMotorGetFault(void)
{
	return s_u8FaultLatch;
}

void vUserMotorCaptureEncoderZero(void)
{
	vMotorCaptureEncoderZero();
	s_u8EncZeroDone = 1;
}

void vUserMotorStop(void)
{
	vMotorSetRun(0);
	vDRV8313Disable(DRV8313);
	vDRV8313ClearOcFlag(DRV8313);
}

/**
 * @brief 								内部启动执行(按键/串口共用)
 * @retval								0=成功 1=故障锁存被拒 2=电流校准未完成被拒
 * @note									编码器来源若未捕获零位,则以当前机械位置为电角度0;
 * 									Auto来源启动前复位斜坡,保证从0平滑起步。测试以开环输出观察电流。
 *
 * */
static uint8_t u8MotorStart(void)
{
	enumMotorAngleSrcTdf emSrc = s_emSelSrc;

	if (s_u8FaultLatch)
	{
		return 1;
	}
	if (u8MotorGetRun())
	{
		return 0;						//已在运行
	}
	if (u8DRV8313GetCalState(DRV8313) != (uint8_t)emDRV8313Cal_Done)
	{
		return 2;						//电流零偏校准未完成
	}

	/* 编码器来源: 首次启动以当前机械位置为电角度0 */
	if (((emSrc == emMotorAngleSrc_EncoderSpi) || (emSrc == emMotorAngleSrc_EncoderAbi)) && !s_u8EncZeroDone)
	{
		vMotorCaptureEncoderZero();
		s_u8EncZeroDone = 1;
	}
	/* Auto: 复位自动斜坡,从0平滑起步 */
	if (emSrc == emMotorAngleSrc_Auto)
	{
		vMotorResetAutoAngle();
	}

	vMotorSetAngleSource(emSrc);
	vMotorSetRun(1);
	vDRV8313Enable(DRV8313);
	return 0;
}

uint8_t u8UserMotorStart(void)
{
	return u8MotorStart();
}

uint8_t u8UserMotorToggleRun(void)
{
	if (u8MotorGetRun())
	{
		vUserMotorStop();
		return 0;
	}
	return u8MotorStart();
}

void vUserInit(void)
{
	/** 1. 各外设驱动注册与初始化 */
	vLedInit();
	vKeyInit();
	vAS5047PInit();
	vDRV8313Init();
	vPwmInit();
	vUartInit();

	/** 示波器应用初始化 */
	OSc_Init();

	/** 串口命令通信初始化 */
	APP_COMM_Init();

	/** 2. 电角度模块初始化(极对数取 bsp_config 的 MOTOR_POLE_PAIRS) */
	vMotorAngleInit(MOTOR_CONFIG_POLE_PAIRS);
	vMotorSetAutoFreqHz(OL_START_FREQ_HZ);
	vMotorSetAngleSource(emMotorAngleSrc_Auto);
	s_emSelSrc = emMotorAngleSrc_Auto;
	vMotorOpenLoopSetVd(OL_VD_V);
	vMotorOpenLoopSetVq(OL_VQ_V);
}

void vUserExecute(void)
{
	static uint32_t u32LastTick = 0;

	vKeyScan();
	APP_COMM_Execute();

	/** 按键: 长按=切换电角度来源; 短按=启停切换 */
	if (bKeyGetLongPressEvent(KEY))
	{
		vUserMotorNextSource();
	}
	if (bKeyGetPressEvent(KEY))
	{
		u8UserMotorToggleRun();
	}

	/** 刷新编码器(SPI/ABI)与母线电压 */
	vAS5047PUpdate(AS5047P);
	vAS5047PEncUpdate(AS5047P);
	vDRV8313UpdateBusVoltage(DRV8313);

	/** 故障保护: 运行中过流/硬件故障 → 立即停机并锁存 */
	if (u8MotorGetRun() &&
		(u8DRV8313GetOcFlag(DRV8313) || u8DRV8313GetFault(DRV8313)))
	{
		vUserMotorStop();
		s_u8FaultLatch = 1;
	}

	/** 刷新电角度(按所选来源)并开环输出三相PWM */
	vMotorAngleUpdate();
	vMotorOpenLoopRun(fDRV8313GetBusVoltage(DRV8313));

	/** VOFA 周期上报 */
	OSc_Task();

	/** LED 状态指示(500ms) */
	if ((HAL_GetTick() - u32LastTick) >= 500)
	{
		u32LastTick = HAL_GetTick();
		vLedToggle(LED);
	}
}

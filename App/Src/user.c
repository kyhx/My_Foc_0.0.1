/***
 * @file				user.c
 * @author 				可以航行
 * @version 			0.0.1
 * @date 				2026/8/19
 * @brief 				用户代码执行
 * */

#include "bsp_DRV8313.h"
#include "bsp_key.h"
#include "motor.h"
#include "app_motor.h"
#include "user.h"
#include "top_config.h"
#include "bsp_config.h"
#include "oscilloscope.h"
#include "app_comm.h"

/* 开环启动参数(可按电机实测调节) */
#define OL_START_FREQ_HZ	14.0f		//开环启动电频率(Hz),对应自动生成角度转速
#define OL_VD_V				0.0f		//开环d轴电压指令(V)
#define OL_VQ_V				0.5f		//开环q轴电压指令(V),幅值决定电流/转矩

/* 编码器启动相关参数(可按电机实测调节) */
#define ENC_ALIGN_LOCK_MS	800			//转子对齐锁定时间(ms)
#define ENC_ALIGN_VD_V		2.0f		//对齐注入d轴电压(V)

/* 电机运行控制状态(按键与串口共用) */
static enumMotorAngleSrcTdf s_emSelSrc   = emMotorAngleSrc_Encoder;	//启动来源(默认Encoder: 首启先对齐,对齐后开环; 要频率扫频需 src auto)
static uint8_t  s_u8EncAligned = 0;		//编码器零位是否已对齐
static uint8_t  s_u8Aligning   = 0;		//正在执行转子对齐
static uint32_t s_u32AlignTick = 0;		//对齐起始时刻(ms)
static uint8_t  u8FaultLatch   = 0;		//故障锁存(过流/硬件故障,需长按复位)

enumMotorAngleSrcTdf emUserMotorGetSource(void)
{
	return s_emSelSrc;
}

void vUserMotorSetSource(enumMotorAngleSrcTdf emSrc)
{
	if ((emSrc >= emMotorAngleSrc_Auto) && (emSrc <= emMotorAngleSrc_Encoder))
	{
		s_emSelSrc = emSrc;
	}
}

uint8_t u8UserMotorIsRunning(void)
{
	return u8MotorGetRun();
}

uint8_t u8UserMotorIsAligning(void)
{
	return s_u8Aligning;
}

uint8_t u8UserMotorGetFault(void)
{
	return u8FaultLatch;
}

uint8_t u8UserMotorStart(void)
{
	enumMotorAngleSrcTdf emSrc = s_emSelSrc;

	/** 启动门槛: 无故障锁存 且 电流零偏校准完成 */
	if (u8FaultLatch)
	{
		return 1;
	}
	if (u8DRV8313GetCalState(DRV8313) != (uint8_t)emDRV8313Cal_Done)
	{
		return 2;
	}
	if (u8MotorGetRun())
	{
		return 0;				//已在运行
	}

	if (emSrc == emMotorAngleSrc_Encoder)
	{
		if (s_u8EncAligned == 0)
		{
			/** 首次: 转子对齐——注入固定d轴矢量锁转子到电角度0,等待后再捕获零位 */
			vAppMotorSetMode(emAppMotorMode_OpenLoop);	//对齐属开环动作,强制开环保证对齐电压输出
			vMotorSetAngleSource(emMotorAngleSrc_Manual);
			vMotorSetManualElecRad(0.0f);
			vMotorOpenLoopSetVd(ENC_ALIGN_VD_V);
			vMotorOpenLoopSetVq(0.0f);
			vMotorSetRun(1);
			vDRV8313Enable(DRV8313);
			s_u8Aligning   = 1;
			s_u32AlignTick = HAL_GetTick();
			return 0;
		}
		/** 已对齐: 直接用编码器电角度驱动 */
		vMotorSetAngleSource(emMotorAngleSrc_Encoder);
		vMotorOpenLoopSetVd(OL_VD_V);
		vMotorOpenLoopSetVq(OL_VQ_V);
		vMotorSetRun(1);
		vDRV8313Enable(DRV8313);
		return 0;
	}

	/** Auto / Manual: 直接开环运行(强制开环模式,防残留在闭环模式用假角度跑FOC) */
	vAppMotorSetMode(emAppMotorMode_OpenLoop);
	vMotorSetAngleSource(emSrc);
	if (emSrc == emMotorAngleSrc_Auto)
	{
		vMotorResetAutoAngle();
	}
	vMotorSetRun(1);
	vDRV8313Enable(DRV8313);
	return 0;
}

void vUserMotorStop(void)
{
	s_u8Aligning = 0;
	vMotorSetRun(0);
	vDRV8313Disable(DRV8313);
	vDRV8313ClearOcFlag(DRV8313);
}

void vUserInit()
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

	/** 2. 电角度与开环模块初始化(极对数取 bsp_config 的 MOTOR_POLE_PAIRS) */
	vMotorAngleInit(MOTOR_POLE_PAIRS);
	vMotorSetAutoFreqHz(OL_START_FREQ_HZ);		//开环自动生成电角度转速
	vMotorOpenLoopSetVd(OL_VD_V);
	vMotorOpenLoopSetVq(OL_VQ_V);

	/** 3. 电机控制应用(FOC电流环)初始化: 初始化电流环PI并注册PWM同步ISR */
	vAppMotorInit();

}

void vUserExecute()
{
	static uint32_t u32LastTick  = 0;
	vKeyScan();

	/** 串口命令轮询 */
	APP_COMM_Execute();

	/** 编码器对齐完成处理: 锁定时间到 → 捕获零位 → 切编码器来源驱动 */
	if (s_u8Aligning &&
		((HAL_GetTick() - s_u32AlignTick) >= ENC_ALIGN_LOCK_MS))
	{
		vMotorCaptureEncoderZero();
		s_u8EncAligned = 1;
		s_u8Aligning   = 0;
		vMotorSetAngleSource(emMotorAngleSrc_Encoder);
		vMotorOpenLoopSetVd(OL_VD_V);
		vMotorOpenLoopSetVq(OL_VQ_V);
	}

	/** 故障保护: 运行中过流/硬件故障 → 立即停机并锁存 */
	if (u8MotorGetRun() &&
		(u8DRV8313GetOcFlag(DRV8313) || u8DRV8313GetFault(DRV8313)))
	{
		vUserMotorStop();
		u8FaultLatch = 1;
	}

	/** 按键: 短按启动(用当前选定来源), 长按=停止+清故障锁存 */
	if (bKeyGetPressEvent(KEY))
	{
		u8UserMotorStart();
	}
	if (bKeyGetLongPressEvent(KEY))
	{
		vUserMotorStop();
		u8FaultLatch = 0;
	}

	vAS5047PUpdate(AS5047P);
	vAS5047PEncUpdate(AS5047P);
	vDRV8313UpdateBusVoltage(DRV8313);		//刷新母线电压(供SVPWM归一化)

	vMotorAngleUpdate();						//按所选来源刷新电角度
	vMotorOpenLoopRun(fDRV8313GetBusVoltage(DRV8313));	//开环输出三相PWM

	OSc_Task();

	/** 3. 500ms状态指示: LED闪烁指示系统运行 */
	if ((HAL_GetTick() - u32LastTick) >= 500)
	{
		u32LastTick = HAL_GetTick();
		vLedToggle(LED);
	}
}

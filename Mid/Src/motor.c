/**
  ******************************************************************************
  * @file    motor.c
  * @brief   电机中间层源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 电机初始化状态机: "电流零偏校正 + 转子零位对齐"。
  *		状态流: NotInit → Calibrating → Aligning → Ready。
  *		- Calibrating: DRV8313 输出保持关断,注入组(PWM同步)自动完成三相电流零偏低通累加,
  *		  本模块仅轮询 u8DRV8313GetCalState 等待完成(状态=2)。
  *		- Aligning: 使能DRV8313,向A相注入 fAlignDuty,产生沿α轴(θelec=0)的定子磁场,
  *		  转子d轴锁定到A相轴;等待 u32LockMs 稳定后读取机械角记为零位偏移。
  *		- Ready: 补偿零位的电角度 θelec = (θmech - θoffset) × 极对数。
  ******************************************************************************
  */

#include	"motor.h"
#include	"foc.h"

//【0】MOTOR设备。
stMotorDeviceParameTdf arrystMotorDeviceparam[MOTOR_DEVICE_NUM];

#define	MOTOR_TWOPI		6.2831855f	//2π

/**
 * @brief 										角度归一化到 [0, 2π)
 * @param		fAngle					输入角度(rad)
 * @retval									归一化后的角度(rad,0~2π)
 *
 * */
static float fNormalizeAngle(float fAngle)
{
	fAngle = fAngle - MOTOR_TWOPI * (float)((int32_t)(fAngle / MOTOR_TWOPI));
	if (fAngle < 0.0f)
	{
		fAngle += MOTOR_TWOPI;
	}
	return fAngle;
}
/**
 * @brief 										MOTOR设备初始化
 * @param		pstInit					 	MOTOR静态参数结构体地址
 * @param		emDeviceNum		 		MOTOR设备号
 * @note											仅记录配置并复位动态参数,不启动状态机
 * 											(由主循环周期调用vMotorInitTask推进)。
 *
 * */
void vMotorInit(stMotorStaticParameTdf *pstInit, enumMotorDeviceNumTdf emDeviceNum)
{
	stMotorDeviceParameTdf 	*pstDev = &arrystMotorDeviceparam[emDeviceNum];

	/** 静态参数 */
	pstDev->MotorStaticParame.fAlignDuty = pstInit->fAlignDuty;
	if (pstDev->MotorStaticParame.fAlignDuty > 1.0f)
	{
		pstDev->MotorStaticParame.fAlignDuty = 1.0f;
	}
	pstDev->MotorStaticParame.u32LockMs      = pstInit->u32LockMs;
	pstDev->MotorStaticParame.u32CalTimeoutMs = pstInit->u32CalTimeoutMs;
	pstDev->MotorStaticParame.u8Enable       = pstInit->u8Enable;
	pstDev->MotorStaticParame.fAutoSpeedRadS = pstInit->fAutoSpeedRadS;
	pstDev->MotorStaticParame.emAngleSrc     = pstInit->emAngleSrc;
	pstDev->MotorStaticParame.fOlUdV         = pstInit->fOlUdV;
	pstDev->MotorStaticParame.fOlUqV         = pstInit->fOlUqV;

	/** 动态参数复位 */
	pstDev->MotorDynamicParame.emState        = emMotorState_NotInit;
	pstDev->MotorDynamicParame.u32Tick        = 0;
	pstDev->MotorDynamicParame.fZeroOffsetRad = 0.0f;
	pstDev->MotorDynamicParame.fElecAngleRad  = 0.0f;
	pstDev->MotorDynamicParame.emAngleSrc     = pstInit->emAngleSrc;
	pstDev->MotorDynamicParame.fAutoAngleRad  = 0.0f;
	pstDev->MotorDynamicParame.fManualElecRad = 0.0f;
	pstDev->MotorDynamicParame.fAutoSpeedRadS = pstInit->fAutoSpeedRadS;
	pstDev->MotorDynamicParame.u32LastTick    = HAL_GetTick();
	pstDev->MotorDynamicParame.u8Run          = 0;
}
/**
 * @brief 										取得MOTOR设备参数
 * @param		emDeviceNum		 	MOTOR设备号
 *
 * */
const stMotorDeviceParameTdf *c_pstGetMotorDeviceParame(enumMotorDeviceNumTdf emDeviceNum)
{
	return &arrystMotorDeviceparam[emDeviceNum];
}
/**
 * @brief 										MOTOR初始化状态机(主循环周期调用)
 * @param		emDeviceNum		 	MOTOR设备号
 * @note											NotInit → Calibrating → Aligning → Ready。
 * 											电流校正期间保持输出关断(零电流安全);
 * 											零位对齐期间向A相注入占空比锁定转子并记录零位偏移;
 * 											完成后将三相占空比回到中点(0.5,无差模电压,停止对齐力矩)。
 *
 * */
void vMotorInitTask(enumMotorDeviceNumTdf emDeviceNum)
{
	stMotorDeviceParameTdf 	*pstDev = &arrystMotorDeviceparam[emDeviceNum];
	uint32_t 				u32Now;
	float 					fDuty;

	u32Now = HAL_GetTick();

	switch (pstDev->MotorDynamicParame.emState)
	{
		/** 未开始: 判定是否需要初始化流程 */
		case emMotorState_NotInit:
			if (pstDev->MotorStaticParame.u8Enable == 0)
			{
				/** 跳过初始化,直接就绪(零位偏移为0,电角度=机械角×极对数) */
				pstDev->MotorDynamicParame.emState = emMotorState_Ready;
			}
			else
			{
				/** 记录校准起点,进入电流零偏校正 */
				pstDev->MotorDynamicParame.u32Tick = u32Now;
				pstDev->MotorDynamicParame.emState = emMotorState_Calibrating;
			}
			break;

		/** 电流零偏校正: 等待DRV8313注入组自动校准完成 */
		case emMotorState_Calibrating:
			if (u8DRV8313GetCalState(DRV8313) == (uint8_t)emDRV8313Cal_Done)
			{
				/** 校正完成 → 使能输出,注入A相对齐占空比(θelec=0) */
				fDuty = pstDev->MotorStaticParame.fAlignDuty;
				vDRV8313Enable(DRV8313);
				vPwmSetDutyAll(PWM, fDuty, 0.0f, 0.0f);
				pstDev->MotorDynamicParame.u32Tick = u32Now;
				pstDev->MotorDynamicParame.emState = emMotorState_Aligning;
			}
			/** 校准超时保护: 注入组未启动/异常时避免永久卡死 */
			else if ((u32Now - pstDev->MotorDynamicParame.u32Tick) >= pstDev->MotorStaticParame.u32CalTimeoutMs)
			{
				vDRV8313Disable(DRV8313);
				pstDev->MotorDynamicParame.emState = emMotorState_Error;
			}
			break;

		/** 零位对齐: 等待转子锁定稳定后记录零位偏移 */
		case emMotorState_Aligning:
			/** 对齐期间检测DRV8313故障/过流 → 立即关断并报错,避免损坏 */
			if ((u8DRV8313GetFault(DRV8313) != 0) || (u8DRV8313GetOcFlag(DRV8313) != 0))
			{
				vDRV8313Disable(DRV8313);
				vPwmSetDutyAll(PWM, 0.5f, 0.5f, 0.5f);
				pstDev->MotorDynamicParame.emState = emMotorState_Error;
				break;
			}
			if ((u32Now - pstDev->MotorDynamicParame.u32Tick) >= pstDev->MotorStaticParame.u32LockMs)
			{
				/** 锁定处机械角即 θelec=0 → 零位偏移 */
				pstDev->MotorDynamicParame.fZeroOffsetRad = fAS5047PGetAngleRad(AS5047P);
				/** 释放: 三相占空比回中点,无差模电压,停止对齐力矩(输出保持使能) */
				vPwmSetDutyAll(PWM, 0.5f, 0.5f, 0.5f);
				pstDev->MotorDynamicParame.emState = emMotorState_Ready;
			}
			break;

		case emMotorState_Ready:
		case emMotorState_Error:
		default:
			break;
	}
}
/**
 * @brief 										获取初始化状态
 * @param		emDeviceNum		 	MOTOR设备号
 * @retval										MOTOR状态枚举
 *
 * */
enumMotorStateTdf emMotorGetState(enumMotorDeviceNumTdf emDeviceNum)
{
	return arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.emState;
}
/**
 * @brief 										查询初始化是否完成
 * @param		emDeviceNum		 	MOTOR设备号
 * @retval										1=就绪可运行, 0=未就绪
 *
 * */
uint8_t u8MotorIsReady(enumMotorDeviceNumTdf emDeviceNum)
{
	return (uint8_t)(arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.emState == emMotorState_Ready);
}/**
 * @brief 						查询初始化是否出错
 * @param		emDeviceNum		 	MOTOR设备号
 * @retval						1=初始化出错(校准超时/对齐故障), 0=正常
 *
 * */
uint8_t u8MotorIsError(enumMotorDeviceNumTdf emDeviceNum)
{
	return (uint8_t)(arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.emState == emMotorState_Error);
}/**
 * @brief 										获取零位偏移(机械角 rad)
 * @param		emDeviceNum		 	MOTOR设备号
 * @retval										零位偏移(rad),未对齐为0
 *
 * */
float fMotorGetZeroOffsetRad(enumMotorDeviceNumTdf emDeviceNum)
{
	return arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.fZeroOffsetRad;
}
/**
 * @brief 										获取当前电角度(rad)
 * @param		emDeviceNum		 	MOTOR设备号
 * @retval										电角度(rad,0~2π)
 * @note											返回最近一次vMotorAngleUpdate按所选来源计算的缓存值。
 * 											主循环需周期调用vMotorAngleUpdate刷新。
 *
 * */
float fMotorGetElecAngleRad(enumMotorDeviceNumTdf emDeviceNum)
{
	return arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.fElecAngleRad;
}
/**
 * @brief 										设置电角度来源
 * @param		emDeviceNum		 	MOTOR设备号
 * @param		emSrc				电角度来源: 编码器/给定/自动生成
 * @note											切换后由vMotorAngleUpdate按新来源更新电角度。
 *
 * */
void vMotorSetAngleSource(enumMotorDeviceNumTdf emDeviceNum, enumMotorAngleSrcTdf emSrc)
{
	arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.emAngleSrc = emSrc;
}
/**
 * @brief 										设置给定电角度(rad)
 * @param		emDeviceNum		 	MOTOR设备号
 * @param		fAngleRad				给定电角度(rad)
 * @note											配合 emMotorAngleSrc_Manual 使用,用于手动/定位。
 *
 * */
void vMotorSetManualElecRad(enumMotorDeviceNumTdf emDeviceNum, float fAngleRad)
{
	arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.fManualElecRad = fAngleRad;
}
/**
 * @brief 										设置自动生成电角度速度(rad/s)
 * @param		emDeviceNum		 	MOTOR设备号
 * @param		fSpeedRadS				电角度速度(rad/s),可正负(方向)
 * @note											配合 emMotorAngleSrc_Auto 使用,用于开环扫频/起动。
 *
 * */
void vMotorSetAutoSpeed(enumMotorDeviceNumTdf emDeviceNum, float fSpeedRadS)
{
	arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.fAutoSpeedRadS = fSpeedRadS;
}
/**
 * @brief 										获取当前电角度来源
 * @param		emDeviceNum		 	MOTOR设备号
 * @retval										电角度来源枚举
 *
 * */
enumMotorAngleSrcTdf emMotorGetAngleSource(enumMotorDeviceNumTdf emDeviceNum)
{
	return arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.emAngleSrc;
}
/**
 * @brief 										电角度更新(主循环周期调用)
 * @param		emDeviceNum		 	MOTOR设备号
 * @note											按当前所选来源计算电角度并缓存:
 * 											- Auto:     θ += ω·dt 匀速递增(开环自动生成);
 * 											- Manual:   θ = 给定角度;
 * 											- Encoder:  θ = (θmech - θoffset) × 极对数。
 * 											dt 由HAL_GetTick差分得到,角度统一归一化到[0,2π)。
 *
 * */
void vMotorAngleUpdate(enumMotorDeviceNumTdf emDeviceNum)
{
	stMotorDeviceParameTdf 	*pstDev = &arrystMotorDeviceparam[emDeviceNum];
	uint32_t 				u32Now;
	float 					fDt;

	u32Now = HAL_GetTick();
	fDt    = (float)(u32Now - pstDev->MotorDynamicParame.u32LastTick) / 1000.0f;
	if (fDt < 0.0f)
	{
		fDt = 0.0f;
	}
	pstDev->MotorDynamicParame.u32LastTick = u32Now;

	switch (pstDev->MotorDynamicParame.emAngleSrc)
	{
		/** 自动生成(开环): 电角度匀速扫描 */
		case emMotorAngleSrc_Auto:
			pstDev->MotorDynamicParame.fAutoAngleRad +=
					pstDev->MotorDynamicParame.fAutoSpeedRadS * fDt;
			pstDev->MotorDynamicParame.fElecAngleRad =
					fNormalizeAngle(pstDev->MotorDynamicParame.fAutoAngleRad);
			break;

		/** 给定电角度(手动/定位) */
		case emMotorAngleSrc_Manual:
			pstDev->MotorDynamicParame.fElecAngleRad =
					fNormalizeAngle(pstDev->MotorDynamicParame.fManualElecRad);
			break;

		/** 编码器读取(闭环FOC) */
		case emMotorAngleSrc_Encoder:
		default:
		{
			float fMech      = fAS5047PGetAngleRad(AS5047P);
			float fPolePairs = (float)c_pstGetAS5047PDeviceParame(AS5047P)->AS5047PStaticParame.u16PolePairs;
			pstDev->MotorDynamicParame.fElecAngleRad =
					fNormalizeAngle((fMech - pstDev->MotorDynamicParame.fZeroOffsetRad) * fPolePairs);
		}
		break;
	}
}
/**
 * @brief 										开环驱动运行(用当前电角度来源驱动三相PWM)
 * @param		emDeviceNum		 	MOTOR设备号
 * @note											电角度 → 逆Park(dq电压指令) → SVPWM → 三相占空比。
 * 											电角度来源默认为自动生成(开环扫频),转子跟随旋转磁场转动。
 * 											dq电压取自静态参数 fOlUdV/fOlUqV(开环d/q轴电压指令)。
 * 											需在 vMotorAngleUpdate 之后、主循环周期调用。
 *
 * */
void vMotorOpenLoopRun(enumMotorDeviceNumTdf emDeviceNum)
{
	stMotorDeviceParameTdf 	*pstDev = &arrystMotorDeviceparam[emDeviceNum];
	T_Dq_t 					stDq;
	T_AlphaBeta_t 			stAb;
	float 					fUdc;
	float 					fDutyA, fDutyB, fDutyC;
	float 					fTheta;

	/** 停止状态: 三相占空比回到中点(无差模电压,电机不驱动) */
	if (pstDev->MotorDynamicParame.u8Run == 0)
	{
		vPwmSetDutyAll(PWM, 0.5f, 0.5f, 0.5f);
		return;
	}

	/** 当前电角度(按所选来源,默认自动生成) */
	fTheta = pstDev->MotorDynamicParame.fElecAngleRad;

	/** dq电压指令 → 逆Park → αβ */
	stDq.fD = pstDev->MotorStaticParame.fOlUdV;
	stDq.fQ = pstDev->MotorStaticParame.fOlUqV;
	T_InvPark(&stDq, fTheta, &stAb);

	/** αβ → SVPWM → 三相占空比 */
	fUdc = fDRV8313GetBusVoltage(DRV8313);
	SVPWM(&stAb, fUdc, &fDutyA, &fDutyB, &fDutyC);
	vPwmSetDutyAll(PWM, fDutyA, fDutyB, fDutyC);
}
/**
 * @brief 						设置开环运行标志
 * @param		emDeviceNum		MOTOR设备号
 * @param		u8Run			1=转动 0=停止
 * @note							停止时开环驱动输出中点占空比(0.5)。
 *
 * */
void vMotorSetRun(enumMotorDeviceNumTdf emDeviceNum, uint8_t u8Run)
{
	arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.u8Run = (u8Run != 0) ? 1u : 0u;
}
/**
 * @brief 						获取开环运行标志
 * @param		emDeviceNum		MOTOR设备号
 * @retval						1=转动 0=停止
 *
 * */
uint8_t u8MotorGetRun(enumMotorDeviceNumTdf emDeviceNum)
{
	return arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.u8Run;
}
/**
 * @brief 						获取自动生成电角度速度(rad/s)
 * @param		emDeviceNum		MOTOR设备号
 * @retval						电角度速度(rad/s)
 *
 * */
float fMotorGetAutoSpeed(enumMotorDeviceNumTdf emDeviceNum)
{
	return arrystMotorDeviceparam[emDeviceNum].MotorDynamicParame.fAutoSpeedRadS;
}

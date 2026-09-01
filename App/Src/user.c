/***
 * @file				user.c
 * @author 				可以航行
 * @version 			0.0.1
 * @date 				2026/8/19
 * @brief 				用户代码执行
 * */

#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_AS5047.h"
#include "bsp_DRV8313.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "user.h"
#include "top_config.h"
#include "bsp_uart.h"
#include "oscilloscope.h"


void vLedInit()
{
	stLedStaticParameTdf stInit;
	stInit.	pstGPIOBase  =  LED_GPIO_Port;
	stInit. u16GPIOPin   =  LED_Pin;
	stInit. emOnLevel    =  emLedOnLevel_Low;
	 vLedDeviceInit( &stInit,emLedDeviceNum0);
}

void vKeyInit()
{
	stKeyStaticParameTdf stInit;
	stInit.	pstGPIOBase  =  KEY_GPIO_Port;
	stInit. u16GPIOPin   =  KEY_Pin;
	stInit. emKeyLevel   =  emKeyOnLevelLow;	//KEY按下为低电平(内部上拉)
	 vKeyDeviceInit( &stInit,emKeyDeviceNum0);
}

void vAS5047PInit()
{
	stAS5047PStaticParameTdf stInit;
	stInit. pstSpiHandle   = &hspi1;
	stInit. pstCsGpioBase  = SPI1_NSS_GPIO_Port;	//CS: PA15
	stInit. u16CsPin       = SPI1_NSS_Pin;
	stInit. u16Timeout     = 10;					//普通模式单帧超时(ms)
	stInit. u32MaxCount    = SPI_AS5047P_MAX_COUNT;	//每圈16384码
	stInit. u16PolePairs   = MOTOR_POLE_PAIRS;		//电机极对数(机械角→电角度)
	/** 传输方式选择:
	 *  - emAS5047PTransferMode_DMA:     非阻塞,主循环周期调 vAS5047PUpdate 持续更新
	 *  - emAS5047PTransferMode_Polling: 阻塞,单帧约6us,简单可靠 */
	stInit. emTransferMode = emAS5047PTransferMode_DMA;
	vAS5047PDeviceInit(&stInit, AS5047P);

	/** 正交编码器(ABI)接口: AS5047P ABI差分输出接TIM3编码器模式(PA6/PA7) */
	{
		stAS5047PENCStaticParameTdf stEnc;
		stEnc. pstTimHandle = &htim3;
		stEnc. u32Cpr       = AS5047P_ENC_CPR;	//每圈计数(ABI倍频后)
		vAS5047PEncDeviceInit(&stEnc, AS5047P);
		vAS5047PEncStart(AS5047P);
	}
}


void vDRV8313Init()
{
	stDRV8313StaticParameTdf stInit;
	stInit. pstADCHandle  = &hadc1;
	stInit. pstPwnenGpio  = PWNEN_GPIO_Port;	//PA11 输出使能(高有效)
	stInit. u16PwnenPin   = PWNEN_Pin;
	stInit. pstErrorGpio  = ERR_GPIO_Port;		//PB11 错误状态(低有效)
	stInit. u16ErrorPin   = ERR_Pin;
	vDRV8313DeviceInit(&stInit, DRV8313);

	/** 启动TIM1 PWM,为注入组ADC提供TRGO触发源(每周期同步采样三相电流)。
	 *  此时DRV8313尚未使能(PA11为低),桥臂高阻,不会驱动电机,采样安全。
	 *  实际驱动电机时再调用 vDRV8313Enable(DRV8313)。 */
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

	vDRV8313StartInjected(DRV8313);		//启动注入组: PWM触发采样三相电流
	vDRV8313StartDMA(DRV8313);			//启动普通组+DMA: 循环采样母线电压
}

void vUartInit()
{
	stUartStaticParameTdf stInit;

	stInit.pstUartHandle  = &huart1;
	stInit.u16Timeout     = UART_DEFAULT_TIMEOUT;
	stInit.emTransferMode = emUartTransferMode_DMA;
	vUartDeviceInit(&stInit, emUartDeviceNum0);

	/* 启动中断单字节接收 */
	enUartReceiveByteIT(emUartDeviceNum0);

}





void vUserInit()
{

	/** 1. 各外设驱动注册与初始化 */
	vLedInit();
	vKeyInit();
	vAS5047PInit();
	vDRV8313Init();
	vUartInit();
	
	/** 示波器应用初始化 */
	OSc_Init();



}






void vUserExecute()
{
	static uint32_t u32LastTick  = 0;

	/** 1. 轮询任务: 按键扫描、串口帧处理 */
	vKeyScan();

	/** 2. AS5047P角度更新(按配置模式: DMA非阻塞 / 普通阻塞)
	 *    获取后可用 fAS5047PGetAngleRad(AS5047P) 得到FOC转子电角度 */
	vAS5047PUpdate(AS5047P);

	/** 2.1 正交编码器(ABI)角度更新(经TIM3编码器模式)
	 *    获取后可用 fAS5047PEncGetAngleRad(AS5047P) 得到ABI单圈角度 */
	vAS5047PEncUpdate(AS5047P);

	
	/** 2.2 示波器应用: 1kHz周期上报多通道数据到VOFA+ */
	OSc_Task();

	/** 3. 500ms状态指示: LED闪烁指示系统运行 */
	if ((HAL_GetTick() - u32LastTick) >= 500)
	{
		u32LastTick = HAL_GetTick();
		vLedToggle(LED);
	}
}

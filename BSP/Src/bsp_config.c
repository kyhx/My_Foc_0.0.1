/**
  ******************************************************************************
  * @file    bsp_config.c
  * @brief   BSP 配置源文件
  * @author  可以航行
  * @version V1.0.0
  * @date    2026-09-02
  ******************************************************************************
  * @attention
  * 
  ******************************************************************************
  */
#include "bsp_config.h"


void vLedInit()
{

    //emLedDeviceNumX【0】LED启动指示灯。【1】是PWNEN使能指示灯。
	stLedStaticParameTdf stLEDInit;
	stLEDInit.	pstGPIOBase  =  LED_GPIO_Port;
	stLEDInit. u16GPIOPin   =  LED_Pin;
	stLEDInit. emOnLevel    =  emLedOnLevel_Low;
	vLedDeviceInit( &stLEDInit,LED);
	//PWNEN使能指示灯初始化，指示灯与DRV8313输出使能相连，打开指示灯与电机使能部分效果一样。
	stLedStaticParameTdf stPWNENInit;
    stPWNENInit.	pstGPIOBase  =  PWNEN_GPIO_Port;
    stPWNENInit. u16GPIOPin   =  PWNEN_Pin;
    stPWNENInit. emOnLevel    =  emLedOnLevel_High;
	vLedDeviceInit( &stPWNENInit,PWNEN);

}

void vKeyInit()
{
		stKeyStaticParameTdf stInit;
	stInit.	pstGPIOBase  =  KEY_GPIO_Port;
	stInit. u16GPIOPin   =  KEY_Pin;
	stInit. emKeyLevel   =  emKeyLevel_Low;	//KEY按下为低电平(内部上拉)
	stInit. emKeyMode    =  emKeyMode_LongPress;	
	stInit.u16LongPressTime = 0;
    stInit.u16RepeatDelay = 0;
    stInit.u16RepeatTime = 0;

	 vKeyDeviceInit( &stInit,KEY);

}

void vPwmInit()
{
	stPwmStaticParameTdf stInit;
	stInit. pstTimHandle   = &htim1;
	stInit. u16Period      = 0;					//0=读取定时器当前ARR(8400)
	stInit. u8ChannelNum   = 3;
	stInit. aemChannel[0]  = emPwmChannel1;		//A相 → TIM1 CH1
	stInit. aemChannel[1]  = emPwmChannel2;		//B相 → TIM1 CH2
	stInit. aemChannel[2]  = emPwmChannel3;		//C相 → TIM1 CH3
	vPwmDeviceInit(&stInit, PWM);
	vPwmStart(PWM);
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

void vAS5047PInit()
{
	stAS5047PStaticParameTdf stInit;
	stInit. pstSpiHandle   = &hspi1;
	stInit. pstCsGpioBase  = SPI1_NSS_GPIO_Port;	//CS: PA15
	stInit. u16CsPin       = SPI1_NSS_Pin;
	stInit. u16Timeout     = 10;					//普通模式单帧超时(ms)
	stInit. u32MaxCount    = SPI_AS5047P_MAX_COUNT;	//每圈16384码
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
	//  hal_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	//  hal_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	//  hal_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
	 	//vPwmStart(PWM);

	vDRV8313StartInjected(DRV8313);		//启动注入组: PWM触发采样三相电流
	vDRV8313StartDMA(DRV8313);			//启动普通组+DMA: 循环采样母线电压
}


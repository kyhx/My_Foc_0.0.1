# My_Foc_0.0.1

基于 **STM32G431** 的 **三相无刷电机 FOC 磁场定向控制** 工程，采用 STM32CubeMX + CMake + Ninja 构建，代码按「BSP 驱动层 → Mid 中间层 → App 应用层」分层组织，支持**开环电压控制**与**FOC 电流闭环**两种模式，可通过按键与串口上位机实时控制与调试。

---

## 1. 功能特性

- **FOC 坐标变换**：三相 abc ⇄ αβ（Clarke）⇄ dq（Park），等幅（2/3）变换，提供带零序版本与逆变换。
- **SVPWM**：等效中心对齐调制（逆 Clarke + 零序中点注入），线性区最大相电压峰值为 $U_{dc}/\sqrt{3}$。
- **双控制模式**：
  - `OpenLoop` 开环电压控制（vd/vq），由主循环驱动，用于对相 / 扫频 / 开环拖动；
  - `Current` FOC 电流闭环（id/iq 双环 PI），由 PWM 同步 ADC 注入组中断（约 20 kHz）驱动。
- **电角度三种来源，支持运行时切换**：自动生成（频率扫频）、手动固定值（定位对相）、编码器（AS5047P 机械角 × 极对数，用于闭环）。
- **转子对齐**：闭环前自动执行编码器零位对齐（注入 d 轴电压锁转子 + 捕获零位）。
- **电流采样与保护**：LM324 运放放大三相低边采样，ADC 注入组在 PWM 峰 / 谷同步触发；含电流零偏校准、过流（9 A）与 DRV8313 硬件故障检测、故障锁存保护。
- **实时调试**：VOFA+ 上位机（JustFloat 波形 + String 协议）经串口上报 10 路信号，配合串口 ASCII 命令控制。
- **按键控制**：短按启动 / 长按急停（并清除故障锁存）。

---

## 2. 硬件平台

| 项目 | 说明 |
| --- | --- |
| MCU | STM32G431CBT6（Cortex-M4F @ 170 MHz，带 CORDIC/FPU） |
| 驱动芯片 | DRV8313（三相栅极驱动，低边电流采样） |
| 位置传感器 | AS5047P 磁编码器（SPI，14 bit） |
| 电流采样 | 10 mΩ 采样电阻 + LM324 运放（增益 16.5），三相低边采样 |
| PWM | TIM1 三相中心对齐（CH1/2/3），ARR=8400，开关频率 ≈ 10.1 kHz |
| 电机 | 极对数 7 的 BLDC/PMSM |
| 通信 | USART1 串口（命令 / VOFA+ 上报），SPI1（编码器） |

> 时钟：HSE + PLL，SYSCLK = 170 MHz；PCLK2 = 85 MHz；PCLK1 = 42.5 MHz。

---

## 3. 软件架构

采用经典分层结构，低层不依赖高层：

```
┌─────────────────────────────────────────────────────────┐
│ App 应用层        user / app_motor / oscilloscope        │
│                   (运行流程、电流环、示波器上报)          │
├─────────────────────────────────────────────────────────┤
│ Mid 中间层        foc / motor / comm / vofa             │
│                   (坐标变换、电角度、命令、上位机协议)    │
├─────────────────────────────────────────────────────────┤
│ BSP 驱动层        DRV8313 / AS5047 / pwm / uart /       │
│                   key / led / config                    │
├─────────────────────────────────────────────────────────┤
│ 底层              STM32CubeMX HAL + CMSIS + DSP Lib     │
└─────────────────────────────────────────────────────────┘
```

- **BSP 层**封装外设（DRV8313 驱动 + 电流采样、AS5047P 编码器、TIM1 PWM、UART、按键、LED），对外提供设备号枚举化接口。
- **Mid 层**提供 FOC 坐标变换、SVPWM、PID、电角度来源、串口命令与 VOFA+ 协议。
- **App 层**组织电机控制流程：初始化各驱动、编码器对齐、故障保护、电流环 ISR 与示波器周期上报。

---

## 4. 目录结构

```
My_Foc_0.0.1/
├── App/
│   ├── Inc/                  app_motor.h / motor_config.h / oscilloscope.h / top_config.h / user.h
│   └── Src/                  app_motor.c / oscilloscope.c / user.c
├── Mid/
│   ├── Inc/                  app_comm.h / foc.h / motor.h / vofa.h
│   └── Src/                  comm.c / foc.c / motor.c / vofa.c
├── BSP/
│   ├── Inc/                  bsp_AS5047 / DRV8313 / key / led / pwm / uart / config
│   └── Src/                  ...
├── Src/                      main.c 及 CubeMX 生成的外设代码
├── Inc/                      CubeMX 头文件
├── Drivers/                  CMSIS / STM32G4 HAL 库 / DSP 库
├── cmake/                    gcc-arm-none-eabi 工具链 / STM32CubeMX 子工程
├── CMakeLists.txt            顶层构建脚本（用户源码入口）
├── CMakePresets.json
├── My_Foc_0.0.1.ioc          STM32CubeMX 工程文件
└── startup_stm32g431xx.s / STM32G431xx_FLASH.ld
```

---

## 5. 关键模块说明

### 5.1 FOC 中间层（`Mid/foc`）
- `T_Clarke / T_Clarke2 / T_InvClarke`：Clarke 及逆变换（支持带零序）。
- `T_Park / T_InvPark`：Park 及逆变换。
- `SVPWM(pAb, Udc, &dA, &dB, &dC)`：输出三相占空比 0~1。
- `PID_Init / PID_Update / PID_Reset`：位置式 PID，带积分限幅（抗饱和）与反馈微分一阶滤波。

### 5.2 电角度模块（`Mid/motor`）
支持三来源并运行时切换：
| 来源 | 说明 |
| --- | --- |
| `Auto` 自动 | 按设定电频率匀速积分，用于开环对相 / 扫频 |
| `Manual` 固定 | 手动给定恒定电角度，用于定位验证 |
| `Encoder` 编码器 | 读 AS5047P 机械角 × 极对数（减零位偏移），用于闭环 |

含开环运行 `vMotorOpenLoopRun`（dq → 逆 Park → SVPWM → PWM 输出）与启停标志 `vMotorSetRun`。

### 5.3 电流闭环（`App/app_motor` + `App/motor_config`）
- 电流环在 **PWM 同步 ADC 注入组中断**（`HAL_ADCEx_InjectedConvCpltCallback` → 注册钩子）内执行，频率 ≈ 20 kHz。
- 每周期：采样三相电流 → Clarke → Park（编码器电角度）→ id/iq PI → 电压按 `0.9 × 实测母线电压` 自适应钳位 → 逆 Park → SVPWM → 更新 PWM 占空比。
- 整定参数集中在 `App/Inc/motor_config.h`（Kp/Ki、采样频率、限幅等）。

### 5.4 通信与上位机
- **串口 ASCII 命令**（`Mid/comm`，轮询 BSP_UART 接收标志，按行解析）：
  `help / angle / current / vd <V> / vq <V> / freq <Hz> / id <A> / iq <A> / foc / mode open|current / src auto|manual|encoder / start / stop / state`
- **VOFA+**（`Mid/vofa`）：JustFloat（浮点波形）+ String 协议，阻塞 / DMA / 中断三种发送方式。
- **示波器应用**（`App/oscilloscope`）：周期（10 ms）采集 10 路信号经 JustFloat 上报：Udc、Ib、Ic、Ia、电角度 θ、转子速度、θre、CCR1~CCR3。

---

## 6. 构建

依赖：CMake（≥ 3.22）、Ninja、`gnu-tools-for-stm32`（arm-none-eabi-gcc，如 14.3.1）。

在 VS Code 中使用 **CMake Tools**：

1. 选择工具链与预设（`CMakePresets.json`）；
2. 选择构建目标 `My_Foc_0.0.1` 并执行 Build。

> ⚠️ **注意**：本工程新增 `BSP/`、`App/` 源码后，**必须手动**将其加入根 `CMakeLists.txt` 的 `target_sources(...)` 列表（不会自动收集），否则链接时出现 `undefined reference`。

---

## 7. 使用流程

### 上电
`vUserInit()` 依次初始化 LED、按键、AS5047P、DRV8313、PWM、UART、示波器、串口命令、电角度与电流环。

### 启动 / 停止
- **短按按键**（或串口 `start`）：按当前选定来源启动。
- **编码器来源首启**：先自动执行转子对齐（注入 d 轴电压锁转子 800 ms → 捕获零位），随后以编码器电角度驱动。
- **长按按键**（或串口 `stop`）：停机并关断桥臂、清除故障锁存。
- **故障保护**：运行中发生过流或 DRV8313 硬件故障，立即停机并锁存，需长按清除。

### 开环 → 电流闭环
1. 默认 `OpenLoop` 开环模式完成对齐 / 对相 / 扫频；
2. 串口发送 `mode current` 切入电流闭环；
3. `iq <A>` 设定 q 轴电流（决定电磁转矩），`id <A>` 设定 d 轴电流（通常为 0）；
4. `start` 后按编码器电角度运行 FOC 电流环。

---

## 8. 主要调试参数

集中配置在以下文件，便于调试时统一修改：

| 文件 | 内容 |
| --- | --- |
| `BSP/Inc/bsp_config.h` | 采样电阻 / 运放增益、电流增益、母线增益、极对数、过流阈值、编码器分辨率、按键 / UART 配置 |
| `App/Inc/motor_config.h` | 电流环采样频率、Kp/Ki、输出 / 积分限幅、母线电压比例、电流参考限幅 |
| `App/Src/user.c` | 开环启动参数（电频率、vd/vq）、编码器对齐电压与锁定时间 |

---

## 9. 说明与待办

- 编码器来源的「启动」在无电流环时本质为**真实转子电角度下的开环 Vq 施加**（开环拖动），会持续加速直到过流 / 负载 / 手动停机；真正闭环请使用 `mode current` 电流环。
- 电流环 ISR 频率假设为 20 kHz（PWM 峰 / 谷各触发一次注入组），如与实测不符，请修正 `motor_config.h` 的 `CUR_LOOP_FS_HZ`。
- 当前实现速度环 / 位置环尚未接入，仅到电流环（内环）层级。
- 时钟、引脚与外设初始化由 STM32CubeMX（`.ioc`）生成，重生成后需在 `main.c` 的 `USER CODE` 段调用 `vUserInit()` 与 `vUserExecute()`。

---

*作者：可以航行 ｜ 版本：0.0.1*

# keil移植出现过的坑 — 索引与速查

> 本文件夹汇总了历次 Keil → VSCode(CMake+Ninja+GCC) 移植实战中踩过的所有坑。
> **遇到编译/构建/链接问题先查 01 总表；新坑一律追加进 01 总表对应章节。**

## 文件清单

| 文件 | 内容 |
|------|------|
| 01_嵌入式编译已知问题总表.md | ★权威总表：§1工具链 / §2移植脚本 / §3 CubeMX / §4链接 / §5代码层 / §6流程，含症状关键词索引 |
| 02_Keil2VSCode移植后补全清单.md | 脚本跑完后必做的手动补全项（GCC启动文件、Project Setup注册、GCC14 三件套 -Wno-） |
| 03_Keil2VSCode脚本勘误手册.docx | keil2cubevscode 脚本官方勘误（输出前缀含义、flash.jlink 相对路径、DRM加密区、ps1 编码等） |
| 04_正点原子BSP的GCC编译三坑.md | 启动文件来源（先查 gcc_ride7 再拷仓库）、CMSIS 头补齐判定、__asm void MSR_MSP 改写、GBK 处理 |
| 05_F1系列迁移工程陷阱.md | SWJ_DISABLE 调试口自杀、SysTick 延时关而不开、封装引脚缺失静默失败等 F1 专属坑 |
| 06_GBK中文注释批量清除.md | 中文注释清除状态机脚本 + 只读属性 .ps1 处理 + 验证三板斧 |


## 本板（Standard_NUEDC_DevBoard, STM32F103VET6）移植前必看

### 1. 芯片参数速查（VET6 = 高密度 HD）

- 启动文件：`startup_stm32f103xe.s`（xe=高密度 RC/VC/VE 256K+；勿用 xb 中密度版）
- 芯片宏：`STM32F103xE`（标准库老工程则是 `STM32F10X_HD` + `USE_STDPERIPH_DRIVER`）
- .ld 手填值：FLASH `0x08000000 / 512K`，RAM `0x20000000 / 64K`
  （脚本生成的 .ld 几乎必然是 0K 占位——总表 §2，每个新工程都要手改）
- CPU 架构：`-mcpu=cortex-m3 -mthumb`（脚本对某些系列会写错成 m0，核对 gcc-arm-none-eabi.cmake 的 TARGET_FLAGS）

### 2. 脚本运行后必查清单（按序）

1. .ld 的 Flash/RAM 是否为 0K 占位 → 手填上面的值
2. 芯片宏是否与 uvprojx `<Define>` 一致（别信脚本抄的 Device 名）
3. `startup/` 下有无 0 字节或缺失的 GCC 启动文件 → 见 04 文档
4. GCC 14 把 implicit-declaration / incompatible-pointer-types 升级为错误 →
   cmake/stm32cubemx/CMakeLists.txt 的 INTERFACE 编译选项加
   `-Wno-implicit-function-declaration -Wno-incompatible-pointer-types -Wno-implicit-int`
5. 多 target 的 uvprojx 脚本只读第一个 target → 移植前核对真板芯片与首 target 是否一致
6. 工程目录里有无 0 字节影子头文件（stdint.h/string.h）劫持系统头 → 见总表 §7
7. CMake 工程名/目标名禁止中文（ASCII only）
8. nano.specs/nosys.specs 勿重复添加（工具链文件已带，重复会报 rename spec 错误）
9. 改 CMakeLists.txt 后必须重新 configure 再 build

### 3. 板载外设注意点（来自 BOM）

- 串口芯片是 **CH340C**（内置晶振版），USB 转串口；下载也可走 USART1（BOOT0 跳线）
- 主晶振 8MHz（YXC XL2EL89CSI-111YLC-8M），LSE 32.768kHz 晶振（泰晶 SF32WK32768D31T002）
  → CubeMX 时钟树 HSE_VALUE 必须是 8000000，三处同步（main.c PLL 配置 / hal_conf.h / .ioc）
- LED 为 GL0603 系列，注意原理图确认极性（大概率低电平点亮）
- 按键 KH-6X6X5H-STM 轻触开关，注意上/下拉与有效电平匹配（PULLUP→检测RESET / PULLDOWN→检测SET）
- Type-C 口 CC1/CC2 各有 5.1kΩ 下拉（R1/R2/R13/R14），是 UFP 从设备识别，正常

### 4. 烧录与调试（J-Link V966）

- SWD 接线：33=SWDIO / 35=SWCLK / 6=NRST，**VTref 必接 3V3**
- 命令行烧录：`JLink.exe` 管道 `printf 'loadfile x.hex\nr\ng\nqc'`，`-speed 1000` 才稳
- 迁移工程调试**不要用** launch.json 里的 `jlinkgdbtarget` 配置（会满屏 0xDEADBEEF），
  一律用 cortex-debug 三件套：`serverpath` 绝对路径 + `armToolchainPath` 必填 +
  `preLaunchTask` 对应 tasks.json label；模板见技能 templates/cortex-debug-launch.json
- 0xDEADBEEF 只证明 halt 不了 CPU，先用 J-Link Commander 分层定位，别急着怪扩展
- flash.jlink 里 `loadfile` 必须相对路径，tasks.json 的 J-Link 任务设 `"cwd": "${workspaceFolder}"`

### 5. 代码层高频坑（编译能过、上板才炸）

- printf 重定向：GCC newlib 用 `int _write(int, char*, int)`，**不是** Keil 的 `__io_putchar`（写错则静默无输出）
- `%f` 打印需链接加 `-u _printf_float`
- ISR 写、主循环读的全局变量必须 `volatile`（定义和 extern 都要），否则 -O2 下优化掉
- F1 工程 hal_msp.c 里出现 `__HAL_AFIO_REMAP_SWJ_DISABLE()` 立即注释掉（自杀式关调试口，见 05）
- CubeMX 手动用了未配置的外设 → 必须去 hal_conf.h 取消注释对应 `HAL_XXX_MODULE_ENABLED`
- CubeMX 重新生成会覆盖 USER CODE 区外的所有代码；注释清理时「只含说明注释」的 USER CODE 块必须保留
- 移植/大改后写代码注释一律按《个人编程代码规范》执行

### 6. 工程版本管理习惯

- 大改前 `git commit` 打基线，验证通过即提交（阶段式存档，可随时回退）
- 编译绿灯交给用户在 VSCode 里 Build 验证；AI 默认只审查不编译（整文件重写除外）

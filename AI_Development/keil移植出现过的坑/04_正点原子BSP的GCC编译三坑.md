# Keil→GCC 移植补充陷阱（2026-08-20 正点原子官方 LED 工程实测）

> 适用：keil2cubevscode 迁移正点原子等厂商 BSP 工程到 VSCode+GCC。
> 前置：SKILL.md「keil2cubevscode 脚本已知问题」第 1-3 条（.ld 占位值、launch.json、CubeMX CMake）。

## 1. GCC 启动文件从哪里找（脚本报「未找到 GCC 语法启动文件」时）

脚本只识别工程内已有的 GCC 启动文件，正点原子工程 CORE/ 里只有 ARMCC 语法的
startup_stm32f103xe.s（注释以 `;` 开头，GCC 编不了）。三个可靠来源，按优先级：

1. 用户已有 CubeMX 生成工程的 `Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/`
2. 固件仓库 `~/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.7/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/`
3. Cube 扩展 bundles（AppData\Local\stm32cube）

**型号选择：** RC(256K)/RD/RE 都用 `startup_stm32f103xe.s`（xE 覆盖高密度三档）；
x6=低密度 xB=中密度 xG=超大密度。

**做法：** 拷贝进工程（如 `CORE/startup_stm32f103xe_gcc.s`）保持自包含，跨盘/跨机
引用必炸；然后填 `cmake/stm32cubemx/CMakeLists.txt`：

```cmake
set(MX_Startup
    ${CMAKE_SOURCE_DIR}/CORE/startup_stm32f103xe_gcc.s
)
```

**验证是 GCC 语法：** `grep -c "syntax unified" <file>` 返回 1（ARMCC 版没有）。
根 CMakeLists 必须有 `enable_language(C ASM)`。

## 2. cmsis_gcc.h 缺失（fatal error: cmsis_gcc.h: No such file or directory）

正点原子 CORE/ 只带 ARMCC 系 CMSIS 内核头（cmsis_armcc.h/cmsis_armclang.h/
cmsis_compiler.h/core_cm3.h），GCC 编译走到 `cmsis_compiler.h:48 #include
"cmsis_gcc.h"` 时炸。修复：从固件仓库 `Drivers/CMSIS/Include/` 拷两个文件进 CORE/：

- `cmsis_gcc.h`
- `cmsis_version.h`（cmsis_gcc.h 会引用）

先 `diff -q` 确认工程内 core_cm3.h 与仓库版本差异——正点原子这套就是 CMSIS 5 系
精简版，与 F1 仓库 V1.8.7 同源，直接补文件即可，不用整目录替换。

## 3. `__asm void` ARMCC 专属函数语法（sys.c 的 MSR_MSP）

正点原子 SYSTEM/sys/sys.c 尾部的设置主堆栈指针函数，GCC 报
`error: expected '(' before 'void'`。Keil 允许整个函数用汇编写；GCC 必须改成
扩展内联汇编的普通函数：

```c
/* 原 ARMCC 写法（GCC 编不了）：
__asm void MSR_MSP(u32 addr)
{
    MSR MSP, r0
    BX r14
} */
void MSR_MSP(u32 addr)
{
    __ASM volatile("msr msp, %0" : : "r" (addr) : "memory");
}
```

sys.h 里的普通原型声明 `void MSR_MSP(u32 addr);` 不用动。同文件 WFI_SET/
INTX_DISABLE/INTX_ENABLE 用的 `__ASM volatile("...")` 单行形式 GCC 天然兼容。

**注意：** 该函数若全工程无人调用，`--gc-sections` 会把它裁掉，nm 查不到符号
属正常——编译零错误即证明改写正确（错误的内联汇编约束编译期就报）。

## 4. GBK 编码源文件（正点原子全家桶常态）

正点原子的 .c/.h/.bat 多为 GBK+CR LF。症状与处理：

- read_file 报 "Binary file (unknown binary)" → 是 GBK 中文注释
- 读：`iconv -f GBK -t UTF-8 <file>`
- 改：用 patch 只改纯 ASCII 区域（安全，GBK 字节流不碰）
- **中文注释清除（用户标准要求）：** VSCode 打开乱码，用户明确要求移植后代码
  不留任何中文注释——完整状态机脚本与验证流程见 `references/strip-cn-comments.md`
  （2026-08-20 起 keil-gcc-porting 的收尾步骤，不再只是"转码保留"）
- 验尸 bat 脚本同样 GBK：keilkill.bat 是正点原子祖传清理工（递归删 *.o/*.d/
  *.axf/*.map 等编译产物，特意保留 .opt 保 JLink 设置），双击运行安全，
  勿在命令行其他目录执行

## 5. 在 Keil 侧编译正点原子老工程（前置检查）

迁移前若想在 Keil 验证原工程可编译：

- 工程 `<uAC6>0</uAC6>` → 用 AC5（armcc）。MDK 5.36 仍自带 AC5：查
  `C:\Keil_v5\ARM\ARMCC\bin\armcc.exe` 存在且 `armcc.exe`（无参）报
  "ARM Compiler 5.06 update 7" 即可用
- Debug/Utilities 页的调试器工程默认可能是 ST-LINK，用 J-Link 时两页都要改
  J-LINK/J-TRACE Cortex-M（Debug 页 Port 选 SW；Utilities 页 Flash Download
  勾 Reset and Run）
- F103RC 用 FLM `STM32F10x_512` 高密度驱动、Device 选 STM32F103RC（DFP 2.2.0）

## 6. GCC 14 兼容性三件套（configure 过 ≠ 能编，2026-08-21 uCOS-II 控制板工程实测）

GCC 14（CubeMX bundle 14.3.1 自带）把 `-Wincompatible-pointer-types`、
`-Wimplicit-function-declaration`、`-Wimplicit-int` 从警告升级为**默认错误**。
老 Keil/ARMCC 代码（宽松模式下写的）一编就炸，典型报错：

```
error: passing argument 2 of 'ReadFlash_4xBye' from incompatible pointer type [-Wincompatible-pointer-types]
note: expected 'uint32_t *' {aka 'long unsigned int *'} but argument is 'unsigned int *'
```

注意第二层坑：newlib 里 `uint32_t` 是 `long unsigned int`，而结构体成员常声明成
裸 `unsigned int`，Keil 下相安无事，GCC 14 判指针类型不兼容。

**修复（不碰固件源码）：** 在 `cmake/stm32cubemx/CMakeLists.txt` 的
`target_compile_options(stm32cubemx INTERFACE ...)` 里加三行，等效 Keil 时代宽松行为：

```cmake
-Wno-incompatible-pointer-types
-Wno-implicit-function-declaration
-Wno-implicit-int
```

建议写进移植脚本的生成模板，以后移植时就不会重复踩。

**诊断要点：configure 成功什么也没证明。** `cmake --preset` 只检查 CMake 语法和
工具链探测，不检查源文件存在性、宏定义、类型匹配——这些全部要等 ninja build
才暴露。用户说"CMake 生成失败"时，先亲自跑 `cmake --preset X && cmake --build build/X`
区分是 configure 还是 build 阶段失败，再看 build/ 目录里已有的 .ninja_log 痕迹。

## 7. 无板子验证流程（编译链接不需要硬件）

移植验证三层，只有第三层需要板子：

1. `cmake --preset Debug/Release` → 验证脚本结构/工具链探测（含金量低）
2. `cmake --build build/<preset>` → 真正暴露移植问题（源文件清单、include、
   启动文件、链接脚本、GCC14 兼容）——无板可做，必须做到全绿 + .elf 产出
3. F5 下载/调试 → 唯一需要板子；无板时只静态检查 launch.json

**交付清单（无板时）：** 编译通过输出 + Memory region 占用表（证明链接脚本与
芯片型号匹配）+ 修过的坑清单 + 注明"未做硬件验证，F5/下载待有板子后实测"。
链接期 `_close/_read/_write is not implemented` 警告是 nosys 弱桩正常提示，无害。

## 8. VCT6 高密度工程二次实战（2026-08-22，含两个新坑）

### 8.1 工程自带 gcc_ride7 启动文件——先查再拷

V3.5 标准外设库的老工程（非正点原子 BSP）`Libraries/CMSIS/CM3/DeviceSupport/ST/
STM32F10x/startup/` 下常自带 **gcc_ride7/** 目录（RIDE7 工具链用，也是 GCC 语法，
按密度分文件：ld/md/hd/xl/cl + _vl 变体），脚本会自动识别并填 MX_Startup。
动手拷固件仓库前先 `find . -type d -name "gcc_ride7"`。验证：`grep -c "syntax
unified"` 且向量数 180（hd 版 60 个外设中断）。

### 8.2 CMSIS 版本决定要不要补 cmsis_gcc.h（别把 3.x 流程套到 1.x）

- **CMSIS 1.x**（V3.5 库自带，core_cm3.h/core_cm3.c 头注 @version V1.30）：
  `__GNUC__` 分支自成一体（static __INLINE 的 __enable_irq/__NOP/__WFI + extern
  naked 函数），**不需要** cmsis_gcc.h/cmsis_version.h
- **CMSIS 3.x/5 系**（core_cm3.h 引用 cmsis_compiler.h→cmsis_gcc.h）：才要补头

判定：看 core_cm3.h 开头 @version，或直接编译看是否报缺头。第 2 节的补头流程
只适用于 CMSIS 3.x/5 系工程。

### 8.3 多 target uvprojx：脚本只读第一个 target

.uvprojx 可含多个 target（实测一例：Flash=F103RB、CpuRAM/ExtSRAM=F103ZE+
VECT_TAB_SRAM），脚本只按**第一个 target**生成 Device/宏/内存。真板子与首 target
不符时三处全错：密度宏（MD≠HD）、.ld 内存（128K/20K≠256K/48K）、调试 device
（RB≠VC，J-Link 按它选 flash 算法）。移植前必查：

```python
targets = re.findall(r'<Target>.*?<TargetName>([^<]*)</TargetName>(.*?)</Target>', xml, re.S)
# 每个 target 抽 <Device> 与 <Define> 配对，与真板子核对
```

**密度宏判定的硬证据：** stm32f10x_it.c 里出现的 HD-only 中断（TIM8_BRK/UP/TRG_COM/
CC、TIM5、UART4 的 IRQHandler，及函数体直接用 UART4 外设）——这些 IRQn 枚举和
外设声明只在 HD+ 分支存在，MD 宏下必炸。注意 grep 源文件时 GBK 内容会让 grep
报 "Binary file matches"，加 `-a` 或用 `grep -c` 计数规避。

### 8.4 空壳"影子头文件"劫持系统头（VSCode 误创建）

现象：uint8_t 全未定义、memset 隐式声明，错误横跨几十个文件。根因：-I 目录
（User/app 等）下出现 **0 字节的 stdint.h/string.h**——GCC 的 -I 优先于工具链
系统头目录，尖括号引号通杀。来源：VSCode 里点开头文件解析问题时误点"创建文件"。

诊断（mtime 扫描，找既不属于原始包也不属于脚本生成时刻的文件）：

```bash
find . -type f \( -name "*.h" -o -name "*.c" \) -not -path "*/build/*" |
while read f; do t=$(stat -c %y "$f" | cut -c1-16)
  case "$t" in 2011-*|2020-*) ;; <脚本运行时刻>*) ;; *) echo "$t ${f#$(pwd)/}";; esac
done
```

修复：直接删（0 字节零损失）。预防：include 目录永不放与系统头同名的文件。

### 8.5 修复→编译全流程凭据（供对照）

修复清单（宏 MD→HD、.ld 256K/48K、-Wno 三件套、device RB→VC×3 文件、cortex-debug
补绝对 serverpath/armToolchainPath/preLaunchTask）落盘后：`rm -rf build/Debug`
全量重建 **38/38 一次绿**，FLASH 40540B/256K=15.46%（HD 比 MD 多编约 13K 外设
支持代码，涨量正常勿慌）、RAM 1768B/48K=3.60%，elf/hex/bin 齐。用户实测烧录
VCT6 核心板成功——代码审查(只读)→修复→编译→上机四段闭环。

### 8.6 经验沉淀已开源

本次方法论已脱敏发布为公开仓库
`https://github.com/SunBinyuan666/keil2vscode-porting-notes`（README+九步清单+
8 坑详解）。后续新坑先补本文件，再同步脱敏版到该仓库 docs/pitfalls.md。

**发布/同步前必做敏感词扫描（脱敏纪律）：** 只发布通用方法论，内部
手册/脚本/勘误文档的内容用自述语言重写而非搬运；机构名、工程代号、内网
地址、工作邮箱、个人盘符路径一律不出现（配置示例用 `<user>` 占位）。推送前在
本地 clone 上扫描，推送后用 contents API 核对文件清单 + repo 元数据核对可见性：

```bash
grep -rinE "机构名|工程代号|内网IP段|[a-z]+@工作域名|[A-Z]:\\\\" README.md docs/ \
  && echo "SENSITIVE RESIDUE — fix before push" || echo "clean"
```

## 排障顺序总结（实测一次通过链）

```
脚本迁移 → 填 MX_Startup(拷 GCC 启动文件) → ninja 报 cmsis_gcc.h 缺
→ 从仓库补 cmsis_gcc.h+cmsis_version.h → ninja 报 __asm void
→ 改写 MSR_MSP → 19/19 全绿（FLASH 2.76% / RAM 2.73%）
→ 清除 GBK 中文注释（strip-cn-comments.md）→ 重编验证 + 非ASCII零残留
```

每次只修一类报错重编（串行推进，GLM 5.3 并发限制=1 尤其注意不要并行
工具调用），ninja 增量编译足够快，不必 rebuild。

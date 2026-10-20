# Playbook: Keil→VSCode 移植后手动补全清单

> 适用: keil2cubevscode.ps1 脚本生成的工程（如 LED_vscode）
> 背景: 实测移植步骤；脚本输出后需人工补全以下各项

## 1. MX_Startup 启动文件路径（脚本必留坑）

脚本会提示: [!] 未找到 GCC 语法启动文件，请手动填写 cmake/stm32cubemx/CMakeLists.txt 中的 MX_Startup

步骤:
1. 从 CubeMX 固件仓库拷 GCC 版启动文件（按芯片容量选后缀）:
   源: C:\Users\LENOVO\STM32Cube\Repository\STM32Cube_FW_F1_V1.8.7\
       Drivers\CMSIS\Device\ST\STM32F1xx\Source\Templates\gcc\startup_stm32f103xe.s
   （RC/RD/RE=高密度用 xe；C8T6=中密度用 xb；R8=xb；VE=xe）
   拷到工程 CORE\ 目录（改名加 _gcc 后缀避免与 ARMCC 版冲突）
2. 验证是 GCC 语法: 文件内含 ".syntax unified" 和 `__attribute__`，
   ARMCC 版特征是分号注释 + AREA 指令，GCC 编不了
3. 填入 cmake/stm32cubemx/CMakeLists.txt:
   set(MX_Startup
       ${CMAKE_SOURCE_DIR}/CORE/startup_stm32f103xe_gcc.s
   )

## 2. CORE 目录补 GCC 版 CMSIS 头

正点原子 CORE 只有 ARMCC 版。编译报 core_cm3.h 找不到 cmsis_gcc.h 时:
从 F1 仓库 Drivers\CMSIS\Include\ 拷 cmsis_gcc.h + cmsis_version.h 到 CORE\
（其余 core_cm3.h 等正点原子已有，无需覆盖）

## 3. ARMCC 专属语法改写

sys.c 的 `__asm void MSR_MSP(u32 addr)` → GCC 内联汇编:
    void MSR_MSP(u32 addr)
    {
        __ASM volatile("msr msp, %0" : : "r" (addr) : "memory");
    }
同族地雷: WFI_SET/INTX_DISABLE/INTX_ENABLE 若也是 __asm void 写法需同样改写

## 4. launch.json 缺失的配置

若 F5 报 'cube' command is not available in current context (PATH not set):
→ VSCode Ctrl+Shift+P → STM32Cube: Configure/Project Setup → 选芯片 →
  Toolchain: GCC → Finish（让扩展注册项目并注入环境）

标准 launch.json 三配置（好工程 Basic_Timer_Test 同款）:
- "STM32Cube: Launch JLink GDB Server" (type: jlinkgdbtarget, preBuild=
  ${command:st-stm32-ide-debug-launch.build}, runEntry main)
- "Debug (J-Link)" (cortex-debug, servertype jlink, executable 指向
  build/Debug/xxx.elf, device 如 STM32F103RC, interface SWD, speed 4000,
  runToEntryPoint main, serverpath JLinkGDBServerCL)
- "STM32Cube: ST-Link Debug" (stlinkgdbtarget, imageFileName 指 build/Debug/xxx.elf)

## 5. 中文注释乱码（可选）

VSCode 打开乱码 = GBK 编码。状态机脚本清除含非 ASCII 的注释并转 UTF-8
（纯 ASCII 注释保留；只读文件先 attrib -r 或 PowerShell IsReadOnly=false）

## 6. GCC 14 兼容性三件套（configure 过 ≠ 能编，真正的坑在 build 阶段）

GCC 14（CubeMX bundle 14.3.1 自带）把以下三类从警告升级为默认错误，
老 Keil/ARMCC 代码一编就炸，典型报错:
  error: passing argument N of 'xxx' from incompatible pointer type [-Wincompatible-pointer-types]

修复: 在 cmake/stm32cubemx/CMakeLists.txt 的 target_compile_options(stm32cubemx INTERFACE ...) 里加:
    -Wno-incompatible-pointer-types
    -Wno-implicit-function-declaration
    -Wno-implicit-int
等效 Keil 时代的宽松行为，不碰固件源码。建议直接写进移植脚本生成模板。
注意: configure 阶段只检查 CMake 语法和工具链，源文件缺失/宏错误/类型错误
全部要等 build 才暴露——验证移植必须跑到出 .elf 为止。

## 7. 无板子验证流程（编译链接不需要硬件）

移植验证三层，只有第三层需要板子:
1. cmake --preset Debug/Release   → 验证脚本结构/工具链探测（含金量低）
2. cmake --build build/<preset>   → 真正暴露移植问题（源文件清单、include、
   启动文件、链接脚本、GCC14 兼容）——无板可做，必须做到 38/38 通过
3. F5 下载/调试                    → 唯一需要板子；无板时只静态检查 launch.json

交付清单（无板时）:
- 编译通过输出 + Memory region 占用表（证明链接脚本和芯片型号匹配）
- 修过的坑清单
- 注明"未做硬件验证，F5/下载待有板子后实测"

## 8. 编译验证

裸终端验证（不依赖 VSCode 扩展）:
export PATH 注入两个目录:
  core 扩展 resources\binaries\win32\x86_64      (cube.exe)
  build-cmake 扩展 resources\cube-cmake\win32\x86_64  (cube-cmake.exe)
然后: cube-cmake --build --preset Debug --parallel
成功标志: .elf/.hex/.bin 产出 + Memory region 表打印

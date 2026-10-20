# Standard NUEDC DevBoard（电赛标准开发板）

自绘的全国大学生电子设计竞赛（NUEDC）通用开发板，主控 **STM32F103VET6**（LQFP100，Flash 512K / RAM 64K），板版本 V1.0（2026-10 打板）。

> [!WARNING]
> 首版板子未经长期验证，**可能存在错误，请谨慎使用**（同目录下的原始声明文件保留）。

## 仓库结构

| 目录 | 内容 |
|---|---|
| `PCB工程文件/` | 立创EDA专业版工程（.epro2）、BOM、网表、PCB PDF |
| `原理图/` | 原理图 PDF（3 页：主控 / 电源 / 接口） |
| `Code_Demo/` | 5 个 VSCode + CMake + arm-none-eabi-gcc 示例工程（含 HAL 库，克隆即编译） |
| `AI_Development/` | 全部元器件引脚连接方式文档、个人 C 编程规范、Keil 移植踩坑笔记 |
| `keil移植脚本/` | keil2cubevscode 脚本（bat / ps1） |

硬件数据手册因体积与版权原因不上传，元器件型号见 BOM（`PCB工程文件/BOM_*.xlsx`），手册请到立创商城 / 厂商官网查询。

## 板载资源

- 主控：STM32F103VET6，外部晶振
- 电源：DC 输入（SS54 防反接 + 自恢复保险丝）→ 降压（Buck，15 µH）→ 3.3 V；Type-C 供电可选
- LED / 按键 / 调试排针等基础外设（板上外设走精简路线，详见原理图）
- 完整引脚网络对照见 `AI_Development/所有元器件的连接方式.md`（原理图 PDF × 网表 TEL 双重核对生成）

## 示例工程

| 工程 | 说明 |
|---|---|
| `KEY_LED` | 按键控制 LED |
| `LED_HEARTBEAT` | 心跳灯 |
| `PWM_LED` | PWM 呼吸灯 |
| `USART1_KEY` | 串口 + 按键 |
| `IIC_TEST` | I2C 外设测试 |

工具链：VSCode + STM32Cube 插件 + CMake + Ninja + arm-none-eabi-gcc（不依赖 Keil；`keil移植脚本/` 用于把 Keil 工程转成该流程）。`build/` 目录不入库，进入工程目录 configure 一次即可重新生成。

烧录：J-Link SWD（工程内附 .jlink 脚本）。

## 开发规范

本仓库代码遵循 `AI_Development/个人编程代码规范.md`（由公司规范脱敏而来的个人版，对齐 MISRA C:2023 / BARR-C:2018）。

## License

MIT。

/***********************************************************************************
 * @file      app_hcsr04.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V2.0.0
 * @date      2026-11-11
 * @brief     HC-SR04超声波测距模块对外接口(2020款IIC模式)
 * @history   V1.0.0 首版: 触发/轮询/取值接口与无效值定义        ---2026-11-11
 *           V2.0.0 改硬件IIC: TIM4输入捕获换I2C2(PB10/PB11)     ---2026-11-11
 **********************************************************************************/
#ifndef APP_HCSR04_H
#define APP_HCSR04_H

#include "stm32f1xx_hal.h"
#include "i2c.h"

/* 2020款HC-SR04用户手册(IIC模式): 从机地址0x57, 写0xAE/读0xAF */
#define HCSR04_I2C_ADDR_WR      0xAEU  /* 0x57<<1 | 0 */
#define HCSR04_I2C_ADDR_RD      0xAFU  /* 0x57<<1 | 1 */
#define HCSR04_I2C_CMD_START    0x01U  /* 启动测距命令 */

#define HCSR04_I2C_XFER_TIMEOUT_MS  20U  /* 单次IIC传输超时(ms) */
#define HCSR04_ECHO_TIMEOUT_MS     250U  /* 触发到可读取上限(ms): 手册200ms+余量 */
#define HCSR04_TRIG_INTERVAL_MS    200U  /* 两次测距最小间隔(ms), 手册要求 */
#define HCSR04_DIST_INVALID        999U  /* 无有效距离时的哨兵值(cm) */

/* app_hcsr04_poll 返回值 */
#define HCSR04_POLL_BUSY   0U           /* 测量进行中(未到读取时刻) */
#define HCSR04_POLL_DONE   1U           /* 本轮测量完成(结果可能有效) */
#define HCSR04_POLL_TIMEOUT 2U          /* IIC通信失败/超时, 结果无效 */

/*************************************
 * 函数名称 ： app_hcsr04_init
 * 描述     ： 超声波模块初始化, 探测IIC总线上的模块是否在线
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 0=模块应答正常 1=无应答(检查接线/模式电阻/上拉)
 **************************************/
uint8_t app_hcsr04_init(void);

/*************************************
 * 函数名称 ： app_hcsr04_trigger
 * 描述     ： 通过IIC写入启动测距命令(0x01), 开始新一轮测距;
 *             两次调用间隔须>=200ms(由上层状态机保证)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_hcsr04_trigger(void);

/*************************************
 * 函数名称 ： app_hcsr04_poll
 * 描述     ： 主循环轮询当前测量状态(非阻塞): 触发后等待
 *             HCSR04_ECHO_TIMEOUT_MS 再通过IIC读回3字节距离
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： HCSR04_POLL_BUSY/DONE/TIMEOUT
 **************************************/
uint8_t app_hcsr04_poll(void);

/*************************************
 * 函数名称 ： app_hcsr04_get_distance
 * 描述     ： 取最近一次有效测距结果(cm)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 距离(cm), 无有效值返回 HCSR04_DIST_INVALID
 **************************************/
uint32_t app_hcsr04_get_distance(void);

#endif /* APP_HCSR04_H */

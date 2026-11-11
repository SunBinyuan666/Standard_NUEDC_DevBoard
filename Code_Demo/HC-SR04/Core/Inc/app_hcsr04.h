/***********************************************************************************
 * @file      app_hcsr04.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     HC-SR04超声波测距模块对外接口
 * @history   V1.0.0 首版: 触发/轮询/取值接口与无效值定义        ---2026-11-11
 **********************************************************************************/
#ifndef APP_HCSR04_H
#define APP_HCSR04_H

#include "stm32f1xx_hal.h"
#include "tim.h"

#define HCSR04_ECHO_TIMEOUT_MS   30U    /* 回波超时(ms), 设计要求§5.3 */
#define HCSR04_DIST_INVALID     999U    /* 无有效距离时的哨兵值(cm) */

/* app_hcsr04_poll 返回值 */
#define HCSR04_POLL_BUSY   0U           /* 测量进行中 */
#define HCSR04_POLL_DONE   1U           /* 本轮测量完成(结果可能有效) */
#define HCSR04_POLL_TIMEOUT 2U          /* 超时/无回波, 结果无效 */

/*************************************
 * 函数名称 ： app_hcsr04_init
 * 描述     ： 超声波模块初始化, 启动TIM4_CH4输入捕获中断
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_hcsr04_init(void);

/*************************************
 * 函数名称 ： app_hcsr04_trigger
 * 描述     ： 发出一次TRIG脉冲(>=10us高), 开始新一轮测距;
 *             两次调用间隔须>=60ms(由上层状态机保证)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_hcsr04_trigger(void);

/*************************************
 * 函数名称 ： app_hcsr04_poll
 * 描述     ： 主循环轮询当前测量状态(非阻塞)
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

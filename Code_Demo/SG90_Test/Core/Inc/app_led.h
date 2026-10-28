/*******************************************************************************
 * @file      app_led.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-20
 * @brief     LED指示模块: LED3(PB6)细调/LED4(PB7)粗调互斥指示,
 *           LED5(PD8)以500ms间隔翻转作系统心跳, 均低电平点亮

 * @note     心跳翻转由本模块周期函数内部定时, 主循环只管调用
 * @history   V1.0.0 首版: 模式指示互斥+500ms心跳              ---2026-10-20
 ******************************************************************************/
#ifndef APP_LED_H
#define APP_LED_H

#include "stm32f1xx_hal.h"

#define APP_LED_MODE_FINE     0U  /* 细调模式(1度/步), 点亮LED3 */
#define APP_LED_MODE_COARSE   1U  /* 粗调模式(5度/步), 点亮LED4 */

/*************************************
 * 函数名称 ： app_led_set_mode
 * 描述     ： 设置调节模式指示灯: 细调亮LED3灭LED4,
 *             粗调亮LED4灭LED3, 两灯互斥
 * 输入     ： mode - APP_LED_MODE_FINE或APP_LED_MODE_COARSE
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_set_mode(uint8_t mode);

/*************************************
 * 函数名称 ： app_led_heartbeat
 * 描述     ： 心跳处理, 以500ms间隔翻转LED5, 需在主循环持续调用,
 *             非阻塞(内部HAL_GetTick门控)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_heartbeat(void);

#endif /* APP_LED_H */

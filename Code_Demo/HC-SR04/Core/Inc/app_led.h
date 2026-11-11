/***********************************************************************************
 * @file      app_led.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     板载LED状态指示模块对外接口
 * @history   V1.0.0 首版: 状态枚举+方向提示参数                  ---2026-11-11
 **********************************************************************************/
#ifndef APP_LED_H
#define APP_LED_H

#include "stm32f1xx_hal.h"
#include "app_obstacle_scan.h"

/* LED显示状态(设计要求§8.1编码表) */
typedef enum
{
    LED_STATE_NORMAL = 0,      /* 正常监测/中位畅通, 心跳1Hz */
    LED_STATE_POINT_LEFT,      /* 已选左侧, LED3亮 */
    LED_STATE_POINT_RIGHT,     /* 已选右侧, LED4亮 */
    LED_STATE_SCANNING,        /* 正在扫描, LED5约2Hz */
    LED_STATE_NO_WAY,          /* 无安全方向, LED5约4Hz */
    LED_STATE_FAULT,           /* 传感器故障, LED5约4Hz */
    LED_STATE_PAUSED           /* 人工暂停, LED5慢闪 */
} led_state_t;

/*************************************
 * 函数名称 ： app_led_init
 * 描述     ： LED模块初始化, 全部熄灭(输出高)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_init(void);

/*************************************
 * 函数名称 ： app_led_set_state
 * 描述     ： 设置状态灯模式; 扫描态需同时给出待测方向
 * 输入     ： state - 显示状态
 *             scan_dir - 扫描方向提示(SCAN_DIR_xxx, 仅扫描态有效)
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_set_state(led_state_t state, uint8_t scan_dir);

/*************************************
 * 函数名称 ： app_led_process
 * 描述     ： 主循环周期调用, 按状态驱动LED(tick门控, 无阻塞)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_process(void);

#endif /* APP_LED_H */

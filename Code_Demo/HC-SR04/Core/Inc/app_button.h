/***********************************************************************************
 * @file      app_button.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     三按键扫描模块对外接口
 * @history   V1.0.0 首版: 按键编号枚举+事件查询接口            ---2026-11-11
 **********************************************************************************/
#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#include "stm32f1xx_hal.h"
#include "main.h"

/* 按键编号(板载丝印 SW2/SW3/SW4) */
typedef enum
{
    BUTTON_ID_SW2 = 0,      /* PE13 暂停/恢复 */
    BUTTON_ID_SW3,          /* PE14 左微调5度 */
    BUTTON_ID_SW4,          /* PE15 右微调5度 */
    BUTTON_ID_MAX
} button_id_t;

/*************************************
 * 函数名称 ： app_button_init
 * 描述     ： 按键模块初始化, 清零消抖状态与事件
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_button_init(void);

/*************************************
 * 函数名称 ： app_button_process
 * 描述     ： 主循环周期调用; tick门控采样+计数消抖+单事件
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_button_process(void);

/*************************************
 * 函数名称 ： app_button_get_event
 * 描述     ： 查询并取走一次按键事件(读清)
 * 输入     ： id - 按键编号
 * 输出     ： 无
 * 返回     ： 1=有一次稳定按下事件, 0=无
 **************************************/
uint8_t app_button_get_event(button_id_t id);

#endif /* APP_BUTTON_H */

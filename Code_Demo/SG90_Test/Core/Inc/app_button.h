/*******************************************************************************
 * @file      app_button.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-20
 * @brief     按键扫描模块: 3按键(PE13/14/15, 按下接地, 内部上拉),
 *           非阻塞周期采样消抖, 单次按下只产生一次事件

 * @note     事件用位域返回: bit0=SW2减小 bit1=SW3增大 bit2=SW4模式,
 *           多键同时按下不产生任何事件(避免不确定角度命令)
 * @history   V1.0.0 首版: 周期采样消抖+单触发+多键互斥           ---2026-10-20
 ******************************************************************************/
#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#include "stm32f1xx_hal.h"

#define APP_BTN_EVENT_DEC    0x01U  /* SW2/PE13 有效按下(角度减小) */
#define APP_BTN_EVENT_INC    0x02U  /* SW3/PE14 有效按下(角度增大) */
#define APP_BTN_EVENT_MODE   0x04U  /* SW4/PE15 有效按下(模式切换) */

/*************************************
 * 函数名称 ： app_button_scan
 * 描述     ： 按键扫描, 以20ms为门控连续采样两次一致才确认状态,
 *             一次稳定按下只返回一次事件标志, 按住不重复触发;
 *             必须先释放才能触发下一次; 多键同时按下不产生事件
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 事件标志位域(APP_BTN_EVENT_xxx按位或), 0=无事件
 **************************************/
uint8_t app_button_scan(void);

#endif /* APP_BUTTON_H */

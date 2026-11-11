/***********************************************************************************
 * @file      app_servo.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     SG90舵机控制模块对外接口
 *
 * @note      脉宽窗口用标称1000~2000us保守值(设计要求§7); 端点堵转
 *           嗡鸣时按25~50us步进收窄, 一次只动一端。
 * @history   V1.0.0 首版: 角度限幅+线性脉宽换算+比较值更新         ---2026-11-11
 **********************************************************************************/
#ifndef APP_SERVO_H
#define APP_SERVO_H

#include "stm32f1xx_hal.h"
#include "tim.h"

#define SERVO_MIN_ANGLE_DEG      0U     /* 目标角度下限(度) */
#define SERVO_MAX_ANGLE_DEG    180U     /* 目标角度上限(度) */
#define SERVO_MIN_PULSE_US   1000U     /* 下限脉宽(us), 标称保守窗口 */
#define SERVO_MAX_PULSE_US   2000U     /* 上限脉宽(us), 标称保守窗口 */
#define SERVO_DEFAULT_ANGLE   90U     /* 上电默认目标角度(度)=中位 */

/*************************************
 * 函数名称 ： app_servo_init
 * 描述     ： 舵机模块初始化, 启动TIM3_CH1 PWM并输出默认角度
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_servo_init(void);

/*************************************
 * 函数名称 ： app_servo_set_angle
 * 描述     ： 设定目标角度: 先限幅到安全范围, 再线性换算脉宽并
 *             更新TIM3_CH1比较值(不重启PWM)
 * 输入     ： angle_deg - 目标角度(度), 超范围自动饱和
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_servo_set_angle(uint16_t angle_deg);

/*************************************
 * 函数名称 ： app_servo_get_angle
 * 描述     ： 读取当前目标角度(已限幅后的实际设定值)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 当前目标角度(度)
 **************************************/
uint16_t app_servo_get_angle(void);

#endif /* APP_SERVO_H */

/***********************************************************************************
 * @file      app_servo.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     SG90舵机控制模块: 目标角度限幅/脉宽换算/PWM更新
 *           (TIM3_CH1 -> PA6, 1us/tick, 50Hz硬件PWM, 无需主循环维持)
 *
 * @note      预期现象: 上电输出中位90度(约1500us); 改变角度只更新CCR,
 *           不重启PWM(设计要求§7)。
 * @history   V1.0.0 首版: 角度限幅+线性脉宽换算+比较值更新         ---2026-11-11
 **********************************************************************************/
#include "app_servo.h"

/* ==========================================================================
 *  私有变量
 * ========================================================================== */
static uint16_t g_angle_deg = SERVO_DEFAULT_ANGLE;   /* 当前目标角度(度) */

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： app_servo_init
 * 描述     ： 舵机模块初始化, 启动TIM3_CH1 PWM并输出
 *             默认角度(90度, 约1500us中位脉冲)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_servo_init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    app_servo_set_angle(SERVO_DEFAULT_ANGLE);
}

/*************************************
 * 函数名称 ： app_servo_set_angle
 * 描述     ： 设定目标角度: 先限幅到配置的机械安全范围(防无符号下溢),
 *             再线性换算为脉宽并更新TIM3_CH1比较值
 * 输入     ： angle_deg - 目标角度(度), 超范围自动饱和
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_servo_set_angle(uint16_t angle_deg)
{
    uint32_t pulse_us;

    /* 先夹紧到安全范围: 减法前判下界, 防uint16下溢回绕 */
    if (angle_deg < SERVO_MIN_ANGLE_DEG)
    {
        angle_deg = SERVO_MIN_ANGLE_DEG;
    }
    if (angle_deg > SERVO_MAX_ANGLE_DEG)
    {
        angle_deg = SERVO_MAX_ANGLE_DEG;
    }

    g_angle_deg = angle_deg;

    /* 线性换算: MIN + (angle - MIN) * (MAX-MIN)/(180-0), 全程uint32无溢出 */
    pulse_us = (uint32_t)SERVO_MIN_PULSE_US +
               ((uint32_t)angle_deg *
                (uint32_t)(SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) /
                180U);

    if (pulse_us > SERVO_MAX_PULSE_US)
    {
        pulse_us = SERVO_MAX_PULSE_US;
    }

    /* 只更新比较值, 不重启PWM(设计要求§7) */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint32_t)pulse_us);
}

/*************************************
 * 函数名称 ： app_servo_get_angle
 * 描述     ： 读取当前目标角度(已限幅后的实际设定值)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 当前目标角度(度)
 **************************************/
uint16_t app_servo_get_angle(void)
{
    return g_angle_deg;
}

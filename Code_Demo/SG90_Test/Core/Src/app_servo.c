/*******************************************************************************
 * @file      app_servo.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.1
 * @date      2026-10-20
 * @brief     SG90舵机控制模块实现: TIM3 1us/tick, 比较值单位即us,
 *           换算公式 pulse = min + (angle*(max-min)+90)/180 (四舍五入)

 * @note     PWM由定时器硬件持续输出, 主循环只改比较值, 禁止忙等生成波形
 * @history   V1.0.1 端点堵转嗡鸣, 脉宽边界收窄至1150~1850us      ---2026-10-20
 *            V1.0.0 首版: 角度限幅+线性脉宽换算+比较值更新         ---2026-10-20
 ******************************************************************************/
#include "app_servo.h"
#include "tim.h"

static uint16_t g_servo_angle = SERVO_DEFAULT_ANGLE; /* 当前目标角度(度) */

/*************************************
 * 函数名称 ： app_servo_angle_to_pulse
 * 描述     ： 目标角度线性换算为脉宽(us), 128倍中间量避免72MHz下
 *             uint32乘法溢出, +90实现四舍五入
 * 输入     ： angle_deg - 目标角度(度), 调用前已限幅
 * 输出     ： 无
 * 返回     ： 脉宽(us), 范围SERVO_MIN~MAX_PULSE_US
 **************************************/
static uint32_t app_servo_angle_to_pulse(uint16_t angle_deg)
{
    uint32_t span = SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US; /* 700us */
    uint32_t pulse;

    /* angle<=180, span=1000: 128*180*1000 = 23,040,000 < 2^32, 安全 */
    pulse = ((uint32_t)angle_deg * span * 128U + 90U * 128U) / 180U / 128U;
    return SERVO_MIN_PULSE_US + pulse;
}

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
    g_servo_angle = SERVO_DEFAULT_ANGLE;
    (void)HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,
                          app_servo_angle_to_pulse(g_servo_angle));
}

/*************************************
 * 函数名称 ： app_servo_set_angle
 * 描述     ： 设定目标角度: 先限幅到0~180度(防无符号下溢),
 *             再线性换算为脉宽并更新TIM3_CH1比较值
 * 输入     ： angle_deg - 目标角度(度), 超范围自动饱和
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_servo_set_angle(uint16_t angle_deg)
{
    if (angle_deg > SERVO_MAX_ANGLE_DEG)
    {
        angle_deg = SERVO_MAX_ANGLE_DEG;         /* 上限饱和, 不回绕 */
    }
    g_servo_angle = angle_deg;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,
                          app_servo_angle_to_pulse(angle_deg));
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
    return g_servo_angle;
}

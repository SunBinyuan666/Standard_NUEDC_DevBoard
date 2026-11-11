/*******************************************************************************
 * @file      motor_tb6612.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     TB6612FNG双路方向、PWM、限幅及安全停机驱动
 * @note      A/B实际转向不同时只修改配置表中的reversed成员
 *
 * @history   V1.0.0 首版: 实现双电机独立控制与安全初态       ---2026-11-12
 ******************************************************************************/

/* 头文件包含 -----------------------------------------------------------------*/
#include "motor_tb6612.h"
#include "tim.h"

/* 私有常量 -------------------------------------------------------------------*/
#define MOTOR_SPEED_MAX            (1000)
#define MOTOR_PWM_PERIOD_COUNTS    (3600U)

/* 私有类型 -------------------------------------------------------------------*/
typedef struct
{
    GPIO_TypeDef *in1_port;        /* IN1端口 */
    uint16_t in1_pin;              /* IN1引脚 */
    GPIO_TypeDef *in2_port;        /* IN2端口 */
    uint16_t in2_pin;              /* IN2引脚 */
    uint32_t tim_channel;          /* PWM定时器通道 */
    bool reversed;                 /* 车体前进方向反转配置 */
} MotorConfig_t;

/* 私有变量 -------------------------------------------------------------------*/
static const MotorConfig_t s_motor_config[MOTOR_COUNT] =
{
    [MOTOR_A] =
    {
        MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin,
        MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin,
        TIM_CHANNEL_1, false
    },
    [MOTOR_B] =
    {
        MOTOR_BIN1_GPIO_Port, MOTOR_BIN1_Pin,
        MOTOR_BIN2_GPIO_Port, MOTOR_BIN2_Pin,
        TIM_CHANNEL_2, false
    }
};

static bool s_pwm_started[MOTOR_COUNT] = {false, false};
static bool s_direction_valid[MOTOR_COUNT] = {false, false};
static bool s_direction_reverse[MOTOR_COUNT] = {false, false};
static int16_t s_speed[MOTOR_COUNT] = {0, 0};
static bool s_enabled = false;

/*************************************
 * 函数名称 ： motor_id_is_valid
 * 描述     ： 检查电机编号是否指向有效通道
 * 输入     ： id - 电机编号
 * 输出     ： 无
 * 返回     ： true - 有效; false - 无效
 **************************************/
static bool motor_id_is_valid(MotorId_t id)
{
    return ((id == MOTOR_A) || (id == MOTOR_B));
}

/*************************************
 * 函数名称 ： motor_set_compare
 * 描述     ： 设置单路CCR; TIM3尚未初始化时不访问空外设地址
 * 输入     ： id - 电机编号; compare - 比较值
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void motor_set_compare(MotorId_t id, uint32_t compare)
{
    if (htim3.Instance == TIM3)
    {
        __HAL_TIM_SET_COMPARE(&htim3,
                              s_motor_config[id].tim_channel,
                              compare);
    }
}

/*************************************
 * 函数名称 ： motor_set_direction
 * 描述     ： 按车体方向和单路反转配置设置IN1/IN2电平
 * 输入     ： id - 电机编号; reverse - true为后退
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void motor_set_direction(MotorId_t id, bool reverse)
{
    const MotorConfig_t *config = &s_motor_config[id];
    GPIO_PinState in1_state;
    GPIO_PinState in2_state;

    if (reverse)
    {
        in1_state = GPIO_PIN_RESET;
        in2_state = GPIO_PIN_SET;
    }
    else
    {
        in1_state = GPIO_PIN_SET;
        in2_state = GPIO_PIN_RESET;
    }

    HAL_GPIO_WritePin(config->in1_port, config->in1_pin, in1_state);
    HAL_GPIO_WritePin(config->in2_port, config->in2_pin, in2_state);
    s_direction_reverse[id] = reverse;
    s_direction_valid[id] = true;
}

/*************************************
 * 函数名称 ： Motor_Init
 * 描述     ： 建立安全初态并启动双路PWM, 结束后保持STBY待机
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void Motor_Init(void)
{
    MotorId_t id;

    Motor_StopAll();

    for (id = MOTOR_A; id < MOTOR_COUNT; id++)
    {
        HAL_GPIO_WritePin(s_motor_config[id].in1_port,
                          s_motor_config[id].in1_pin,
                          GPIO_PIN_RESET);
        HAL_GPIO_WritePin(s_motor_config[id].in2_port,
                          s_motor_config[id].in2_pin,
                          GPIO_PIN_RESET);
        s_direction_valid[id] = false;
        s_pwm_started[id] = false;
    }

    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
    {
        Motor_StopAll();
        Error_Handler();
    }
    s_pwm_started[MOTOR_A] = true;

    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2) != HAL_OK)
    {
        (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        s_pwm_started[MOTOR_A] = false;
        Motor_StopAll();
        Error_Handler();
    }
    s_pwm_started[MOTOR_B] = true;

    HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin,
                      GPIO_PIN_RESET);
    s_enabled = false;
}

/*************************************
 * 函数名称 ： Motor_Enable
 * 描述     ： 控制双通道待机; 失能会清零CCR, 使能不恢复旧速度
 * 输入     ： enable - true解除待机, false进入安全停机
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void Motor_Enable(bool enable)
{
    if (!enable)
    {
        Motor_StopAll();
        return;
    }

    if (s_enabled)
    {
        return;
    }

    if ((!s_pwm_started[MOTOR_A]) || (!s_pwm_started[MOTOR_B]))
    {
        Motor_StopAll();
        Error_Handler();
    }

    Motor_Stop(MOTOR_A);
    Motor_Stop(MOTOR_B);
    HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin,
                      GPIO_PIN_SET);
    s_enabled = true;
}

/*************************************
 * 函数名称 ： Motor_SetSpeed
 * 描述     ： 设置单路千分比速度, 超限限幅, 反向前先清零CCR
 * 输入     ： id - 电机编号; speed - -1000~1000速度
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void Motor_SetSpeed(MotorId_t id, int16_t speed)
{
    int32_t limited_speed = speed;
    int32_t magnitude;
    uint32_t compare;
    bool reverse;

    if (!motor_id_is_valid(id))
    {
        Motor_StopAll();
        Error_Handler();
        return;
    }

    if (limited_speed > MOTOR_SPEED_MAX)
    {
        limited_speed = MOTOR_SPEED_MAX;
    }
    else if (limited_speed < -MOTOR_SPEED_MAX)
    {
        limited_speed = -MOTOR_SPEED_MAX;
    }

    if (limited_speed == 0)
    {
        Motor_Stop(id);
        return;
    }

    if ((!s_enabled) || (!s_pwm_started[id]))
    {
        Motor_Stop(id);
        return;
    }

    reverse = ((limited_speed < 0) != s_motor_config[id].reversed);
    if ((!s_direction_valid[id]) ||
        (s_direction_reverse[id] != reverse))
    {
        motor_set_compare(id, 0U);
        motor_set_direction(id, reverse);
    }

    magnitude = (limited_speed < 0) ? -limited_speed : limited_speed;
    compare = ((uint32_t)magnitude * MOTOR_PWM_PERIOD_COUNTS) /
              (uint32_t)MOTOR_SPEED_MAX;
    motor_set_compare(id, compare);
    s_speed[id] = (int16_t)limited_speed;
}

/*************************************
 * 函数名称 ： Motor_Stop
 * 描述     ： 单路占空比清零, STBY有效时形成短路制动
 * 输入     ： id - 电机编号
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void Motor_Stop(MotorId_t id)
{
    if (!motor_id_is_valid(id))
    {
        Motor_StopAll();
        Error_Handler();
        return;
    }

    motor_set_compare(id, 0U);
    s_speed[id] = 0;
}

/*************************************
 * 函数名称 ： Motor_StopAll
 * 描述     ： 双路占空比清零并拉低STBY, 可从任意错误路径调用
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void Motor_StopAll(void)
{
    motor_set_compare(MOTOR_A, 0U);
    motor_set_compare(MOTOR_B, 0U);
    s_speed[MOTOR_A] = 0;
    s_speed[MOTOR_B] = 0;

    HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin,
                      GPIO_PIN_RESET);
    s_enabled = false;
}

/*******************************************************************************
 * @file      motor_test.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     按步骤表执行双电机独立正反转、停止和同步调速测试
 * @note      上电1s后开始, 总流程15s, 只执行一次, 结束保持待机
 *
 * @history   V1.0.0 首版: 实现非阻塞单次功能测试             ---2026-11-12
 ******************************************************************************/

/* 头文件包含 -----------------------------------------------------------------*/
#include "motor_test.h"
#include "motor_tb6612.h"
#include <stdbool.h>
#include <stddef.h>

/* 私有常量 -------------------------------------------------------------------*/
#define TEST_SPEED_20_PERCENT      (200)
#define TEST_SPEED_50_PERCENT      (500)
#define TEST_INITIAL_STOP_MS       (1000U)
#define TEST_RUN_MS                (2000U)
#define TEST_STOP_MS               (500U)

/* 私有类型 -------------------------------------------------------------------*/
typedef struct
{
    int16_t left_speed;            /* A路千分比速度 */
    int16_t right_speed;           /* B路千分比速度 */
    uint32_t duration_ms;          /* 当前步骤持续时间 */
    bool driver_enabled;           /* 当前步骤是否解除待机 */
} MotorTestStep_t;

/* 私有变量 -------------------------------------------------------------------*/
static const MotorTestStep_t s_test_steps[] =
{
    {0, 0, TEST_INITIAL_STOP_MS, false},
    {TEST_SPEED_20_PERCENT, 0, TEST_RUN_MS, true},
    {0, 0, TEST_STOP_MS, true},
    {-TEST_SPEED_20_PERCENT, 0, TEST_RUN_MS, true},
    {0, 0, TEST_STOP_MS, true},
    {0, TEST_SPEED_20_PERCENT, TEST_RUN_MS, true},
    {0, 0, TEST_STOP_MS, true},
    {0, -TEST_SPEED_20_PERCENT, TEST_RUN_MS, true},
    {0, 0, TEST_STOP_MS, true},
    {TEST_SPEED_20_PERCENT, TEST_SPEED_20_PERCENT, TEST_RUN_MS, true},
    {TEST_SPEED_50_PERCENT, TEST_SPEED_50_PERCENT, TEST_RUN_MS, true},
    {0, 0, 0U, false}
};

#define TEST_STEP_COUNT \
    ((uint8_t)(sizeof(s_test_steps) / sizeof(s_test_steps[0])))

static uint8_t s_step_index = 0U;
static uint32_t s_step_start_ms = 0U;
static bool s_finished = false;

/*************************************
 * 函数名称 ： motor_test_enter_step
 * 描述     ： 执行步骤入口动作, 测试层不直接访问GPIO或定时器
 * 输入     ： step_index - 步骤表索引
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void motor_test_enter_step(uint8_t step_index)
{
    const MotorTestStep_t *step = &s_test_steps[step_index];

    if (step->driver_enabled)
    {
        Motor_Enable(true);
        Motor_SetSpeed(MOTOR_A, step->left_speed);
        Motor_SetSpeed(MOTOR_B, step->right_speed);
    }
    else
    {
        Motor_StopAll();
    }

    if (step_index == (TEST_STEP_COUNT - 1U))
    {
        s_finished = true;
    }
}

/*************************************
 * 函数名称 ： MotorTest_Init
 * 描述     ： 初始化单次测试状态机并建立1s安全等待段
 * 输入     ： now_ms - 当前HAL毫秒时间戳
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MotorTest_Init(uint32_t now_ms)
{
    s_step_index = 0U;
    s_step_start_ms = now_ms;
    s_finished = false;
    motor_test_enter_step(s_step_index);
}

/*************************************
 * 函数名称 ： MotorTest_Process
 * 描述     ： 非阻塞推进测试步骤, 时间差可跨HAL时间戳回绕
 * 输入     ： now_ms - 当前HAL毫秒时间戳
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MotorTest_Process(uint32_t now_ms)
{
    const MotorTestStep_t *step;

    if (s_finished)
    {
        return;
    }

    step = &s_test_steps[s_step_index];
    if ((uint32_t)(now_ms - s_step_start_ms) < step->duration_ms)
    {
        return;
    }

    s_step_index++;
    if (s_step_index >= TEST_STEP_COUNT)
    {
        Motor_StopAll();
        s_finished = true;
        return;
    }

    s_step_start_ms = now_ms;
    motor_test_enter_step(s_step_index);
}

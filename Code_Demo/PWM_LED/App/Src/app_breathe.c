/********************************************************************************
 * @file      app_breathe.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-19
 * @brief     LED3 的非阻塞 PWM 呼吸控制。
 * @history   V1.0.0 将呼吸控制逻辑封装为独立应用模块。
 ********************************************************************************/
#include <stdbool.h>
#include <stdint.h>

#include "app_breathe.h"
#include "tim.h"

/* 私有宏定义 -----------------------------------------------------------------*/
#define APP_BREATHE_UPDATE_INTERVAL_MS (10U)
#define APP_BREATHE_CCR_MIN            (0U)
#define APP_BREATHE_CCR_MAX            (1000U)
#define APP_BREATHE_CCR_STEP           (5U)

/* 私有类型 -------------------------------------------------------------------*/
typedef enum
{
    BREATHE_CCR_INCREASING = 0,
    BREATHE_CCR_DECREASING,
    BREATHE_DIRECTION_COUNT
} BreatheDirection_t;

/* 私有变量 -------------------------------------------------------------------*/
static uint32_t s_breathe_last_tick = 0U;
static uint16_t s_breathe_ccr = (uint16_t)APP_BREATHE_CCR_MAX;
static BreatheDirection_t s_breathe_direction = BREATHE_CCR_INCREASING;
static bool s_breathe_initialized = false;

/* 公共函数 -------------------------------------------------------------------*/
/*******************************************************************************
 * 函数名称：app_breathe_init
 * 功能说明：初始化呼吸控制状态，并将 LED3 设置为熄灭。
 * 输入参数：无。
 * 输出参数：无。
 * 返回值：无。
 ******************************************************************************/
void app_breathe_init(void)
{
    if (s_breathe_initialized)
    {
        return;
    }

    s_breathe_last_tick = HAL_GetTick();
    s_breathe_ccr = (uint16_t)APP_BREATHE_CCR_MAX;
    s_breathe_direction = BREATHE_CCR_INCREASING;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, s_breathe_ccr);
    s_breathe_initialized = true;
}

/*******************************************************************************
 * 函数名称：app_breathe_process
 * 功能说明：按设定周期更新 LED3 对应的 PWM 比较值。
 * 输入参数：无。
 * 输出参数：无。
 * 返回值：无。
 ******************************************************************************/
void app_breathe_process(void)
{
    const uint32_t current_tick = HAL_GetTick();
    const uint32_t elapsed_ticks =
        (uint32_t)(current_tick - s_breathe_last_tick);

    if (!s_breathe_initialized)
    {
        return;
    }

    if (elapsed_ticks < APP_BREATHE_UPDATE_INTERVAL_MS)
    {
        return;
    }

    s_breathe_last_tick = current_tick;

    if (s_breathe_direction == BREATHE_CCR_DECREASING)
    {
        if (s_breathe_ccr <= APP_BREATHE_CCR_STEP)
        {
            s_breathe_ccr = (uint16_t)APP_BREATHE_CCR_MIN;
            s_breathe_direction = BREATHE_CCR_INCREASING;
        }
        else
        {
            s_breathe_ccr = (uint16_t)(
                s_breathe_ccr - APP_BREATHE_CCR_STEP);
        }
    }
    else if (s_breathe_direction == BREATHE_CCR_INCREASING)
    {
        if (s_breathe_ccr >=
            (APP_BREATHE_CCR_MAX - APP_BREATHE_CCR_STEP))
        {
            s_breathe_ccr = (uint16_t)APP_BREATHE_CCR_MAX;
            s_breathe_direction = BREATHE_CCR_DECREASING;
        }
        else
        {
            s_breathe_ccr = (uint16_t)(
                s_breathe_ccr + APP_BREATHE_CCR_STEP);
        }
    }
    else
    {
        s_breathe_ccr = (uint16_t)APP_BREATHE_CCR_MAX;
        s_breathe_direction = BREATHE_CCR_DECREASING;
    }

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, s_breathe_ccr);
}

/*******************************************************************************
 * 函数名称：app_breathe_deinit
 * 功能说明：停止呼吸效果并熄灭 LED3，不停止 TIM4。
 * 输入参数：无。
 * 输出参数：无。
 * 返回值：无。
 ******************************************************************************/
void app_breathe_deinit(void)
{
    if (!s_breathe_initialized)
    {
        return;
    }

    s_breathe_ccr = (uint16_t)APP_BREATHE_CCR_MAX;
    s_breathe_direction = BREATHE_CCR_INCREASING;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, s_breathe_ccr);
    s_breathe_initialized = false;
}

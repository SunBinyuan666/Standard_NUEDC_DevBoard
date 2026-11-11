/***********************************************************************************
 * @file      app_button.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     三按键扫描模块: tick门控周期采样+计数消抖+单事件触发
 *           (SW2/PE13 暂停恢复, SW3/PE14 左微调, SW4/PE15 右微调)
 *
 * @note      全部低电平有效(输入上拉, 按下接地)。每键独立三件套:
 *           按下计数/松开计数/locked标志, 连续N次采到有效电平才触发,
 *           连续N次采到无效电平才解锁; 无共享key_up标志。
 *           10ms周期门控, 主循环零阻塞(设计要求§8.2: 20ms稳定)。
 * @history   V1.0.0 首版: 三键表驱动+计数消抖+单事件              ---2026-11-11
 **********************************************************************************/
#include "app_button.h"

/* ==========================================================================
 *  私有常量
 * ========================================================================== */
#define BTN_SCAN_PERIOD_MS    10U   /* 采样周期(ms) */
#define BTN_STABLE_COUNT       2U   /* 连续2次采样(20ms)确认状态改变 */

/* ==========================================================================
 *  私有类型定义
 * ========================================================================== */
/* 每键消抖状态三件套 */
typedef struct
{
    uint8_t press_cnt;      /* 连续采到按下电平的次数 */
    uint8_t release_cnt;    /* 连续采到松开电平的次数 */
    uint8_t locked;         /* 已触发且未松开期间禁止重触发 */
} btn_state_t;

/* ==========================================================================
 *  私有变量
 * ========================================================================== */
/* 端口/引脚查表驱动: 加按键只加表项不改逻辑 */
static GPIO_TypeDef *const g_port[BUTTON_ID_MAX] =
{
    SW2_GPIO_Port,          /* PE13 暂停/恢复 */
    SW3_GPIO_Port,          /* PE14 左微调 */
    SW4_GPIO_Port           /* PE15 右微调 */
};

static const uint16_t g_pin[BUTTON_ID_MAX] =
{
    SW2_Pin,
    SW3_Pin,
    SW4_Pin
};

static btn_state_t g_btn[BUTTON_ID_MAX];            /* 每键消抖状态 */
static uint32_t g_last_scan_tick = 0U;              /* 上次采样时刻 */
static uint8_t g_event[BUTTON_ID_MAX];              /* 待取走的事件 */

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： app_button_init
 * 描述     ： 按键模块初始化, 清零消抖状态与事件
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_button_init(void)
{
    uint8_t i;

    for (i = 0U; i < (uint8_t)BUTTON_ID_MAX; i++)
    {
        g_btn[i].press_cnt = 0U;
        g_btn[i].release_cnt = 0U;
        g_btn[i].locked = 0U;
        g_event[i] = 0U;
    }
    g_last_scan_tick = HAL_GetTick();
}

/*************************************
 * 函数名称 ： app_button_process
 * 描述     ： 主循环周期调用; tick门控10ms采样, 连续20ms低电平
 *             确认按下并产生一次事件, 松开后才允许下一次
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_button_process(void)
{
    uint8_t i;
    uint32_t now = HAL_GetTick();

    /* 周期门控: 未到采样周期直接返回, 主循环零阻塞 */
    if ((now - g_last_scan_tick) < BTN_SCAN_PERIOD_MS)
    {
        return;
    }
    g_last_scan_tick = now;

    for (i = 0U; i < (uint8_t)BUTTON_ID_MAX; i++)
    {
        GPIO_PinState level = HAL_GPIO_ReadPin(g_port[i], g_pin[i]);

        if (level == GPIO_PIN_RESET)
        {
            /* 采到按下电平(低有效) */
            g_btn[i].release_cnt = 0U;
            if (g_btn[i].press_cnt < BTN_STABLE_COUNT)
            {
                g_btn[i].press_cnt++;
            }
            else if ((g_btn[i].locked == 0U) && (g_event[i] == 0U))
            {
                /* 连续稳定按下且未锁定: 触发一次事件 */
                g_event[i] = 1U;
                g_btn[i].locked = 1U;
            }
            else
            {
                /* 已锁定, 按住不重触发 */
            }
        }
        else
        {
            /* 采到松开电平 */
            g_btn[i].press_cnt = 0U;
            if (g_btn[i].release_cnt < BTN_STABLE_COUNT)
            {
                g_btn[i].release_cnt++;
            }
            else
            {
                /* 连续稳定松开: 解锁, 允许下一次触发 */
                g_btn[i].locked = 0U;
            }
        }
    }
}

/*************************************
 * 函数名称 ： app_button_get_event
 * 描述     ： 查询并取走一次按键事件(读清)
 * 输入     ： id - 按键编号
 * 输出     ： 无
 * 返回     ： 1=该键有一次稳定按下事件, 0=无
 **************************************/
uint8_t app_button_get_event(button_id_t id)
{
    if ((id >= BUTTON_ID_MAX) || (g_event[id] == 0U))
    {
        return 0U;
    }

    g_event[id] = 0U;
    return 1U;
}

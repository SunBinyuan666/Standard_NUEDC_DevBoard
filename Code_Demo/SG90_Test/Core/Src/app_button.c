/*******************************************************************************
 * @file      app_button.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-20
 * @brief     按键扫描模块实现: HAL_GetTick时基, 20ms采样门控,
 *           连续2次一致电平确认按下/释放(状态稳定至少20ms)

 * @note     每键独立消抖计数与锁定标志; 采样到多键同时按下时
 *           清空全部计数, 不产生事件(需求§4.1多键互斥)
 * @history   V1.0.0 首版: 周期采样消抖+单触发+多键互斥           ---2026-10-20
 ******************************************************************************/
#include "app_button.h"
#include "main.h"

#define BTN_SCAN_PERIOD_MS     20U  /* 采样周期(ms), 2次一致=稳定20ms */
#define BTN_CONFIRM_COUNT       2U  /* 连续N次一致电平确认状态翻转 */

/* 按键索引: 0=SW2/PE13(减) 1=SW3/PE14(增) 2=SW4/PE15(模式) */
static GPIO_TypeDef *const btn_ports[3] = {
    KEY_DEC_GPIO_Port, KEY_INC_GPIO_Port, KEY_MODE_GPIO_Port
};
/* 按键端口表; 指针本身const, 兼容HAL非const参数 */

static const uint16_t btn_pins[3] = {
    KEY_DEC_Pin, KEY_INC_Pin, KEY_MODE_Pin
};
/* 按键引脚表 */

static uint8_t g_btn_pressed_cnt[3] = {0U, 0U, 0U}; /* 按下消抖计数器 */
static uint8_t g_btn_release_cnt[3] = {0U, 0U, 0U}; /* 松开消抖计数器 */
static uint8_t g_btn_locked[3]      = {0U, 0U, 0U}; /* 1=已触发等松开 */

static uint32_t g_btn_last_tick = 0U;               /* 上次采样时刻 */

/*************************************
 * 函数名称 ： app_button_scan
 * 描述     ： 按键扫描, 以20ms为门控连续采样两次一致才确认状态,
 *             一次稳定按下只返回一次事件标志, 按住不重复触发;
 *             必须先释放才能触发下一次; 多键同时按下不产生事件
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 事件标志位域(APP_BTN_EVENT_xxx按位或), 0=无事件
 **************************************/
uint8_t app_button_scan(void)
{
    uint8_t  events = 0U;
      uint8_t  pressed_mask = 0U;                   /* 本轮按下位图 */
    uint8_t  i;
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - g_btn_last_tick) < BTN_SCAN_PERIOD_MS)
    {
        return 0U;                                /* 未到采样周期 */
    }
    g_btn_last_tick = now;

    /* 第一遍: 采样并消抖 */
    for (i = 0U; i < 3U; i++)
    {
        if (HAL_GPIO_ReadPin(btn_ports[i], btn_pins[i]) == GPIO_PIN_RESET)
        {
            g_btn_pressed_cnt[i]++;               /* 采样到按下(低) */
            g_btn_release_cnt[i] = 0U;
            if (g_btn_pressed_cnt[i] >= BTN_CONFIRM_COUNT)
            {
                pressed_mask |= (uint8_t)(1U << i);
            }
        }
        else
        {
            g_btn_release_cnt[i]++;               /* 采样到松开(高) */
            g_btn_pressed_cnt[i] = 0U;
            if (g_btn_release_cnt[i] >= BTN_CONFIRM_COUNT)
            {
                g_btn_locked[i] = 0U;             /* 确认松开后解锁 */
            }
        }
    }

    /* 第二遍: 生成事件; 多键同时按下则全部不触发 */
    if ((pressed_mask != 0U) && (pressed_mask != 0x01U) &&
        (pressed_mask != 0x02U) && (pressed_mask != 0x04U))
    {
        for (i = 0U; i < 3U; i++)
        {
            g_btn_locked[i] = 1U;                 /* 多键互锁, 等全部释放 */
        }
        return 0U;
    }

    for (i = 0U; i < 3U; i++)
    {
        if (((pressed_mask & (uint8_t)(1U << i)) != 0U) &&
            (g_btn_locked[i] == 0U))
        {
            events |= (uint8_t)(1U << i);         /* 单键按下, 产生事件 */
            g_btn_locked[i] = 1U;                 /* 按住只触发一次 */
        }
    }
    return events;
}

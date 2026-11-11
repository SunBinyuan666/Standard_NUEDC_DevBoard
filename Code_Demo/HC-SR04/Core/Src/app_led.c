/***********************************************************************************
 * @file      app_led.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     板载LED状态指示模块: LED3/PB6(左) LED4/PB7(右) LED5/PD8(状态心跳)
 *           全部低电平点亮, 共阳极接法
 *
 * @note      LED编码表(设计要求§8.1):
 *           正常监测    -> LED3/4灭, LED5慢速心跳(约1Hz)
 *           已选左侧    -> LED3亮, LED4灭, LED5心跳
 *           已选右侧    -> LED3灭, LED4亮, LED5心跳
 *           正在扫描    -> LED3/4随待测方向提示, LED5约2Hz闪
 *           无安全方向  -> LED3/4灭, LED5约4Hz快闪
 *           传感器故障  -> 同无安全方向(可软件区分错误码)
 *           人工暂停    -> 按当前人工指向显示, LED5慢闪
 *           时序全部用 HAL_GetTick() 门控, 无阻塞延时。
 * @history   V1.0.0 首版: 方向灯+四档心跳灯, tick门控非阻塞       ---2026-11-11
 **********************************************************************************/
#include "app_led.h"
#include "main.h"

/* ==========================================================================
 *  私有变量
 * ========================================================================== */
static led_state_t g_state = LED_STATE_NORMAL;      /* 当前显示状态 */
static uint8_t g_scan_dir = SCAN_DIR_CENTER;        /* 扫描提示方向 */

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： app_led_init
 * 描述     ： LED模块初始化, 全部熄灭(输出高)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_init(void)
{
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, GPIO_PIN_SET);
}

/*************************************
 * 函数名称 ： app_led_set_state
 * 描述     ： 设置状态灯模式; 扫描态需同时给出待测方向
 * 输入     ： state - 显示状态
 *             scan_dir - 扫描方向提示(仅LED_STATE_SCANNING有效)
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_set_state(led_state_t state, uint8_t scan_dir)
{
    g_state = state;
    if (state == LED_STATE_SCANNING)
    {
        g_scan_dir = scan_dir;
    }
}

/*************************************
 * 函数名称 ： app_led_process
 * 描述     ： 主循环周期调用, 按当前状态驱动三只LED;
 *             心跳/闪烁用HAL_GetTick门控, 无阻塞
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_process(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t on;

    switch (g_state)
    {
        case LED_STATE_NORMAL:
        case LED_STATE_POINT_LEFT:
        case LED_STATE_POINT_RIGHT:
            /* 方向灯: 左=LED3, 右=LED4; LED5慢速心跳1Hz(500ms翻转) */
            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin,
                (g_state == LED_STATE_POINT_LEFT) ? GPIO_PIN_RESET
                                                  : GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin,
                (g_state == LED_STATE_POINT_RIGHT) ? GPIO_PIN_RESET
                                                   : GPIO_PIN_SET);
            on = ((now / 500U) & 1U);
            HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin,
                              (on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
            break;

        case LED_STATE_SCANNING:
            /* 待测方向提示: 左LED3/中两灭/右LED4; LED5约2Hz(250ms翻转) */
            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin,
                (g_scan_dir == SCAN_DIR_LEFT) ? GPIO_PIN_RESET
                                              : GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin,
                (g_scan_dir == SCAN_DIR_RIGHT) ? GPIO_PIN_RESET
                                               : GPIO_PIN_SET);
            on = ((now / 250U) & 1U);
            HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin,
                              (on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
            break;

        case LED_STATE_NO_WAY:
        case LED_STATE_FAULT:
        default:
            /* 无安全方向/故障: 左右灯灭, LED5约4Hz(125ms翻转) */
            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
            on = ((now / 125U) & 1U);
            HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin,
                              (on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
            break;

        case LED_STATE_PAUSED:
            /* 暂停: 按当前人工指向显示方向灯, LED5慢闪(500ms) */
            HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin,
                (g_scan_dir == SCAN_DIR_LEFT) ? GPIO_PIN_RESET
                                              : GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin,
                (g_scan_dir == SCAN_DIR_RIGHT) ? GPIO_PIN_RESET
                                               : GPIO_PIN_SET);
            on = ((now / 500U) & 1U);
            HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin,
                              (on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
            break;
    }
}

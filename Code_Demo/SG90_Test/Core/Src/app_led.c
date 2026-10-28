/*******************************************************************************
 * @file      app_led.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-20
 * @brief     LED指示模块实现: 低电平点亮(共阳接法),
 *           写RESET=亮, 写SET=灭

 * @note     心跳500ms翻转由HAL_GetTick门控, 不阻塞主循环
 * @history   V1.0.0 首版: 模式指示互斥+500ms心跳              ---2026-10-20
 ******************************************************************************/
#include "app_led.h"
#include "main.h"

#define LED_HEARTBEAT_PERIOD_MS   500U  /* 心跳翻转周期(ms) */

static uint32_t g_led_last_tick = 0U;   /* 上次心跳翻转时刻 */

/*************************************
 * 函数名称 ： app_led_set_mode
 * 描述     ： 设置调节模式指示灯: 细调亮LED3灭LED4,
 *             粗调亮LED4灭LED3, 两灯互斥
 * 输入     ： mode - APP_LED_MODE_FINE或APP_LED_MODE_COARSE
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_set_mode(uint8_t mode)
{
    if (mode == APP_LED_MODE_FINE)
    {
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); /* 亮 */
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);   /* 灭 */
    }
    else
    {
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);   /* 灭 */
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_RESET); /* 亮 */
    }
}

/*************************************
 * 函数名称 ： app_led_heartbeat
 * 描述     ： 心跳处理, 以500ms间隔翻转LED5, 需在主循环持续调用,
 *             非阻塞(内部HAL_GetTick门控)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_led_heartbeat(void)
{
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - g_led_last_tick) >= LED_HEARTBEAT_PERIOD_MS)
    {
        g_led_last_tick = now;
        HAL_GPIO_TogglePin(LED5_GPIO_Port, LED5_Pin);
    }
}

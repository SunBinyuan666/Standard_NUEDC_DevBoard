/***********************************************************************************
 * @file      gpio.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     GPIO初始化: TRIG/LED输出(初始熄灭) + SW2/3/4输入上拉
 *
 * @note      LED均为低电平点亮(共阳), 初始输出高=熄灭;
 *           PA6/PB9由tim.c的MSP配置, 此处不重复。
 * @history   V1.0.0 首版: TRIG+三LED+三按键GPIO配置            ---2026-11-11
 **********************************************************************************/
#include "gpio.h"

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： MX_GPIO_Init
 * 描述     ： GPIO初始化: PB0 TRIG推挽输出(初始低);
 *             PB6/PB7/PD8 LED推挽输出(初始高=灭);
 *             PE13/PE14/PE15按键输入上拉(低有效)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* TRIG(PB0) 初始低电平 */
    HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin,
                      GPIO_PIN_RESET);

    /* LED(PB6/PB7/PD8) 共阳低亮, 初始写高=熄灭 */
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, GPIO_PIN_SET);

    /* TRIG + LED: 推挽输出, 低速即可 */
    GPIO_InitStruct.Pin = HCSR04_TRIG_Pin | LED3_Pin | LED4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LED5_Pin;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* SW2/SW3/SW4(PE13/PE14/PE15): 输入上拉, 按下接地 */
    GPIO_InitStruct.Pin = SW2_Pin | SW3_Pin | SW4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

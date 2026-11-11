/***********************************************************************************
 * @file      tim.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V2.0.0
 * @date      2026-11-11
 * @brief     定时器外设初始化: TIM3_CH1/PA6 舵机50Hz PWM(1us/tick)
 *
 * @note      本文件结构与 CubeMX 生成布局一致(USER CODE 区可保护),
 *           应用逻辑在 app_* 模块, 此处只做外设配置。
 * @history   V1.0.0 首版: TIM3 PWM + TIM4 IC 双定时器初始化     ---2026-11-11
 *           V2.0.0 改硬件IIC: 删TIM4 IC(超声波走I2C2)           ---2026-11-11
 **********************************************************************************/
#include "tim.h"

/* ==========================================================================
 *  公共变量
 * ========================================================================== */
TIM_HandleTypeDef htim3;

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： MX_TIM3_Init
 * 描述     ： TIM3初始化: 内部时钟, CH1 PWM输出(PA6),
 *             PSC=71/ARR=19999 -> 72MHz/72=1MHz, 1us/tick, 50Hz周期
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MX_TIM3_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 71U;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 19999U;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0U;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_TIM_MspPostInit(&htim3);
}

/* ==========================================================================
 *  MSP初始化(覆盖HAL弱符号, 不可加static)
 * ========================================================================== */

/*************************************
 * 函数名称 ： HAL_TIM_Base_MspInit
 * 描述     ： 定时器底层初始化: 使能TIM3时钟
 * 输入     ： tim_baseHandle - 定时器句柄
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle)
{
    if (tim_baseHandle->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
    else
    {
        /* 其他定时器未使用 */
    }
}

/*************************************
 * 函数名称 ： HAL_TIM_MspPostInit
 * 描述     ： PWM引脚底层初始化: PA6复用推挽输出(TIM3_CH1)
 * 输入     ： timHandle - 定时器句柄
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *timHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (timHandle->Instance == TIM3)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* TIM3_CH1 -> PA6 复用推挽 */
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}


/***********************************************************************************
 * @file      tim.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     定时器外设初始化: TIM3_CH1/PA6 舵机50Hz PWM(1us/tick)
 *           + TIM4_CH4/PB9 ECHO输入捕获(1us/tick, 16位自由运行)
 *
 * @note      本文件结构与 CubeMX 生成布局一致(USER CODE 区可保护),
 *           应用逻辑在 app_* 模块, 此处只做外设配置。
 * @history   V1.0.0 首版: TIM3 PWM + TIM4 IC 双定时器初始化     ---2026-11-11
 **********************************************************************************/
#include "tim.h"

/* ==========================================================================
 *  公共变量
 * ========================================================================== */
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

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

/*************************************
 * 函数名称 ： MX_TIM4_Init
 * 描述     ： TIM4初始化: 内部时钟, CH4输入捕获(PB9),
 *             PSC=71/ARR=65535 -> 1us/tick自由运行, 上升沿起始
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MX_TIM4_Init(void)
{
    TIM_IC_InitTypeDef sConfigIC = {0};

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 71U;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 65535U;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_IC_Init(&htim4) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0U;
    if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==========================================================================
 *  MSP初始化(覆盖HAL弱符号, 不可加static)
 * ========================================================================== */

/*************************************
 * 函数名称 ： HAL_TIM_Base_MspInit
 * 描述     ： 定时器底层初始化: 使能TIM3/TIM4时钟;
 *             TIM4使能全局中断并配置PB9为浮空输入(IC捕获脚)
 * 输入     ： tim_baseHandle - 定时器句柄
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (tim_baseHandle->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
    else if (tim_baseHandle->Instance == TIM4)
    {
        __HAL_RCC_TIM4_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* TIM4_CH4 -> PB9 浮空输入(F1输入捕获不用AF开漏) */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(TIM4_IRQn, 3U, 0U);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
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


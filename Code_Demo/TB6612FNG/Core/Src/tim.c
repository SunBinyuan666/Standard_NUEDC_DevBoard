/*******************************************************************************
 * @file      tim.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     TIM3_CH1/CH2输出20kHz PWM, 引脚为PA6/PA7
 * @note      72MHz/(PSC+1)/(ARR+1)=72MHz/1/3600=20kHz
 *
 * @history   V1.0.0 首版: 添加双路20kHz PWM配置             ---2026-11-12
 ******************************************************************************/
/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */

/* 头文件包含 -----------------------------------------------------------------*/
#include "tim.h"

/* 私有常量 -------------------------------------------------------------------*/
#define TIM3_PRESCALER_VALUE       (0U)
#define TIM3_PERIOD_VALUE          (3599U)
#define TIM3_INITIAL_PULSE_VALUE   (0U)

/* 外部变量 -------------------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/*************************************
 * 函数名称 ： MX_TIM3_Init
 * 描述     ： 配置TIM3内部时钟和两路PWM1输出, 初始占空比为0
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MX_TIM3_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = TIM3_PRESCALER_VALUE;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = TIM3_PERIOD_VALUE;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3,
                                              &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = TIM3_INITIAL_PULSE_VALUE;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC,
                                  TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC,
                                  TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim3);
}

/*************************************
 * 函数名称 ： HAL_TIM_Base_MspInit
 * 描述     ： 使能TIM3外设时钟
 * 输入     ： tim_baseHandle - TIM句柄指针
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle)
{
    if (tim_baseHandle->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
}

/*************************************
 * 函数名称 ： HAL_TIM_MspPostInit
 * 描述     ： 配置PA6/PA7为TIM3_CH1/CH2复用推挽输出, 不做重映射
 * 输入     ： timHandle - TIM句柄指针
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *timHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (timHandle->Instance == TIM3)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/*************************************
 * 函数名称 ： HAL_TIM_Base_MspDeInit
 * 描述     ： 关闭TIM3外设时钟
 * 输入     ： tim_baseHandle - TIM句柄指针
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *tim_baseHandle)
{
    if (tim_baseHandle->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_DISABLE();
    }
}

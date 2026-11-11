/*******************************************************************************
 * @file      gpio.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     配置PE8~PE12为低速推挽输出, 初始全部低电平
 * @note      STBY外部仍需安全下拉, 保证复位和下载期间电机待机
 *
 * @history   V1.0.0 首版: 添加TB6612FNG方向与待机引脚       ---2026-11-12
 ******************************************************************************/
/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */

/* 头文件包含 -----------------------------------------------------------------*/
#include "gpio.h"

/*************************************
 * 函数名称 ： MX_GPIO_Init
 * 描述     ： 初始化电机方向和待机GPIO, 默认输出关闭状态
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOE,
                      MOTOR_AIN1_Pin | MOTOR_AIN2_Pin |
                      MOTOR_BIN1_Pin | MOTOR_BIN2_Pin |
                      MOTOR_STBY_Pin,
                      GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = MOTOR_AIN1_Pin | MOTOR_AIN2_Pin |
                          MOTOR_BIN1_Pin | MOTOR_BIN2_Pin |
                          MOTOR_STBY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

/*******************************************************************************
 * @file      gpio.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.1
 * @date      2026-10-16
 * @brief     GPIO初始化: LED3(PB6)推挽输出, 低电平点亮
 *
 * @note     LED3=PB6, 板载3.3V经2kΩ电阻到LED阳极, 低电平点亮
 *
 * @history   V1.0.1 注释规范化(个人编程代码规范)                ---2026-10-17
 *            V1.0.0 首版: LED3 GPIO初始化                        ---2026-10-16
 ******************************************************************************/

/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/*************************************
 * 函数名称 ： MX_GPIO_Init
 * 描述     ： GPIO初始化, 使能GPIOA/GPIOB时钟(实际用到的端口),
 *             配置LED3(PB6)为推挽输出, 初始输出SET(LED熄灭),
 *             速度LOW(点灯无需高速)
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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : LED3_Pin */
  GPIO_InitStruct.Pin = LED3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED3_GPIO_Port, &GPIO_InitStruct);

}


/*******************************************************************************

 * @file      gpio.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-19
 * @brief     GPIO初始化: 3个LED推挽输出 + 3个按键输入(内部上拉)

 * @note     LED3=PB6/LED4=PB7/LED5=PD8, 2kΩ上拉到3.3V, 低电平点亮;
 *           KEY0=PE13/KEY1=PE14/KEY2=PE15, 按下接地, 内部上拉, 各带100nF滤波

 * @history   V1.0.0 首版: 3路LED + 3路按键GPIO初始化             ---2026-10-19
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
 * 描述     ： GPIO初始化, 使能GPIOB/GPIOD/GPIOE时钟, 配置3个LED
 *           (PB6/PB7/PD8)为推挽输出(初始熄灭), 3个按键
 *           (PE13/PE14/PE15)为输入模式(内部上拉, 按下为低)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /*Configure GPIO pins Output Level (LED共阳, 初始输出高=熄灭) */
  HAL_GPIO_WritePin(GPIOB, LED3_Pin | LED4_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOD, LED5_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : LED3_Pin(PB6) LED4_Pin(PB7) LED5_Pin(PD8) */
  GPIO_InitStruct.Pin = LED3_Pin | LED4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED5_Pin;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : KEY0(PE13) KEY1(PE14) KEY2(PE15)
     按下接地且无外部上拉, 必须使能内部上拉(原理图SW2/3/4节) */
  GPIO_InitStruct.Pin = KEY0_Pin | KEY1_Pin | KEY2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

}


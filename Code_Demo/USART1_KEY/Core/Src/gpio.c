/*******************************************************************************

 * @file      gpio.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-19
 * @brief     GPIO初始化: 3个LED推挽输出(初始熄灭) + 3个按键输入(内部上拉)
 *           LED3=PB6/LED4=PB7/LED5=PD8, 2kΩ上拉到3.3V, 低电平点亮;
 *           KEY1=PE13/KEY2=PE14/KEY3=PE15, 按下接地, 内部上拉

 * @note     预期现象: 上电串口打印USART1 KEY DEMO READY; 按SW2/SW3/SW4
 *           打印KEY1/2/3 PRESSED并翻转LED3/4/5; 收到PING回PONG;
 *           收到LED<n> ON|OFF|TOGGLE|STATUS控制LED并回应答
 *           (串口115200-8-N-1, 命令以
结尾, 不区分大小写)

 * @history   V1.0.0 首版: 3路LED + 3路按键GPIO初始化               ---2026-10-19
 ******************************************************************************/



/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/*************************************
 * 函数名称 ： MX_GPIO_Init
 * 描述     ： GPIO初始化, 使能GPIOA/B/D/E时钟, 配置3个LED(PB6/PB7/PD8)
 *           为推挽输出(初始熄灭), 3个按键(PE13/PE14/PE15)为输入
 *           模式(内部上拉, 按下为低)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level (LED共阳, 初始输出高=熄灭) */
  HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level (LED共阳, 初始输出高=熄灭) */
  HAL_GPIO_WritePin(GPIOB, LED3_Pin|LED4_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : KEY1_Pin KEY2_Pin KEY3_Pin */
  GPIO_InitStruct.Pin = KEY1_Pin|KEY2_Pin|KEY3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : LED5_Pin */
  GPIO_InitStruct.Pin = LED5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED5_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED3_Pin LED4_Pin */
  GPIO_InitStruct.Pin = LED3_Pin|LED4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

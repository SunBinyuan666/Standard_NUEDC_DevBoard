/*******************************************************************************

 * @file      usart.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-19
 * @brief     USART1驱动: 初始化115200-8-N-1, 逐字节中断接收状态机,
 *           printf重定向(_write)与字符串发送接口

 * @note     预期现象: 上电串口打印USART1 KEY DEMO READY; 按SW2/SW3/SW4
 *           打印KEY1/2/3 PRESSED并翻转LED3/4/5; 收到PING回PONG;
 *           收到LED<n> ON|OFF|TOGGLE|STATUS控制LED并回应答
 *           (串口115200-8-N-1, 命令以
结尾, 不区分大小写)

 * @history   V1.0.0 首版: 串口命令+按键控制LED轮询实验               ---2026-10-19
 ******************************************************************************/


/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
/* 私有包含(GCC重定向与缓冲操作所需标准头) */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* USART1_RX_BUF_SIZE 在 usart.h 中定义 */

/* 接收状态寄存器: bit15=接收完成标志, bit14=收到0x0D,
 * bit13~0=已收字节数 (正点原子经典协议, \r\n结尾) */
volatile uint16_t g_usart1_rx_sta = 0U;
volatile uint8_t  g_usart1_rx_buf[USART1_RX_BUF_SIZE];

/* 当前中断接收的写入位置(独立于rx_sta, 避免主循环复位
 * rx_sta后回调用错索引的竞态) */
static volatile uint16_t g_usart1_rx_idx = 0U;
/* USER CODE END 0 */

UART_HandleTypeDef huart1;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_PE);
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_ERR);

  /* 清标志后启动逐字节中断接收 */
  __HAL_UART_CLEAR_IDLEFLAG(&huart1);
  g_usart1_rx_idx = 0U;
  HAL_UART_Receive_IT(&huart1, (uint8_t *)&g_usart1_rx_buf[0], 1U);
  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN USART1_MspInit 1 */
    /* USART1 全局中断使能 (在MspInit里配NVIC是HAL推荐位置) */
    HAL_NVIC_SetPriority(USART1_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE END USART1_MspInit 1 */

  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

  /* USER CODE BEGIN USART1_MspDeInit 1 */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE END USART1_MspDeInit 1 */

  }
}

/* USER CODE BEGIN 1 */
/*************************************
 * 函数名称 ： USART1_SendString
 * 描述     ： 阻塞发送一个字符串(自动补\r\n)
 * 输入     ： s - 以'\0'结尾的字符串指针
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void USART1_SendString(const char *s)
{
  uint16_t len = (uint16_t)strlen(s);

  if (len > 0U)
  {
    (void)HAL_UART_Transmit(&huart1, (const uint8_t *)s, len, 100U);
  }
}

/*************************************
 * 函数名称 ： USART1_SendLine
 * 描述     ： 发送一行(字符串+\r\n), 用于命令应答
 * 输入     ： s - 以'\0'结尾的字符串指针
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void USART1_SendLine(const char *s)
{
  USART1_SendString(s);
  USART1_SendString("\r\n");
}

/*************************************
 * 函数名称 ： HAL_UART_RxCpltCallback
 * 描述     ： 逐字节中断接收状态机, 收到\r\n(0x0D 0x0A)
 *             置bit15通知主循环; 缓冲满丢弃并复位
 * 输入     ： huart - 触发回调的串口句柄
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  uint8_t ch;

  if (huart->Instance == USART1)
  {
    ch = g_usart1_rx_buf[g_usart1_rx_idx];
    if ((g_usart1_rx_sta & 0x8000U) == 0U)          /* 上一帧未处理则丢弃 */
    {
      if ((g_usart1_rx_sta & 0x4000U) != 0U)        /* 已收到0x0D */
      {
        if (ch != 0x0AU)                            /* 必须紧跟0x0A */
        {
          g_usart1_rx_sta = 0U;
          g_usart1_rx_idx = 0U;
        }
        else
        {
          g_usart1_rx_sta |= 0x8000U;               /* 一帧完成 */
          g_usart1_rx_idx = 0U;                     /* 下一帧从头写 */
        }
      }
      else                                          /* 还在攒数据 */
      {
        if (ch == 0x0D)
        {
          g_usart1_rx_sta |= 0x4000U;
        }
        else if (ch != 0x0A)                        /* 裸\n不作帧结束 */
        {
          g_usart1_rx_buf[g_usart1_rx_idx] = ch;
          g_usart1_rx_idx++;
          g_usart1_rx_sta++;
          if ((g_usart1_rx_sta & 0x3FFFU) >= (USART1_RX_BUF_SIZE - 1U))
          {
            g_usart1_rx_sta = 0U;                   /* 溢出丢弃整帧 */
            g_usart1_rx_idx = 0U;
          }
        }
        else
        {
          /* 空闲的0x0A忽略 */
        }
      }
    }
    /* 重新装载接收, 必须每次都挂否则只收一帧 */
    HAL_UART_Receive_IT(huart,
                        (uint8_t *)&g_usart1_rx_buf[g_usart1_rx_idx],
                        1U);
  }
}

/*************************************
 * 函数名称 ： _write
 * 描述     ： newlib printf重定向(GCC环境必须用_write而非
 *             __io_putchar), 输出到USART1
 * 输入     ： file - 文件描述符(未用); ptr - 数据指针;
 *             len - 数据长度
 * 输出     ： 无
 * 返回     ： 实际发送的字节数
 **************************************/
int _write(int file, char *ptr, int len)
{
  (void)file;
  (void)HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len,
                          HAL_MAX_DELAY);
  return len;
}
/* USER CODE END 1 */

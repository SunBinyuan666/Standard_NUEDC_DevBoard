/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 主程序入口与系统初始化。
  * @author         : Sunbinyuan（EEE2409036@xmu.edu.my）
  * @version        : V1.0.0
  * @date           : 2026-10-19
  * @history        : V1.0.0 标准化应用逻辑并拆分 LED 呼吸模块。
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* 头文件包含 -----------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* 私有头文件 -----------------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_breathe.h"
/* USER CODE END Includes */

/* 私有类型 -------------------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* 私有常量 -------------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCOPE_PWM_COMPARE_VALUE (500U)
/* USER CODE END PD */

/* 私有宏 ---------------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* 私有变量 -------------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* 私有函数声明 ---------------------------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* 应用程序 -------------------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  应用程序入口。
  * @retval int 返回状态。
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* 单片机配置 --------------------------------------------------------------*/

  /* 初始化 HAL、Flash 接口和 SysTick。 */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* 配置系统时钟。 */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* 初始化已配置的外设。 */
  MX_GPIO_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
    app_breathe_init();
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3,
                          SCOPE_PWM_COMPARE_VALUE);

    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

  /* USER CODE END 2 */

  /* 主循环。 */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    app_breathe_process();
  }
  /* USER CODE END 3 */
}

/**
  * @brief 配置系统时钟。
  * @retval 无。
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* 按照指定参数初始化 RCC 振荡器。 */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* 初始化 CPU、AHB 和 APB 总线时钟。 */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  发生错误时执行此函数。
  * @retval 无。
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* 可在此处添加 HAL 错误状态的报告逻辑。 */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  报告断言失败所在的源文件和行号。
  * @param  file：源文件名指针。
  * @param  line：断言失败所在的行号。
  * @retval 无。
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* 可在此处添加断言失败信息输出，例如源文件名和行号。 */
  /* USER CODE END 6 */
}
#endif /* 完整断言使能 */

/*******************************************************************************
 * @file      main.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     初始化72MHz时钟、GPIO、TIM3并调度双电机单次测试
 * @note      预期现象: 待机1s; A正转2s/停0.5s/反转2s/停0.5s;
 *           B正转2s/停0.5s/反转2s/停0.5s; 双路20%运行2s后升至
 *           50%运行2s; 最后STBY拉低, 双电机停止且不再循环
 *
 * @history   V1.0.0 首版: 完成TB6612FNG双电机功能测试        ---2026-11-12
 ******************************************************************************/
/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */

/* 头文件包含 -----------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"
#include "tim.h"

/* 私有头文件 -----------------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "motor_tb6612.h"
#include "motor_test.h"
/* USER CODE END Includes */

/* 私有函数声明 ---------------------------------------------------------------*/
void SystemClock_Config(void);

/*************************************
 * 函数名称 ： main
 * 描述     ： 完成外设与电机驱动初始化, 主循环非阻塞调度测试
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： int - 正常不会返回
 **************************************/
int main(void)
{
    /* USER CODE BEGIN 1 */
    SCB->VTOR = 0x08000000UL;
    /* USER CODE END 1 */

    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_TIM3_Init();

    /* USER CODE BEGIN 2 */
    Motor_Init();
    MotorTest_Init(HAL_GetTick());
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        MotorTest_Process(HAL_GetTick());
    }
    /* USER CODE END 3 */
}

/*************************************
 * 函数名称 ： SystemClock_Config
 * 描述     ： HSE 8MHz经PLL九倍频至72MHz, APB1定时器时钟72MHz
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                            FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/*************************************
 * 函数名称 ： Error_Handler
 * 描述     ： 出错时先关闭全部电机输出, 再关闭中断并停住程序
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    Motor_StopAll();
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/*************************************
 * 函数名称 ： assert_failed
 * 描述     ： 报告参数断言失败的源文件和行号
 * 输入     ： file - 源文件名; line - 断言行号
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    (void)file;
    (void)line;
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

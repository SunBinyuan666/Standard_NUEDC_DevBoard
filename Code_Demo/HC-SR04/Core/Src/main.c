/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */
/***********************************************************************************
 * @file      main.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     HC-SR04+SG90超声波扫描避障主程序
 *           初始化链: 时钟->GPIO->TIM3 PWM->TIM4 IC->LED->按键->扫描状态机
 *
 * @note      预期现象(设计要求§3.1):
 *           1.上电舵机回中(90度/1500us), LED5慢速心跳;
 *           2.前方畅通(>35cm)时保持中位, 每5s复测;
 *           3.前方受阻(<25cm)时依次扫描左/右(各3次采样中值),
 *             转向更开阔一侧; LED3=左/LED4=右指示;
 *           4.三方向均无效时LED5快闪(约4Hz), 2s后重试;
 *           5.SW2暂停/恢复(暂停时回中), SW3/SW4手动±5度微调;
 *           6.全程无阻塞延时, 舵机PWM由TIM3硬件维持。
 * @history   V1.0.0 首版: 应用编排层, 模块见app_*             ---2026-11-11
 **********************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"
#include "tim.h"
#include "app_hcsr04.h"
#include "app_servo.h"
#include "app_led.h"
#include "app_button.h"
#include "app_obstacle_scan.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* Private user code ---------------------------------------------------------*/

/*************************************
 * 函数名称 ： main
 * 描述     ： 主函数: 外设初始化后交由扫描状态机驱动, 主循环
 *             仅周期调用非阻塞处理函数
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 不返回
 **************************************/
int main(void)
{

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset peripherals, init Flash interface and the Systick */
  HAL_Init();


  /* Configure the system clock */
  SystemClock_Config();


  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  app_led_init();
  app_button_init();
  app_hcsr04_init();
  app_servo_init();
  app_scan_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    app_scan_process();
    app_led_process();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/*************************************
 * 函数名称 ： SystemClock_Config
 * 描述     ： 系统时钟配置: HSE 8MHz -> PLL×9 -> SYSCLK/HCLK 72MHz,
 *             APB1=36MHz(定时器倍频72MHz), APB2=72MHz, Flash等待2
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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


/*************************************
 * 函数名称 ： Error_Handler
 * 描述     ： HAL错误兜底: 关中断后死循环, 由看门狗/复位恢复
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/*************************************
 * 函数名称 ： assert_failed
 * 描述     ： 参数断言失败回调(仅USE_FULL_ASSERT启用时)
 * 输入     ： file - 源文件名
 *             line - 行号
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name
     and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

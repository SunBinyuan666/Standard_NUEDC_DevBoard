/*******************************************************************************
 * @file      main.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-20
 * @brief     SG90舵机按键控制实验: 3按键(PE13/PE14/PE15, 按下接地)
 *           调整TIM3_CH1(PA6)输出的舵机目标角度(0~180度),
 *           SW2减小/SW3增大当前步进角度, SW4在细调(1度)与
 *           粗调(5度)间切换, LED3/LED4互斥指示模式, LED5心跳500ms

 * @note     预期现象: 上电舵机回中位(90度, 约1500us), LED3亮LED4灭,
 *           LED5每500ms翻转; 按SW2/SW3角度按步进增减(边界饱和不回绕),
 *           按SW4切换步进且LED3/LED4切换; 全程非阻塞,
 *           PWM由TIM3硬件输出(72MHz/72=1us/tick, ARR=19999即50Hz)
 *           时钟HSE 8MHz x PLL9 = 72MHz, APB1=36MHz, TIM3时钟=72MHz

 * @history   V1.0.0 首版: 舵机按键控制实验(细调/粗调/心跳)       ---2026-10-20
 ******************************************************************************/

/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_servo.h"
#include "app_button.h"
#include "app_led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SERVO_STEP_FINE_DEG     1U   /* 细调步进(度/次) */
#define SERVO_STEP_COARSE_DEG   5U   /* 粗调步进(度/次) */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static uint8_t g_servo_mode = APP_LED_MODE_FINE; /* 当前调节模式 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*************************************
 * 函数名称 ： main
 * 描述     ： 主函数, 初始化时钟/GPIO/TIM3后启动舵机PWM(90度中位),
 *             默认细调模式, 主循环非阻塞执行按键扫描与心跳
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 程序不返回
 **************************************/
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, init Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  app_servo_init();                      /* 启动PWM, 90度约1500us中位 */
  g_servo_mode = APP_LED_MODE_FINE;      /* 上电默认细调(1度/步) */
  app_led_set_mode(g_servo_mode);        /* LED3亮, LED4灭 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint8_t events = app_button_scan();  /* 非阻塞按键扫描+消抖 */

    if ((events & APP_BTN_EVENT_MODE) != 0U)
    {
      g_servo_mode ^= 1U;                /* SW4: 细调<->粗调 */
      app_led_set_mode(g_servo_mode);    /* LED3/LED4互斥切换 */
    }
    if ((events & APP_BTN_EVENT_DEC) != 0U)
    {
      uint16_t step = (g_servo_mode == APP_LED_MODE_FINE)
                      ? SERVO_STEP_FINE_DEG : SERVO_STEP_COARSE_DEG;
      uint16_t angle = app_servo_get_angle();

      app_servo_set_angle((uint16_t)(angle - step)); /* 减, 内部限幅 */
    }
    if ((events & APP_BTN_EVENT_INC) != 0U)
    {
      uint16_t step = (g_servo_mode == APP_LED_MODE_FINE)
                      ? SERVO_STEP_FINE_DEG : SERVO_STEP_COARSE_DEG;
      uint16_t angle = app_servo_get_angle();

      app_servo_set_angle((uint16_t)(angle + step)); /* 增, 内部限幅 */
    }

    app_led_heartbeat();                 /* LED5 500ms翻转, 非阻塞 */
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/*************************************
 * 函数名称 ： SystemClock_Config
 * 描述     ： 配置系统时钟: HSE 8MHz经PLL 9倍频至72MHz,
 *             AHB=72MHz, APB1=36MHz(定时器x2=72MHz), APB2=72MHz,
 *             Flash等待2周期
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

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/*************************************
 * 函数名称 ： Error_Handler
 * 描述     ： HAL库错误处理, 关全局中断后死循环(可在此挂断言/日志)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/*************************************
 * 函数名称 ： assert_failed
 * 描述     ： HAL断言失败回调, 报告出错文件与行号
 * 输入     ： file - 源文件名; line - 行号
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and
     line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

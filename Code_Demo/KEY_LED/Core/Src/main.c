/*******************************************************************************

 * @file      main.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-19
 * @brief     按键轮询实验: 3个按键(PE13/PE14/PE15, 按下接地)分别
 *           控制3个LED(PB6/PB7/PD8, 低电平点亮)的开关/翻转,
 *           非阻塞周期采样消抖(HAL_GetTick时基, 10ms周期, 2次一致确认)

 * @note     预期现象: 按KEY0(SW2)翻转LED3, 按KEY1(SW3)翻转LED4,
 *           按KEY2(SW4)翻转LED5; 每键独立消抖, 按住只翻转一次
 *           (等待松开才允许下一次触发)
 *           时钟HSE 8MHz x PLL9 = 72MHz, APB1=36MHz

 * @history   V1.0.0 首版: 3按键翻转3 LED轮询实验               ---2026-10-19
 ******************************************************************************/

/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define KEY_SCAN_PERIOD_MS       10U   /* 按键扫描周期(ms) */
#define KEY_DEBOUNCE_COUNT        2U   /* 连续N次采样一致确认按下(约20ms) */
#define KEY_RELEASE_COUNT         2U   /* 连续N次采样一致确认松开 */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static uint8_t g_key_pressed_cnt[3] = {0U, 0U, 0U};  /* 按下消抖计数器 */
static uint8_t g_key_release_cnt[3] = {0U, 0U, 0U};  /* 松开消抖计数器 */
static uint8_t g_key_locked[3] = {0U, 0U, 0U};       /* 1=已触发等松开 */

static const uint16_t keyPins[3] = {KEY0_Pin, KEY1_Pin, KEY2_Pin};
/* 按键引脚: KEY0=PE13(SW2) KEY1=PE14(SW3) KEY2=PE15(SW4) */

static GPIO_TypeDef *const keyPorts[3] = {
  KEY0_GPIO_Port, KEY1_GPIO_Port, KEY2_GPIO_Port
};
/* 按键端口表; 指针本身const, 兼容HAL非const参数(规范§指针const写法) */

static const uint16_t ledPins[3] = {LED3_Pin, LED4_Pin, LED5_Pin};
/* LED引脚表: LED3=PB6 LED4=PB7 LED5=PD8, 低电平点亮 */

static GPIO_TypeDef *const ledPorts[3] = {
  LED3_GPIO_Port, LED4_GPIO_Port, LED5_GPIO_Port
};
/* LED端口表 */

static uint32_t g_key_last_scan_tick = 0U;  /* 上次扫描时刻 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Key_Scan(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/*************************************
 * 函数名称 ： main
 * 描述     ： 主函数, 初始化时钟/GPIO后进入主循环,
 *             循环体按10ms周期执行按键扫描, 不阻塞
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
    /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Key_Scan();
  }
  /* USER CODE END 3 */
}

/*************************************
 * 函数名称 ： SystemClock_Config
 * 描述     ： 配置系统时钟: HSE 8MHz经PLL 9倍频至72MHz,
 *             AHB=72MHz, APB1=36MHz, APB2=72MHz, Flash等待2周期
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
/*************************************
 * 函数名称 ： Key_Scan
 * 描述     ： 按键扫描, 按KEY_SCAN_PERIOD_MS周期采样, 连续
 *             KEY_DEBOUNCE_COUNT次读到低电平确认按下并翻转对应LED,
 *             连续KEY_RELEASE_COUNT次读到高电平确认松开后解锁;
 *             无符号减法保证tick回绕正确
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void Key_Scan(void)
{
  uint32_t now = HAL_GetTick();
  uint8_t i;

  if ((uint32_t)(now - g_key_last_scan_tick) < KEY_SCAN_PERIOD_MS)
  {
    return;                                   /* 未到扫描周期 */
  }
  g_key_last_scan_tick = now;

  for (i = 0U; i < 3U; i++)
  {
    if (HAL_GPIO_ReadPin(keyPorts[i], keyPins[i]) == GPIO_PIN_RESET)
    {
      g_key_pressed_cnt[i]++;                 /* 采样到按下(低电平) */
      g_key_release_cnt[i] = 0U;
      if ((g_key_locked[i] == 0U) &&
          (g_key_pressed_cnt[i] >= KEY_DEBOUNCE_COUNT))
      {
        HAL_GPIO_TogglePin(ledPorts[i], ledPins[i]);
        g_key_locked[i] = 1U;                 /* 按住只触发一次 */
      }
    }
    else
    {
      g_key_release_cnt[i]++;                 /* 采样到松开(高电平) */
      g_key_pressed_cnt[i] = 0U;
      if (g_key_release_cnt[i] >= KEY_RELEASE_COUNT)
      {
        g_key_locked[i] = 0U;                 /* 确认松开后解锁 */
      }
    }
  }
}
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
 * 描述     ： HAL参数断言失败回调, 报告源文件名与行号
 * 输入     ： file - 源文件名指针; line - 断言行号
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void assert_failed(uint8_t *file, uint32_t line)
{
  }
#endif /* USE_FULL_ASSERT */

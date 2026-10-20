/*******************************************************************************
 * @file      main.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.1
 * @date      2026-10-16
 * @brief     LED心跳灯实验: LED3(PB6, 低电平点亮)双闪-停顿周期闪烁
 *           （非阻塞状态机实现, 依赖SysTick时基; 时钟HSE 8MHz x PLL9 = 72MHz）
 *
 * @note     预期现象: LED3双闪(亮100ms-灭100ms-亮100ms)后停顿700ms循环
 *
 * @history   V1.0.1 注释规范化(个人编程代码规范)                ---2026-10-17
 *            V1.0.0 首版: LED心跳双闪状态机                      ---2026-10-16
 ******************************************************************************/

/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  HEARTBEAT_FIRST_ON = 0,   /* 第一声点亮 */
  HEARTBEAT_FIRST_OFF,      /* 第一声熄灭 */
  HEARTBEAT_SECOND_ON,      /* 第二声点亮 */
  HEARTBEAT_PAUSE           /* 停顿 */
} HeartbeatState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static const uint32_t heartbeatDurationMs[] = {100U, 100U, 100U, 700U};
/* 各状态持续时间(ms): 亮100/灭100/亮100/停顿700 */

static const GPIO_PinState heartbeatOutput[] = {
  GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_SET
};
/* 各状态LED输出电平; LED3共阳(2k上拉到3.3V), RESET点亮/SET熄灭 */

static HeartbeatState heartbeatState = HEARTBEAT_FIRST_ON;
/* 当前心跳状态 */

static uint32_t heartbeatStateStartedAt = 0U;
/* 当前状态的起始时刻(HAL_GetTick时间戳) */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Heartbeat_Init(void);
static void Heartbeat_Update(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/*************************************
 * 函数名称 ： main
 * 描述     ： 主函数, 初始化时钟/GPIO/心跳模块后进入主循环,
 *             循环体仅调用心跳状态机更新, 不阻塞
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
  /* USER CODE BEGIN 2 */
  Heartbeat_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Heartbeat_Update();
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
 * 函数名称 ： Heartbeat_Init
 * 描述     ： 心跳模块初始化, 进入FIRST_ON状态并点亮LED
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void Heartbeat_Init(void)
{
  heartbeatState = HEARTBEAT_FIRST_ON;
  heartbeatStateStartedAt = HAL_GetTick();
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, heartbeatOutput[heartbeatState]);
}

/*************************************
 * 函数名称 ： Heartbeat_Update
 * 描述     ： 心跳状态机更新, 当前状态超时后切换到下一状态并
 *             更新LED输出; 无符号减法保证tick回绕正确
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void Heartbeat_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((uint32_t)(now - heartbeatStateStartedAt) >=
      heartbeatDurationMs[heartbeatState])
  {
    if (heartbeatState == HEARTBEAT_PAUSE)
    {
      heartbeatState = HEARTBEAT_FIRST_ON;
    }
    else
    {
      heartbeatState = (HeartbeatState)(heartbeatState + 1);
    }

    /* 状态切换时重锚定到当前时刻, 主循环阻塞/调试暂停后不补播 */
    heartbeatStateStartedAt = now;
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin,
                      heartbeatOutput[heartbeatState]);
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

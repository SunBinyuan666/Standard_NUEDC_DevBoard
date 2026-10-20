/*******************************************************************************

 * @file      main.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-19
 * @brief     串口命令+按键控制LED实验: 3按键轮询扫描打印
 *           与翻转LED, 串口命令解析(PING/LED控制)

 * @note     预期现象: 上电串口打印USART1 KEY DEMO READY; 按SW2/SW3/SW4
 *           打印KEY1/2/3 PRESSED并翻转LED3/4/5; 收到PING回PONG;
 *           收到LED<n> ON|OFF|TOGGLE|STATUS(n=3..5,同丝印)控制LED并回应答
 *           (串口115200-8-N-1, 命令以\r\n结尾, 不区分大小写)

 * @history   V1.0.0 首版: 串口命令+按键控制LED轮询实验               ---2026-10-19
 ******************************************************************************/


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define KEY_SCAN_PERIOD_MS       10U  /* 按键扫描周期(ms) */
#define KEY_DEBOUNCE_COUNT        2U  /* 连续N次采样一致确认按下(约20ms) */
#define KEY_RELEASE_COUNT         2U  /* 连续N次采样一致确认松开 */
#define KEY_COUNT                 3U  /* 按键数量 */
#define CMD_BUF_SIZE             32U  /* 命令缓冲大小(含结束符) */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t g_key_pressed_cnt[KEY_COUNT] = {0U};  /* 按下消抖计数器 */
static uint8_t g_key_release_cnt[KEY_COUNT] = {0U};  /* 松开消抖计数器 */
static uint8_t g_key_locked[KEY_COUNT] = {0U};       /* 1=已触发等松开 */
static uint32_t g_key_last_scan_tick = 0U;           /* 上次扫描时刻 */

static const uint16_t s_key_pins[KEY_COUNT] = {
  KEY1_Pin, KEY2_Pin, KEY3_Pin
};
/* 按键引脚: KEY1=PE13(SW2) KEY2=PE14(SW3) KEY3=PE15(SW4), 按下为低 */

static GPIO_TypeDef *const s_key_ports[KEY_COUNT] = {
  KEY1_GPIO_Port, KEY2_GPIO_Port, KEY3_GPIO_Port
};
/* 按键端口表; 指针本身const, 兼容HAL非const参数 */

static const uint16_t s_led_pins[KEY_COUNT] = {
  LED3_Pin, LED4_Pin, LED5_Pin
};
/* LED引脚表: LED3=PB6 LED4=PB7 LED5=PD8, 低电平点亮, 与按键一一对应 */

static GPIO_TypeDef *const s_led_ports[KEY_COUNT] = {
  LED3_GPIO_Port, LED4_GPIO_Port, LED5_GPIO_Port
};
/* LED端口表 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Key_Scan(void);
static void Cmd_Process(void);
static void Led_Set(uint8_t idx, uint8_t on);
static uint8_t Led_Get(uint8_t idx);
static void Cmd_Handle(char *cmd);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*************************************
 * 函数名称 ： main
 * 描述     ： 主函数, 初始化时钟/GPIO/串口后打印banner,
 *           主循环轮询执行按键扫描与串口命令处理
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 程序不返回
 **************************************/
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* 复位所有外设, 初始化Flash接口与SysTick */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("USART1 KEY DEMO READY\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Key_Scan();      /* 按键: 打印KEYx PRESSED并翻转对应LED */
    Cmd_Process();   /* 串口命令: PING / LEDx ON|OFF|TOGGLE|STATUS */
  }
  /* USER CODE END 3 */
}

/*************************************
 * 函数名称 ： SystemClock_Config
 * 描述     ： 配置系统时钟: HSE 8MHz经PLL 9倍频至72MHz,
 *           AHB=72MHz, APB1=36MHz, APB2=72MHz, Flash等待2周期
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
 * 描述     ： 非阻塞按键扫描, 10ms周期采样, 连续2次低电平
 *             确认按下后打印KEYx PRESSED并翻转对应LED,
 *             连续2次高电平确认松开后才允许下次触发
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

  for (i = 0U; i < KEY_COUNT; i++)
  {
    if (HAL_GPIO_ReadPin(s_key_ports[i], s_key_pins[i]) == GPIO_PIN_RESET)
    {
      g_key_pressed_cnt[i]++;                 /* 采样到按下(低电平) */
      g_key_release_cnt[i] = 0U;
      if ((g_key_locked[i] == 0U) &&
          (g_key_pressed_cnt[i] >= KEY_DEBOUNCE_COUNT))
      {
        printf("KEY%u PRESSED\r\n", (unsigned)(i + 1U));
        HAL_GPIO_TogglePin(s_led_ports[i], s_led_pins[i]);
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

/*************************************
 * 函数名称 ： Cmd_Process
 * 描述     ： 串口命令处理, 收到完整一帧(\r\n结尾)后取出
 *             转大写交给Cmd_Handle解析, 然后复位接收状态
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void Cmd_Process(void)
{
  char cmd[CMD_BUF_SIZE];
  uint16_t len;
  uint16_t i;

  if ((g_usart1_rx_sta & 0x8000U) == 0U)
  {
    return;                                   /* 无完整帧 */
  }

  len = g_usart1_rx_sta & 0x3FFFU;
  if (len >= CMD_BUF_SIZE)
  {
    len = CMD_BUF_SIZE - 1U;                  /* 截断超长命令 */
  }
  for (i = 0U; i < len; i++)
  {
    cmd[i] = (char)toupper((int)g_usart1_rx_buf[i]);
  }
  cmd[len] = '\0';
  g_usart1_rx_sta = 0U;                       /* 复位, 允许接收下一帧 */

  if (len > 0U)
  {
    Cmd_Handle(cmd);
  }
}

/*************************************
 * 函数名称 ： Cmd_Handle
 * 描述     ： 命令解析执行: PING->PONG;
 *             LEDx ON/OFF/TOGGLE|STATUS (x=3..5, 与板上丝印一致,
 *             LED3=PB6/LED4=PB7/LED5=PD8); 未知命令回ERR
 * 输入     ： cmd - 已转大写、以'\0'结尾的命令串
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void Cmd_Handle(char *cmd)
{
  uint8_t idx;
  char *arg;

  if (strcmp(cmd, "PING") == 0)
  {
    printf("PONG\r\n");
    return;
  }

  /* 命令编号直接用板上丝印: LED3/LED4/LED5 (PB6/PB7/PD8) */
  if ((strncmp(cmd, "LED", 3U) == 0) && (isdigit((int)cmd[3]) != 0) &&
      (cmd[3] >= '3') && (cmd[3] <= '5'))
  {
    idx = (uint8_t)(cmd[3] - '3');            /* LED3->0 LED4->1 LED5->2 */
    arg = &cmd[4];
    if (strcmp(arg, " ON") == 0)
    {
      Led_Set(idx, 1U);
      printf("LED%u ON OK\r\n", (unsigned)(idx + 3U));
    }
    else if (strcmp(arg, " OFF") == 0)
    {
      Led_Set(idx, 0U);
      printf("LED%u OFF OK\r\n", (unsigned)(idx + 3U));
    }
    else if (strcmp(arg, " TOGGLE") == 0)
    {
      HAL_GPIO_TogglePin(s_led_ports[idx], s_led_pins[idx]);
      printf("LED%u TOGGLE OK\r\n", (unsigned)(idx + 3U));
    }
    else if (strcmp(arg, " STATUS") == 0)
    {
      printf("LED%u %s\r\n", (unsigned)(idx + 3U),
             (Led_Get(idx) != 0U) ? "ON" : "OFF");
    }
    else
    {
      printf("ERR USAGE: LED<n> ON|OFF|TOGGLE|STATUS (n=3..5)\r\n");
    }
    return;
  }

  printf("ERR UNKNOWN CMD\r\n");
}

/*************************************
 * 函数名称 ： Led_Set
 * 描述     ： 设置LED开关(低电平点亮, 写RESET=亮)
 * 输入     ： idx - LED索引0..2(LED3..LED5); on - 1亮0灭
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void Led_Set(uint8_t idx, uint8_t on)
{
  HAL_GPIO_WritePin(s_led_ports[idx], s_led_pins[idx],
                    (on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/*************************************
 * 函数名称 ： Led_Get
 * 描述     ： 读LED当前状态(低电平=亮)
 * 输入     ： idx - LED索引0..2(LED3..LED5)
 * 输出     ： 无
 * 返回     ： 1=亮 0=灭
 **************************************/
static uint8_t Led_Get(uint8_t idx)
{
  return (HAL_GPIO_ReadPin(s_led_ports[idx], s_led_pins[idx])
          == GPIO_PIN_RESET) ? 1U : 0U;
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
 * 描述     ： HAL参数断言失败回调, 报告源文件名与行号
 * 输入     ： file - 源文件名指针; line - 断言行号
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* 用户可在此报告断言失败的文件名与行号 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

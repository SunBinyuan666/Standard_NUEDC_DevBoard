/***********************************************************************************
 * @file      app_hcsr04.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     HC-SR04超声波测距模块: TRIG触发/ECHO输入捕获/脉宽转距离
 *           (TIM4_CH4 -> PB9, 1us/tick输入捕获; TRIG -> PB0推挽输出)
 *
 * @note      测距流程(设计要求§6.1): 空闲->TRIG>=10us高脉冲->等ECHO上升沿
 *           ->等下降沿->脉宽差值(模65536防回绕)->cm换算; 30ms内未完成
 *           视为超时。TRIG间隔>=60ms由上层状态机保证。
 *           中断回调只记录捕获值与置完成标志, 距离换算在主循环完成。
 * @history   V1.0.0 首版: 单次测距状态机+中值滤波接口           ---2026-11-11
 **********************************************************************************/
#include "app_hcsr04.h"

/* ==========================================================================
 *  私有类型定义
 * ========================================================================== */
/* 单次测距内部阶段 */
typedef enum
{
    HCSR04_SEQ_IDLE = 0,     /* 空闲, 可接受新触发 */
    HCSR04_SEQ_WAIT_RISE,    /* 已发TRIG, 等ECHO上升沿 */
    HCSR04_SEQ_WAIT_FALL     /* 已捕获上升沿, 等下降沿 */
} hcsr04_seq_t;

/* ==========================================================================
 *  私有变量
 * ========================================================================== */
static hcsr04_seq_t g_seq = HCSR04_SEQ_IDLE;          /* 当前测距阶段 */
static volatile uint16_t g_rise_cnt = 0U;             /* 上升沿捕获值(ISR写) */
static volatile uint16_t g_fall_cnt = 0U;             /* 下降沿捕获值(ISR写) */
static volatile uint8_t g_done = 0U;                  /* 完成标志(ISR写/主循环清) */
static uint32_t g_trig_tick = 0U;                     /* 触发时刻(HAL tick) */
static uint16_t g_last_us = 0U;                       /* 最近一次有效脉宽(us) */

/* ==========================================================================
 *  私有函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： hcsr04_delay_10us
 * 描述     ： TRIG高电平保持约10us的忙等延时
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void hcsr04_delay_10us(void)
{
    /* 72MHz下 720 cycles = 10us; 每次循环约3~4周期, 取保守180次 */
    volatile uint32_t cnt = 180U;

    while (cnt > 0U)
    {
        cnt--;
        __NOP();
    }
}

/*************************************
 * 函数名称 ： hcsr04_read_cm
 * 描述     ： 读取最近一次有效脉宽并换算为距离
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 距离(cm); 无有效样本返回 HCSR04_DIST_INVALID
 **************************************/
static uint32_t hcsr04_read_cm(void)
{
    uint32_t echo_us;

    if (g_last_us == 0U)
    {
        return HCSR04_DIST_INVALID;
    }

    /* cm = us / 58; 上限30ms脉宽 -> 约517cm, uint32足够无溢出 */
    echo_us = (uint32_t)g_last_us;
    return echo_us / 58U;
}

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： app_hcsr04_init
 * 描述     ： 超声波模块初始化, 启动TIM4_CH4输入捕获中断
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_hcsr04_init(void)
{
    /* 极性初始化为上升沿(设计要求§5.3), 等待第一次触发 */
    __HAL_TIM_SET_CAPTUREPOLARITY(&htim4, TIM_CHANNEL_4,
                                  TIM_INPUTCHANNELPOLARITY_RISING);
    HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_4);
    g_seq = HCSR04_SEQ_WAIT_RISE;
    g_done = 0U;
}

/*************************************
 * 函数名称 ： app_hcsr04_trigger
 * 描述     ： 发出一次TRIG脉冲(>=10us高), 开始新一轮测距
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_hcsr04_trigger(void)
{
    /* 复位捕获顺序状态, 忽略残留的旧边沿 */
    g_seq = HCSR04_SEQ_WAIT_RISE;
    g_done = 0U;
    g_last_us = 0U;
    g_trig_tick = HAL_GetTick();

    HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin, GPIO_PIN_SET);
    hcsr04_delay_10us();
    HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin,
                      GPIO_PIN_RESET);
}

/*************************************
 * 触发后主循环轮询: 0=进行中 1=完成 2=超时
 **************************************/
uint8_t app_hcsr04_poll(void)
{
    uint32_t now = HAL_GetTick();

    if (g_done != 0U)
    {
        return HCSR04_POLL_DONE;
    }

    /* 30ms 未形成完整脉冲判超时(设计要求§5.3/§6.1) */
    if ((now - g_trig_tick) > HCSR04_ECHO_TIMEOUT_MS)
    {
        g_seq = HCSR04_SEQ_IDLE;
        return HCSR04_POLL_TIMEOUT;
    }

    return HCSR04_POLL_BUSY;
}

/*************************************
 * 函数名称 ： app_hcsr04_get_distance
 * 描述     ： 取最近一次有效测距结果(cm)
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 距离(cm), 无有效值返回 HCSR04_DIST_INVALID
 **************************************/
uint32_t app_hcsr04_get_distance(void)
{
    return hcsr04_read_cm();
}

/* ==========================================================================
 *  中断回调(ISR上下文, 只记录数据)
 * ========================================================================== */

/*************************************
 * 函数名称 ： HAL_TIM_IC_CaptureCallback
 * 描述     ： TIM4_CH4输入捕获中断回调: 上升沿记录起点并切换下降沿,
 *             下降沿计算脉宽并恢复上升沿
 * 输入     ： htim - 定时器句柄
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if ((htim->Instance == TIM4) && (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4))
    {
        uint16_t cnt = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);

        if (g_seq == HCSR04_SEQ_WAIT_RISE)
        {
            g_rise_cnt = cnt;
            /* 切换为下降沿捕获 */
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_4,
                                          TIM_INPUTCHANNELPOLARITY_FALLING);
            g_seq = HCSR04_SEQ_WAIT_FALL;
        }
        else if (g_seq == HCSR04_SEQ_WAIT_FALL)
        {
            g_fall_cnt = cnt;
            /* 脉宽 = 模65536差值, 天然处理16位回绕 */
            g_last_us = (uint16_t)(g_fall_cnt - g_rise_cnt);
            /* 恢复上升沿等待下一次测量 */
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_4,
                                          TIM_INPUTCHANNELPOLARITY_RISING);
            g_seq = HCSR04_SEQ_IDLE;
            g_done = 1U;
        }
        else
        {
            /* IDLE状态下的意外边沿, 忽略 */
        }
    }
}

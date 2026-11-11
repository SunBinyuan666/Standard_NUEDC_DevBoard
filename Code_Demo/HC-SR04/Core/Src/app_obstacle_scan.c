/***********************************************************************************
 * @file      app_obstacle_scan.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     超声波扫描避障状态机(设计要求§6/§6.5)
 *           INIT->CENTER_SETTLE->CENTER_MEASURE
 *             ├─中位畅通->FORWARD_HOLD->定期复测
 *             └─中位受阻/未知->MOVE_LEFT->LEFT_SETTLE->LEFT_MEASURE
 *                ->MOVE_RIGHT->RIGHT_SETTLE->RIGHT_MEASURE->SELECT
 *                   ├─有有效畅通方向->POINT->HOLD->重新评估
 *                   └─无->NO_CLEAR_OR_FAULT->延时重试
 *
 * @note      预期现象: 前方畅通时舵机保持中位不摆动; 前方受阻时
 *           依次测左/右并转向更开阔一侧; 三方向均无效时LED5快闪
 *           并在安全角度等待重试; 任何状态不长时间阻塞。
 * @history   V1.0.0 首版: 全状态机+中值滤波+滞回判定+平局保持      ---2026-11-11
 **********************************************************************************/
#include "app_obstacle_scan.h"
#include "app_hcsr04.h"
#include "app_servo.h"
#include "app_led.h"
#include "app_button.h"

/* ==========================================================================
 *  扫描配置常量(设计要求§6.2/§6.3)
 * ========================================================================== */
#define SCAN_ANGLE_CENTER_DEG     90U   /* 中位角度(度), 相对舵机行程 */
#define SCAN_ANGLE_LEFT_DEG       70U   /* 左侧角度: 中位-20度(首版保守) */
#define SCAN_ANGLE_RIGHT_DEG      110U  /* 右侧角度: 中位+20度 */
#define SCAN_SERVO_SETTLE_MS     200U   /* 舵机换向后稳定等待(ms) */
#define SCAN_TRIG_INTERVAL_MS     60U   /* 两次TRIG最小间隔(ms) */
#define SCAN_SAMPLES_PER_DIR       3U   /* 每方向采样次数, 取中值 */
#define SCAN_HOLD_MS            1000U   /* 方向保持时间(ms), 期满复测 */
#define SCAN_RECHECK_MS         5000U   /* 前方畅通时复测周期(ms) */
#define SCAN_NO_WAY_RETRY_MS    2000U   /* 无安全方向重试间隔(ms) */
#define SCAN_OBSTACLE_CM         25U   /* 障碍阈值(cm): 低于此判受阻 */
#define SCAN_CLEAR_CM            35U   /* 畅通阈值(cm): 高于此判畅通 */
#define SCAN_HYSTERESIS_MARGIN  10U   /* 平局切换裕量(cm), 差小于此保持原向 */

/* 人工暂停微调(设计要求§3.2) */
#define SCAN_MANUAL_STEP_DEG      5U   /* 手动微调步进(度) */

/* ==========================================================================
 *  私有类型定义
 * ========================================================================== */
/* 状态机状态(设计要求§6.5) */
typedef enum
{
    SCAN_ST_INIT = 0,        /* 初始化: 舵机回中 */
    SCAN_ST_CENTER_SETTLE,   /* 中位稳定等待 */
    SCAN_ST_CENTER_MEASURE,  /* 中位采样(3次中值) */
    SCAN_ST_FORWARD_HOLD,    /* 前方畅通保持, 定期复测 */
    SCAN_ST_MOVE_LEFT,       /* 转向左侧 */
    SCAN_ST_LEFT_SETTLE,
    SCAN_ST_LEFT_MEASURE,
    SCAN_ST_MOVE_RIGHT,
    SCAN_ST_RIGHT_SETTLE,
    SCAN_ST_RIGHT_MEASURE,
    SCAN_ST_SELECT,          /* 三方向数据齐, 方向决策 */
    SCAN_ST_POINT,           /* 转向选定方向 */
    SCAN_ST_DIR_HOLD,        /* 方向保持, 期满重新评估 */
    SCAN_ST_NO_WAY,          /* 无安全方向/故障, 等待重试 */
    SCAN_ST_PAUSED           /* 人工暂停, 手动微调 */
} scan_state_t;

/* 单方向采样上下文 */
typedef struct
{
    uint8_t phase;           /* 子阶段: 0=等间隔触发 1=等完成/超时 */
    uint8_t sample_idx;      /* 已完成采样次数 */
    uint8_t valid_cnt;       /* 有效样本数 */
    uint8_t timeout_cnt;     /* 连续超时计数(故障判定) */
    uint32_t dist[SCAN_SAMPLES_PER_DIR];  /* 样本(cm) */
    uint32_t state_tick;     /* 本状态进入时刻(ms) */
    uint32_t last_trig_tick; /* 上次TRIG时刻 */
} dir_measure_t;

/* 方向判定结果 */
typedef enum
{
    DIR_RESULT_BLOCKED = 0,  /* 受阻(有效且低于障碍阈值) */
    DIR_RESULT_CLEAR,        /* 畅通(有效且高于畅通阈值) */
    DIR_RESULT_CAUTION,      /* 谨慎区(阈值之间) */
    DIR_RESULT_UNKNOWN       /* 未知(有效样本<2或超时) */
} dir_result_t;

/* ==========================================================================
 *  私有变量
 * ========================================================================== */
static scan_state_t g_state = SCAN_ST_INIT;         /* 当前状态 */
static uint32_t g_state_tick = 0U;                  /* 状态进入时刻 */
static uint32_t g_last_trig_tick = 0U;              /* 全局TRIG时刻 */
static dir_measure_t g_dir_ctx;                     /* 当前方向采样上下文 */
static uint32_t g_dist_center = HCSR04_DIST_INVALID;/* 中位代表距离 */
static uint32_t g_dist_left = HCSR04_DIST_INVALID;  /* 左侧代表距离 */
static uint32_t g_dist_right = HCSR04_DIST_INVALID; /* 右侧代表距离 */
static dir_result_t g_res_center = DIR_RESULT_UNKNOWN;
static dir_result_t g_res_left = DIR_RESULT_UNKNOWN;
static dir_result_t g_res_right = DIR_RESULT_UNKNOWN;
static uint8_t g_selected_dir = SCAN_DIR_CENTER;    /* 当前指向 */
static uint8_t g_prev_selected = SCAN_DIR_CENTER;   /* 平局保持用 */

/* ==========================================================================
 *  私有函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： scan_set_state
 * 描述     ： 状态转移助手: 记录进入时刻
 * 输入     ： st - 目标状态
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void scan_set_state(scan_state_t st)
{
    g_state = st;
    g_state_tick = HAL_GetTick();
}

/*************************************
 * 函数名称 ： scan_dir_angle
 * 描述     ： 方向编号转舵机角度
 * 输入     ： dir - 方向编号(SCAN_DIR_xxx)
 * 输出     ： 无
 * 返回     ： 舵机角度(度)
 **************************************/
static uint16_t scan_dir_angle(uint8_t dir)
{
    switch (dir)
    {
        case SCAN_DIR_LEFT:  return SCAN_ANGLE_LEFT_DEG;
        case SCAN_DIR_RIGHT: return SCAN_ANGLE_RIGHT_DEG;
        default:             return SCAN_ANGLE_CENTER_DEG;
    }
}

/*************************************
 * 函数名称 ： scan_get_median
 * 描述     ： 对有效样本冒泡取中值(每方向仅3样本, 冒泡足够)
 * 输入     ： dist - 样本数组
 *             cnt - 有效样本数(>=2)
 * 输出     ： 无
 * 返回     ： 中值代表距离(cm)
 **************************************/
static uint32_t scan_get_median(uint32_t *dist, uint8_t cnt)
{
    uint8_t i;
    uint8_t j;

    for (i = 0U; i < cnt; i++)
    {
        for (j = 0U; (uint8_t)(j + 1U) < cnt; j++)
        {
            if (dist[j] > dist[j + 1U])
            {
                uint32_t tmp = dist[j];
                dist[j] = dist[j + 1U];
                dist[j + 1U] = tmp;
            }
        }
    }
    return dist[cnt / 2U];
}

/*************************************
 * 函数名称 ： scan_classify
 * 描述     ： 单方向判定: 按滞回阈值分类(设计要求§6.3)
 * 输入     ： dm - 该方向采样上下文
 * 输出     ： 无
 * 返回     ： 判定结果(BLOCKED/CLEAR/CAUTION/UNKNOWN)
 **************************************/
static dir_result_t scan_classify(const dir_measure_t *dm)
{
    uint32_t median;
    uint32_t tmp[SCAN_SAMPLES_PER_DIR];

    if (dm->valid_cnt < 2U)
    {
        return DIR_RESULT_UNKNOWN;   /* 有效样本<2 -> 未知, 不参与比较 */
    }

    /* 拷贝到本地再排序, 保持入参只读 */
    for (uint8_t k = 0U; k < dm->valid_cnt; k++)
    {
        tmp[k] = dm->dist[k];
    }
    median = scan_get_median(tmp, dm->valid_cnt);
    if (median < SCAN_OBSTACLE_CM)
    {
        return DIR_RESULT_BLOCKED;
    }
    if (median > SCAN_CLEAR_CM)
    {
        return DIR_RESULT_CLEAR;
    }
    return DIR_RESULT_CAUTION;
}

/*************************************
 * 函数名称 ： scan_dir_measure_step
 * 描述     ： 单方向测量子状态机: 等间隔触发(>=60ms)->轮询完成/
 *             超时->存样本, 采满3次返回1
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 1=该方向测量完成, 0=进行中
 **************************************/
static uint8_t scan_dir_measure_step(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t poll;

    if (g_dir_ctx.phase == 0U)
    {
        /* 等待触发间隔 */
        if ((now - g_dir_ctx.last_trig_tick) < SCAN_TRIG_INTERVAL_MS)
        {
            return 0U;
        }
        app_hcsr04_trigger();
        g_last_trig_tick = now;
        g_dir_ctx.last_trig_tick = now;
        g_dir_ctx.phase = 1U;
        return 0U;
    }

    /* phase 1: 轮询本次测量结果 */
    poll = app_hcsr04_poll();
    if (poll == HCSR04_POLL_BUSY)
    {
        return 0U;
    }

    if (poll == HCSR04_POLL_DONE)
    {
        uint32_t d = app_hcsr04_get_distance();

        /* 过近(最小量程内)记为无效样本, 不伪造正常距离 */
        if ((d != HCSR04_DIST_INVALID) && (d >= 2U))
        {
            if (g_dir_ctx.sample_idx < SCAN_SAMPLES_PER_DIR)
            {
                g_dir_ctx.dist[g_dir_ctx.sample_idx] = d;
                g_dir_ctx.sample_idx++;
                g_dir_ctx.valid_cnt++;
            }
        }
        else
        {
            g_dir_ctx.timeout_cnt++;
        }
    }
    else
    {
        /* 超时: 计数, 样本留空 */
        g_dir_ctx.timeout_cnt++;
    }

    /* 完成一轮, 判断是否采满 */
    if (g_dir_ctx.sample_idx >= SCAN_SAMPLES_PER_DIR)
    {
        return 1U;
    }

    g_dir_ctx.phase = 0U;   /* 回到等间隔, 触发下一次 */
    return 0U;
}

/*************************************
 * 函数名称 ： scan_start_dir_measure
 * 描述     ： 复位单方向采样上下文
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
static void scan_start_dir_measure(void)
{
    g_dir_ctx.phase = 0U;
    g_dir_ctx.sample_idx = 0U;
    g_dir_ctx.valid_cnt = 0U;
    g_dir_ctx.timeout_cnt = 0U;
    g_dir_ctx.state_tick = HAL_GetTick();
    g_dir_ctx.last_trig_tick = g_last_trig_tick;
}

/*************************************
 * 函数名称 ： scan_select_direction
 * 描述     ： 三方向决策(设计要求§6.4表): 中位畅通优先回中;
 *             中位受阻时选有效畅通侧, 平局差<10cm保持原向
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 选定方向(SCAN_DIR_xxx)
 **************************************/
static uint8_t scan_select_direction(void)
{
    /* 中位有效且畅通 -> 回中(优先) */
    if (g_res_center == DIR_RESULT_CLEAR)
    {
        return SCAN_DIR_CENTER;
    }

    /* 中位畅通优先级最高; 中位受阻/未知时比较左右 */
    if ((g_res_left == DIR_RESULT_CLEAR) && (g_res_right == DIR_RESULT_CLEAR))
    {
        /* 双侧畅通: 距离差<10cm保持原向, 否则选距离大的一侧 */
        if (g_dist_left > g_dist_right)
        {
            if ((g_dist_left - g_dist_right) < SCAN_HYSTERESIS_MARGIN)
            {
                return g_prev_selected;
            }
            return SCAN_DIR_LEFT;
        }
        if ((g_dist_right - g_dist_left) < SCAN_HYSTERESIS_MARGIN)
        {
            return g_prev_selected;
        }
        return SCAN_DIR_RIGHT;
    }

    if (g_res_left == DIR_RESULT_CLEAR)
    {
        return SCAN_DIR_LEFT;
    }
    if (g_res_right == DIR_RESULT_CLEAR)
    {
        return SCAN_DIR_RIGHT;
    }

    /* 无有效畅通方向 */
    return SCAN_DIR_NO_WAY;
}

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： app_scan_init
 * 描述     ： 扫描状态机初始化, 舵机回中
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_scan_init(void)
{
    g_state = SCAN_ST_INIT;
    g_state_tick = HAL_GetTick();
}

/*************************************
 * 函数名称 ： app_scan_process
 * 描述     ： 主循环周期调用; 驱动状态机+按键+LED, 全程非阻塞
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_scan_process(void)
{
    uint32_t now = HAL_GetTick();

    /* 按键处理(所有状态共享) */
    app_button_process();
    if (app_button_get_event(BUTTON_ID_SW2) != 0U)
    {
        if (g_state == SCAN_ST_PAUSED)
        {
            /* 恢复: 回中重测 */
            app_servo_set_angle(SCAN_ANGLE_CENTER_DEG);
            scan_start_dir_measure();
            scan_set_state(SCAN_ST_CENTER_SETTLE);
            return;
        }
        /* 暂停: 舵机回中(设计要求§3.2) */
        g_prev_selected = g_selected_dir;
        app_servo_set_angle(SCAN_ANGLE_CENTER_DEG);
        scan_set_state(SCAN_ST_PAUSED);
        return;
    }

    if (g_state == SCAN_ST_PAUSED)
    {
        /* 暂停态手动微调: 受安全角度范围限制 */
        if (app_button_get_event(BUTTON_ID_SW3) != 0U)
        {
            uint16_t cur = app_servo_get_angle();
            if (cur >= (SCAN_MANUAL_STEP_DEG + SERVO_MIN_ANGLE_DEG))
            {
                app_servo_set_angle(cur - SCAN_MANUAL_STEP_DEG);
            }
        }
        if (app_button_get_event(BUTTON_ID_SW4) != 0U)
        {
            uint16_t cur = app_servo_get_angle();
            if (cur <= (SERVO_MAX_ANGLE_DEG - SCAN_MANUAL_STEP_DEG))
            {
                app_servo_set_angle(cur + SCAN_MANUAL_STEP_DEG);
            }
        }
        app_led_set_state(LED_STATE_PAUSED, g_selected_dir);
        return;
    }

    switch (g_state)
    {
        case SCAN_ST_INIT:
            app_servo_set_angle(SCAN_ANGLE_CENTER_DEG);
            app_led_set_state(LED_STATE_SCANNING, SCAN_DIR_CENTER);
            scan_start_dir_measure();
            scan_set_state(SCAN_ST_CENTER_SETTLE);
            break;

        case SCAN_ST_CENTER_SETTLE:
        case SCAN_ST_LEFT_SETTLE:
        case SCAN_ST_RIGHT_SETTLE:
            /* 舵机换向稳定等待200ms */
            if ((now - g_state_tick) >= SCAN_SERVO_SETTLE_MS)
            {
                scan_start_dir_measure();
                scan_set_state((g_state == SCAN_ST_CENTER_SETTLE)
                               ? SCAN_ST_CENTER_MEASURE
                               : ((g_state == SCAN_ST_LEFT_SETTLE)
                                  ? SCAN_ST_LEFT_MEASURE
                                  : SCAN_ST_RIGHT_MEASURE));
            }
            break;

        case SCAN_ST_CENTER_MEASURE:
            app_led_set_state(LED_STATE_SCANNING, SCAN_DIR_CENTER);
            if (scan_dir_measure_step() != 0U)
            {
                g_res_center = scan_classify(&g_dir_ctx);
                g_dist_center = (g_res_center == DIR_RESULT_UNKNOWN)
                                ? HCSR04_DIST_INVALID
                                : scan_get_median(g_dir_ctx.dist,
                                                  g_dir_ctx.valid_cnt);
                if (g_res_center == DIR_RESULT_CLEAR)
                {
                    g_selected_dir = SCAN_DIR_CENTER;
                    app_led_set_state(LED_STATE_NORMAL, SCAN_DIR_CENTER);
                    scan_set_state(SCAN_ST_FORWARD_HOLD);
                }
                else
                {
                    /* 中位受阻/未知/谨慎 -> 扫描左侧 */
                    app_servo_set_angle(SCAN_ANGLE_LEFT_DEG);
                    scan_set_state(SCAN_ST_MOVE_LEFT);
                }
            }
            break;

        case SCAN_ST_MOVE_LEFT:
            app_servo_set_angle(SCAN_ANGLE_LEFT_DEG);
            app_led_set_state(LED_STATE_SCANNING, SCAN_DIR_LEFT);
            scan_set_state(SCAN_ST_LEFT_SETTLE);
            break;

        case SCAN_ST_LEFT_MEASURE:
            if (scan_dir_measure_step() != 0U)
            {
                g_res_left = scan_classify(&g_dir_ctx);
                g_dist_left = (g_res_left == DIR_RESULT_UNKNOWN)
                              ? HCSR04_DIST_INVALID
                              : scan_get_median(g_dir_ctx.dist,
                                                g_dir_ctx.valid_cnt);
                app_servo_set_angle(SCAN_ANGLE_RIGHT_DEG);
                scan_set_state(SCAN_ST_MOVE_RIGHT);
            }
            break;

        case SCAN_ST_MOVE_RIGHT:
            app_servo_set_angle(SCAN_ANGLE_RIGHT_DEG);
            app_led_set_state(LED_STATE_SCANNING, SCAN_DIR_RIGHT);
            scan_set_state(SCAN_ST_RIGHT_SETTLE);
            break;

        case SCAN_ST_RIGHT_MEASURE:
            if (scan_dir_measure_step() != 0U)
            {
                g_res_right = scan_classify(&g_dir_ctx);
                g_dist_right = (g_res_right == DIR_RESULT_UNKNOWN)
                               ? HCSR04_DIST_INVALID
                               : scan_get_median(g_dir_ctx.dist,
                                                 g_dir_ctx.valid_cnt);
                scan_set_state(SCAN_ST_SELECT);
            }
            break;

        case SCAN_ST_SELECT:
        {
            uint8_t sel = scan_select_direction();

            if (sel == SCAN_DIR_NO_WAY)
            {
                app_led_set_state(LED_STATE_NO_WAY, SCAN_DIR_CENTER);
                scan_set_state(SCAN_ST_NO_WAY);
            }
            else
            {
                g_prev_selected = g_selected_dir;
                g_selected_dir = sel;
                app_servo_set_angle(scan_dir_angle(sel));
                scan_set_state(SCAN_ST_POINT);
            }
            break;
        }

        case SCAN_ST_POINT:
            /* 已在SELECT里下发角度, 显示方向灯后进入保持 */
            app_led_set_state((g_selected_dir == SCAN_DIR_LEFT)
                              ? LED_STATE_POINT_LEFT
                              : ((g_selected_dir == SCAN_DIR_RIGHT)
                                 ? LED_STATE_POINT_RIGHT
                                 : LED_STATE_NORMAL),
                              g_selected_dir);
            scan_set_state(SCAN_ST_DIR_HOLD);
            break;

        case SCAN_ST_DIR_HOLD:
            app_led_set_state((g_selected_dir == SCAN_DIR_LEFT)
                              ? LED_STATE_POINT_LEFT
                              : ((g_selected_dir == SCAN_DIR_RIGHT)
                                 ? LED_STATE_POINT_RIGHT
                                 : LED_STATE_NORMAL),
                              g_selected_dir);
            /* 保持期满 -> 回中重新评估 */
            if ((now - g_state_tick) >= SCAN_HOLD_MS)
            {
                app_servo_set_angle(SCAN_ANGLE_CENTER_DEG);
                scan_set_state(SCAN_ST_CENTER_SETTLE);
            }
            break;

        case SCAN_ST_FORWARD_HOLD:
            app_led_set_state(LED_STATE_NORMAL, SCAN_DIR_CENTER);
            /* 前方畅通保持, 期满复测中位 */
            if ((now - g_state_tick) >= SCAN_RECHECK_MS)
            {
                scan_start_dir_measure();
                scan_set_state(SCAN_ST_CENTER_MEASURE);
            }
            break;

        case SCAN_ST_NO_WAY:
            /* 无安全方向: 云台回中, 等待重试 */
            app_led_set_state(LED_STATE_NO_WAY, SCAN_DIR_CENTER);
            if ((now - g_state_tick) >= SCAN_NO_WAY_RETRY_MS)
            {
                app_servo_set_angle(SCAN_ANGLE_CENTER_DEG);
                scan_set_state(SCAN_ST_CENTER_SETTLE);
            }
            break;

        default:
            scan_set_state(SCAN_ST_INIT);
            break;
    }
}

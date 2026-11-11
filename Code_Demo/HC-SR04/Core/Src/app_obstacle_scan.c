/***********************************************************************************
 * @file      app_obstacle_scan.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V2.1.0
 * @date      2026-11-11
 * @brief     超声波扫描避障状态机
 *           INIT->CENTER_SETTLE->CENTER_MEASURE
 *             ├─前方无物体->FORWARD_HOLD->定期复测
 *             └─前方有物体->MOVE_LEFT->LEFT_SETTLE->LEFT_MEASURE
 *                ->MOVE_RIGHT->RIGHT_SETTLE->RIGHT_MEASURE->SELECT
 *                   ├─有无物体侧->POINT->HOLD->回中重测
 *                   └─两侧都有物体->NO_WAY->延时重试
 *
 * @note      按键: SW2=暂停/恢复, SW3=位置复原(舵机回中),
 *           SW4=暂停态手动+5度微调。
 *           扫描幅度中位±60度(30~150), 舵机限幅0~180度(SG90),
 *           任何转动不超过360度上限。
 * @history   V1.0.0 首版: 全状态机+中值滤波+滞回判定+平局保持      ---2026-11-11
 *           V2.1.0 逻辑重构: 二值判定(有/无物体), 扫描角±20->±60,
 *                    SW3改位置复原                              ---2026-11-11
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
#define SCAN_ANGLE_LEFT_DEG       30U   /* 左侧角度: 中位-60度(大幅摆头) */
#define SCAN_ANGLE_RIGHT_DEG     150U   /* 右侧角度: 中位+60度(大幅摆头) */
#define SCAN_SERVO_SETTLE_MS     300U   /* 舵机换向后稳定等待(ms), 摆幅加大取余量 */
#define SCAN_TRIG_INTERVAL_MS     60U   /* 两次TRIG最小间隔(ms) */
#define SCAN_SAMPLES_PER_DIR       3U   /* 每方向采样次数, 取中值 */
#define SCAN_HOLD_MS            5000U   /* 方向保持时间(ms), 期满复测 */
#define SCAN_RECHECK_MS         5000U   /* 前方无物体时复测周期(ms) */
#define SCAN_NO_WAY_RETRY_MS    2000U   /* 两侧都有物体重试间隔(ms) */
#define SCAN_OBSTACLE_CM         25U   /* 有物体阈值(cm): 低于此判有物体 */

/* 人工暂停微调 */
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
    DIR_RESULT_BLOCKED = 0,  /* 有物体(有效且低于阈值) */
    DIR_RESULT_CLEAR,        /* 无物体(有效且不低于阈值) */
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
 * 描述     ： 单方向判定: 中值距离<阈值判有物体, 否则无物体
 * 输入     ： dm - 该方向采样上下文
 * 输出     ： 无
 * 返回     ： 判定结果(BLOCKED=有物体/CLEAR=无物体/UNKNOWN=未知)
 **************************************/
static dir_result_t scan_classify(const dir_measure_t *dm)
{
    uint32_t median;
    uint32_t tmp[SCAN_SAMPLES_PER_DIR];

    if (dm->valid_cnt < 2U)
    {
        return DIR_RESULT_UNKNOWN;   /* 有效样本<2 -> 未知, 按有物体处理 */
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
    return DIR_RESULT_CLEAR;
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
 * 描述     ： 方向决策: 前方有物体后比较左右, 转向无物体的一侧;
 *             左无右有->左, 右无左有->右, 两侧都无->保持原方向,
 *             两侧都有/未知->无路可走
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 选定方向(SCAN_DIR_xxx)
 **************************************/
static uint8_t scan_select_direction(void)
{
    uint8_t left_clear  = (g_res_left  == DIR_RESULT_CLEAR) ? 1U : 0U;
    uint8_t right_clear = (g_res_right == DIR_RESULT_CLEAR) ? 1U : 0U;

    if ((left_clear != 0U) && (right_clear != 0U))
    {
        /* 两侧都无物体: 保持原方向 */
        return g_prev_selected;
    }
    if (left_clear != 0U)
    {
        return SCAN_DIR_LEFT;
    }
    if (right_clear != 0U)
    {
        return SCAN_DIR_RIGHT;
    }

    /* 两侧都有物体(或未知按有物体) */
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
        /* 暂停: 舵机回中 */
        g_prev_selected = g_selected_dir;
        app_servo_set_angle(SCAN_ANGLE_CENTER_DEG);
        scan_set_state(SCAN_ST_PAUSED);
        return;
    }
    /* SW3: 位置复原(任意状态) - 舵机立即回中并重新检测正前方 */
    if (app_button_get_event(BUTTON_ID_SW3) != 0U)
    {
        g_prev_selected = g_selected_dir;
        app_servo_set_angle(SCAN_ANGLE_CENTER_DEG);
        scan_start_dir_measure();
        scan_set_state(SCAN_ST_CENTER_SETTLE);
        return;
    }

    if (g_state == SCAN_ST_PAUSED)
    {
        /* 暂停态手动微调: 受安全角度范围限制 */
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
                if (g_res_center == DIR_RESULT_BLOCKED)
                {
                    /* 前方有物体 -> 扫描左侧 */
                    app_servo_set_angle(SCAN_ANGLE_LEFT_DEG);
                    scan_set_state(SCAN_ST_MOVE_LEFT);
                }
                else
                {
                    /* 前方无物体(或未知): 保持中位待复测 */
                    g_selected_dir = SCAN_DIR_CENTER;
                    app_led_set_state(LED_STATE_NORMAL, SCAN_DIR_CENTER);
                    scan_set_state(SCAN_ST_FORWARD_HOLD);
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

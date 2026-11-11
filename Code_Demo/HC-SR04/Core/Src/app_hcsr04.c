/***********************************************************************************
 * @file      app_hcsr04.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V2.0.0
 * @date      2026-11-11
 * @brief     HC-SR04超声波测距模块(2020款IIC模式):
 *           IIC写0x01启动测距 -> 等200ms -> IIC读3字节换算距离
 *           (I2C2 -> PB10=SCL/PB11=SDA, 经P1接口接模块)
 *
 * @note      协议(2020款HC-SR04用户手册 三:IIC模式): 从机地址0x57,
 *           向写地址0xAE写0x01开始测距, 等待200ms(最大测距时间)
 *           以上, 从读地址0xAF读BYTE_H/M/L三个字节,
 *           距离mm=((H<<16)+(M<<8)+L)/1000。
 *           两次测量间隔不小于200ms, 由上层状态机保证。
 *           IIC读取阻塞约0.3ms(100kHz×3字节), 主循环可接受。
 * @history   V1.0.0 首版: 单次测距状态机+中值滤波接口           ---2026-11-11
 *           V2.0.0 改硬件IIC: 删TRIG/ECHO+TIM4捕获, 换I2C2收发   ---2026-11-11
 **********************************************************************************/
#include "app_hcsr04.h"

/* ==========================================================================
 *  私有类型定义
 * ========================================================================== */
/* 单次测距内部阶段 */
typedef enum
{
    HCSR04_SEQ_IDLE = 0,     /* 空闲, 可接受新触发 */
    HCSR04_SEQ_WAIT_ECHO     /* 已发启动命令, 等测距完成后读取 */
} hcsr04_seq_t;

/* ==========================================================================
 *  私有变量
 * ========================================================================== */
static hcsr04_seq_t g_seq = HCSR04_SEQ_IDLE;   /* 当前测距阶段 */
static uint32_t g_trig_tick = 0U;              /* 触发时刻(HAL tick) */
static uint32_t g_last_cm = HCSR04_DIST_INVALID; /* 最近一次有效距离(cm) */

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： app_hcsr04_init
 * 描述     ： 超声波模块初始化, 以0xAE地址探测模块是否应答
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 0=在线 1=无应答
 **************************************/
uint8_t app_hcsr04_init(void)
{
    g_seq = HCSR04_SEQ_IDLE;
    g_last_cm = HCSR04_DIST_INVALID;

    /* 短超时探测, 失败不阻塞启动: 上层按超时处理 */
    if (HAL_I2C_IsDeviceReady(&hi2c2, HCSR04_I2C_ADDR_WR, 3U, 10U) != HAL_OK)
    {
        return 1U;
    }
    return 0U;
}

/*************************************
 * 函数名称 ： app_hcsr04_trigger
 * 描述     ： 通过IIC向模块写0x01启动一次测距
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_hcsr04_trigger(void)
{
    uint8_t cmd = HCSR04_I2C_CMD_START;

    g_trig_tick = HAL_GetTick();
    g_last_cm = HCSR04_DIST_INVALID;

    if (HAL_I2C_Master_Transmit(&hi2c2, HCSR04_I2C_ADDR_WR, &cmd, 1U,
                                HCSR04_I2C_XFER_TIMEOUT_MS) != HAL_OK)
    {
        /* 发送失败: 保持IDLE, poll直接判超时 */
        g_seq = HCSR04_SEQ_IDLE;
        return;
    }
    g_seq = HCSR04_SEQ_WAIT_ECHO;
}

/*************************************
 * 触发后主循环轮询: 0=进行中 1=完成 2=超时/通信失败
 **************************************/
uint8_t app_hcsr04_poll(void)
{
    uint8_t buf[3];
    uint32_t now = HAL_GetTick();
    uint32_t raw;

    if (g_seq == HCSR04_SEQ_IDLE)
    {
        /* 未在测量(触发失败或已完成未重启) -> 超时语义 */
        return HCSR04_POLL_TIMEOUT;
    }

    /* 未到模块最大测距时间(手册200ms+余量)继续等 */
    if ((now - g_trig_tick) < HCSR04_ECHO_TIMEOUT_MS)
    {
        return HCSR04_POLL_BUSY;
    }

    /* 读3字节距离: BYTE_H/BYTE_M/BYTE_L (末字节NAK由HAL处理) */
    if (HAL_I2C_Master_Receive(&hi2c2, HCSR04_I2C_ADDR_RD, buf, 3U,
                               HCSR04_I2C_XFER_TIMEOUT_MS) != HAL_OK)
    {
        g_seq = HCSR04_SEQ_IDLE;
        return HCSR04_POLL_TIMEOUT;
    }
    g_seq = HCSR04_SEQ_IDLE;

    /* 原始值单位um: mm=raw/1000, cm=raw/10000; 全零=无效回波 */
    raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];
    if (raw == 0U)
    {
        return HCSR04_POLL_TIMEOUT;
    }
    g_last_cm = raw / 10000U;
    return HCSR04_POLL_DONE;
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
    return g_last_cm;
}

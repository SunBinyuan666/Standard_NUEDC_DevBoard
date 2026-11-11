/*******************************************************************************
 * @file      motor_tb6612.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     TB6612FNG双直流电机驱动公开接口
 *           速度采用千分比, 正数为前进, 负数为后退
 *
 * @history   V1.0.0 首版: 定义双电机控制接口                ---2026-11-12
 ******************************************************************************/

#ifndef __MOTOR_TB6612_H
#define __MOTOR_TB6612_H

#ifdef __cplusplus
extern "C" {
#endif

/* 头文件包含 -----------------------------------------------------------------*/
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/* 类型定义 -------------------------------------------------------------------*/
typedef enum
{
    MOTOR_A = 0,                   /* A路, 默认对应左电机 */
    MOTOR_B = 1,                   /* B路, 默认对应右电机 */
    MOTOR_COUNT = 2                /* 电机通道数量 */
} MotorId_t;

/* 函数声明 -------------------------------------------------------------------*/
void Motor_Init(void);
void Motor_Enable(bool enable);
void Motor_SetSpeed(MotorId_t id, int16_t speed);
void Motor_Stop(MotorId_t id);
void Motor_StopAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_TB6612_H */

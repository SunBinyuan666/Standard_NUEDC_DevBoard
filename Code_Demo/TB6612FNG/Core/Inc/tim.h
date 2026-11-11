/*******************************************************************************
 * @file      tim.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     TIM3双路PWM初始化接口
 *
 * @history   V1.0.0 首版: 添加TIM3初始化声明                ---2026-11-12
 ******************************************************************************/
/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */

#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 头文件包含 -----------------------------------------------------------------*/
#include "main.h"

/* 外部变量 -------------------------------------------------------------------*/
extern TIM_HandleTypeDef htim3;

/* 函数声明 -------------------------------------------------------------------*/
void MX_TIM3_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

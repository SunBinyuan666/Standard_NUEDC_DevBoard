/***********************************************************************************
 * @file      tim.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     定时器外设初始化接口(TIM3 PWM / TIM4 输入捕获)
 * @history   V1.0.0 首版: 句柄extern与初始化函数声明          ---2026-11-11
 **********************************************************************************/
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

void MX_TIM3_Init(void);
void MX_TIM4_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
#endif /* __TIM_H__ */

/*******************************************************************************
 * @file      gpio.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     GPIO初始化接口
 *
 * @history   V1.0.0 首版: 添加GPIO初始化声明                ---2026-11-12
 ******************************************************************************/
/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */

#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 头文件包含 -----------------------------------------------------------------*/
#include "main.h"

/* 函数声明 -------------------------------------------------------------------*/
void MX_GPIO_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_H__ */

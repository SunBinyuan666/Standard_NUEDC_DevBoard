/*******************************************************************************
 * @file      main.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     应用公共声明与TB6612FNG方向、待机引脚定义
 *
 * @history   V1.0.0 首版: 定义电机控制引脚                 ---2026-11-12
 ******************************************************************************/
/* USER CODE BEGIN Header */
/* 注: 文件信息见顶部规范文件头 (个人编程代码规范) */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* 头文件包含 -----------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* 函数声明 -------------------------------------------------------------------*/
void Error_Handler(void);

/* 私有定义 -------------------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
#define MOTOR_AIN1_Pin             GPIO_PIN_8
#define MOTOR_AIN1_GPIO_Port       GPIOE
#define MOTOR_AIN2_Pin             GPIO_PIN_9
#define MOTOR_AIN2_GPIO_Port       GPIOE
#define MOTOR_BIN1_Pin             GPIO_PIN_10
#define MOTOR_BIN1_GPIO_Port       GPIOE
#define MOTOR_BIN2_Pin             GPIO_PIN_11
#define MOTOR_BIN2_GPIO_Port       GPIOE
#define MOTOR_STBY_Pin             GPIO_PIN_12
#define MOTOR_STBY_GPIO_Port       GPIOE
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

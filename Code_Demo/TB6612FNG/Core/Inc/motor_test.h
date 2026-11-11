/*******************************************************************************
 * @file      motor_test.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-12
 * @brief     双电机非阻塞功能测试状态机接口
 *
 * @history   V1.0.0 首版: 定义单次测试流程接口              ---2026-11-12
 ******************************************************************************/

#ifndef __MOTOR_TEST_H
#define __MOTOR_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* 头文件包含 -----------------------------------------------------------------*/
#include <stdint.h>

/* 函数声明 -------------------------------------------------------------------*/
void MotorTest_Init(uint32_t now_ms);
void MotorTest_Process(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_TEST_H */

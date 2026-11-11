/***********************************************************************************
 * @file      app_obstacle_scan.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     超声波扫描避障状态机对外接口(设计要求§6.5)
 * @history   V1.0.0 首版: 方向定义+初始化/周期处理接口        ---2026-11-11
 **********************************************************************************/
#ifndef APP_OBSTACLE_SCAN_H
#define APP_OBSTACLE_SCAN_H

#include "stm32f1xx_hal.h"

/* 扫描方向编号(相对传感器安装方向; 舵机反向安装时交换左右映射) */
#define SCAN_DIR_CENTER   0U     /* 中位/正前方 */
#define SCAN_DIR_LEFT     1U     /* 左侧 */
#define SCAN_DIR_RIGHT    2U     /* 右侧 */
#define SCAN_DIR_NO_WAY   3U     /* 无安全方向哨兵(非物理方向) */

/*************************************
 * 函数名称 ： app_scan_init
 * 描述     ： 扫描状态机初始化, 舵机回中并启动首次测量周期
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_scan_init(void);

/*************************************
 * 函数名称 ： app_scan_process
 * 描述     ： 主循环周期调用; 内部驱动按键扫描/测距/方向决策/LED,
 *             全程非阻塞
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void app_scan_process(void);

#endif /* APP_OBSTACLE_SCAN_H */

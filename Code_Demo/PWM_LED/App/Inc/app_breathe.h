/********************************************************************************
 * @file      app_breathe.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-10-19
 * @brief     LED3 非阻塞呼吸控制模块接口。
 * @history   V1.0.0 建立呼吸控制模块接口。
 ********************************************************************************/
#ifndef APP_BREATHE_H
#define APP_BREATHE_H

/* 函数声明 -------------------------------------------------------------------*/
void app_breathe_init(void);
void app_breathe_process(void);
void app_breathe_deinit(void);

#endif /* APP_BREATHE_H */
